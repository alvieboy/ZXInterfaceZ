#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "defs.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <string.h>
#include "spectint.h"
#include "fpga.h"
#include "spectcmd.h"
#include "gpio.h"
#include "usb_ll.h"
// Target needs extra 4 bytes at start
#include <arpa/inet.h>
#include "videostreamer.h"
#include "interfacez_tasks.h"
#include "list.h"
#include <netinet/in.h>
#include "keyboard.h"

#define TAG "VIDEOSTREAMER"

static volatile unsigned interrupt_count = 0;
static uint32_t fb[SPECTRUM_FRAME_SIZE/4];
static uint32_t fb_prev[SPECTRUM_FRAME_SIZE/4];

#define MAX_FRAME_PAYLOAD 1024
#define MAX_PACKET_SIZE   1080

#define VIDEOSTREAMER_PACKET_LOGIN (0x00)
#define VIDEOSTREAMER_PACKET_KEYBOARD (0x01)

static dlist_t *clients = NULL;
static int server_sock;

struct video_client {
    struct sockaddr_in addr;
    unsigned error_counter;
};

struct client_packet {
    uint8_t cmd;
    union {
        struct {
            uint32_t addr;
            uint16_t port;
            uint8_t unused;
        } __attribute__((packed));
        struct {
            uint8_t keys[5];
            uint8_t joy[2];
        } __attribute__((packed));
    };
} __attribute__((packed));

extern void usb__trigger_interrupt(void);


struct frame {
    uint8_t seq:7;
    uint8_t val:1;    /* 1: */
    uint8_t frag:4;
    uint8_t fragsize_bits:4;
    uint8_t payload[MAX_FRAME_PAYLOAD];
};

static uint8_t video_packet[MAX_PACKET_SIZE];
static uint8_t *videodata;

static struct video_client *videostreamer__get_client_by_addr(const struct sockaddr_in *s);

static inline bool has_room_for_frame(unsigned payloadsize)
{
    payloadsize += 2;
    return ((videodata+payloadsize) < &video_packet[MAX_PACKET_SIZE]);
}

static inline int transmit_video_data(struct video_client *vc)
{
    int retries = 3;
    int r;
    do {
        //ESP_LOGI(TAG, "Send frame %d bytes", videodata - &video_packet[0]);
        r = sendto( server_sock, video_packet, videodata - &video_packet[0], 0,
                   (struct sockaddr*)&vc->addr, sizeof(struct sockaddr_in)
                  );
        if (r<0) {
           // ESP_LOGI(TAG, "ERR TX %d\n", r);
            if ((retries--)==0)
                return r; // TBD.
        }
    } while (r<0);

    return r;
}

static int fill_and_send_frames(struct video_client *vc,
                                uint8_t seq,
                                const uint8_t *data,
                                const uint8_t *prev_data)
{

    unsigned off = 0;
    uint8_t frag = 0;
    int r;
    unsigned size = SPECTRUM_FRAME_SIZE;
    uint8_t fragsize_log2 = __builtin_log2(MAX_FRAME_PAYLOAD);

    videodata = &video_packet[0];

    seq &= 0x7f;

    while (size) {
        bool need_send = 0;

        unsigned chunk = size > MAX_FRAME_PAYLOAD?MAX_FRAME_PAYLOAD:size;

        if ((seq & 15) == 15) {
            need_send = 1;
        } else {
            // Compare with previous data.
            if (memcmp(&data[off], &prev_data[off], chunk)!=0) {
                need_send = 1;
            }
        }

        // Update size immediatly so we can see if it's last fragment.
        size-=chunk;

        if (!need_send) {
            chunk = 0;
        }


        // Evaluate if it still fits
        //ESP_LOGI(TAG,"Test has_room_for_frame chunk %d", chunk);
        if (!has_room_for_frame(chunk)) {
            //ESP_LOGI(TAG,"No room, flush");
            // Send out what we have so far.
            r = transmit_video_data(vc);
            if (r<0) {
                ESP_LOGI(TAG,"Error TX");
                break;
            }
            videodata = &video_packet[0];
        }

        struct frame *f = (struct frame *)videodata;

        f->val = need_send;
        f->seq = seq;
        f->frag = frag;
        f->fragsize_bits = fragsize_log2;

        if (chunk>0)
            memcpy(f->payload, &data[off], chunk);
        off+=chunk;

        chunk += 2; // Size of header
        videodata += chunk;

        //ESP_LOGI(TAG, "Used space now %d", videodata - &video_packet[0]);

        if (size==0) {
            // Last chunk of data.
            r = transmit_video_data(vc);
            if (r<0) {
                break;
            }
            videodata = &video_packet[0];
        }
        frag++;
    }
    return 0;
}

const uint8_t *videostreamer__getlastfb()
{
    return (uint8_t*)fb;
}

unsigned videostreamer__getinterrupts()
{
    return interrupt_count;
}

static int videostreamer__handle_login(const struct sockaddr_in *sock, const struct client_packet *packet)
{
    struct video_client *vc;
    // Login.
    vc = videostreamer__get_client_by_addr(sock);
    if (vc!=NULL) {
        ESP_LOGE(TAG,"Client already registered!");
        return -1;
    }
    vc = malloc(sizeof(struct video_client));
    memcpy(&vc->addr, sock, sizeof(*sock));
    vc->error_counter = 0;
    clients = dlist__append(clients, vc);
    return 0;
}


static int videostreamer__handle_keyboard(const struct sockaddr_in *sock, const struct client_packet *packet)
{
    // Unpack keyboard data.
    uint64_t keys =
        ((uint64_t)packet->keys[0])<<0  |
        ((uint64_t)packet->keys[1])<<8  |
        ((uint64_t)packet->keys[2])<<16  |
        ((uint64_t)packet->keys[3])<<24  |
        ((uint64_t)packet->keys[4])<<32;
#ifdef __linux__
    ESP_LOGI(TAG,"Setting keyboard %016lx", keys);
#endif
    keyboard__set(keys);
    return 0;
}

static void videostreamer__handle_packet()
{
    struct sockaddr_in sock;
    socklen_t len = sizeof(sock);
    struct client_packet packet;

    // if client is behind a firewall, we might need to look at the actual IP+port inside the packet.
    int r = recvfrom(server_sock, &packet, sizeof(packet), 0, (struct sockaddr*)&sock, &len);
    if (r<0 || len!=sizeof(sock))
        return;
    if (r!=sizeof(packet)) {
        ESP_LOGE(TAG,"Invalid packet size received %d (expect %d)", r, sizeof(packet));
        return;
    }

    switch (packet.cmd) {
    case VIDEOSTREAMER_PACKET_LOGIN:
        videostreamer__handle_login(&sock, &packet);
        break;
    case VIDEOSTREAMER_PACKET_KEYBOARD:
        videostreamer__handle_keyboard(&sock, &packet);
        break;
    }
}

static void videostreamer__process_socket_data()
{
    // Process socket data.
    fd_set rfs;
    FD_ZERO(&rfs);
    FD_SET(server_sock, &rfs);
    struct timeval tv = {0,0};
    int r;
    r = select(server_sock+1, &rfs, NULL, NULL, &tv);
    //ESP_LOGI(TAG,"Select %d\n", r);
    switch (r) {
    case 0:
        break;
    case -1:
        break;
    default:
        if (FD_ISSET(server_sock, &rfs)) {
            videostreamer__handle_packet();
        }
        break;
    }
}

static void videostreamer__server_task(void *pvParameters)
{
    uint32_t io_num;
    uint8_t seqno = 0;
    interrupt_count = 0;

    unsigned framecounter = 0;
    ESP_LOGI(TAG, "VideoStreamer task initialised");
    do {
        io_num = spectint__getinterrupt(10);
        if(io_num)
        {
            uint8_t intstatus;
            // Get interrupt status
            intstatus = fpga__readinterrupt();
           // ESP_LOGI(TAG, "Spect int %08x", intstatus);

            if (intstatus & FPGA_INTERRUPT_USB) {
                // Command request
                usb__trigger_interrupt();
            }

            if (intstatus & FPGA_INTERRUPT_CMD) {
                // Command request
                spectcmd__request();
            }
            
            if (intstatus & FPGA_INTERRUPT_SPECT) {
                interrupt_count++;
                if (framecounter == 1) {
                    framecounter = 0;
                    //ESP_LOGI(TAG, "Frame");
                    if ( dlist__count(clients)>0 ) {
                        memcpy(fb_prev, fb, sizeof(fb_prev));
                        fpga__get_framebuffer(fb);
#if 0
                        if ((seqno & 0x3F)==0x00) {
                            ESP_LOGI(TAG, "Sending frames seq %d", seqno);
                        }
#endif

#ifdef __linux__
                        if (memcmp(fb, fb_prev, SPECTRUM_FRAME_SIZE)!=0) {
                            ESP_LOGI(TAG, "Frames mismatch");
                        }
#endif
                        dlist_t *client = clients;
                        while (client) {
                            struct video_client *vc = (struct video_client*) dlist__data(client);

                            int r = fill_and_send_frames( vc, seqno, (uint8_t*)fb, (uint8_t*)fb_prev);
                            if (r<0) {
                                vc->error_counter++;
                            } else {
                                vc->error_counter = 0;
                            }
                            if (vc->error_counter>32) {
                                ESP_LOGE(TAG, "Error sending to client socket: %d errno %d", r,errno);
                                client = dlist__remove(clients, client);
                                free(vc);
                            }
                            client = dlist__next(client);
                        }
                    } 
                    seqno++;
                } else {
                    framecounter++;
                }
            }
            // Re-enable
            fpga__ackinterrupt(intstatus);
        }
        //else {
        videostreamer__process_socket_data();
        //}
    } while (1);
}

static struct video_client *videostreamer__get_client_by_addr(const struct sockaddr_in *s)
{
    dlist_t *client = clients;
    struct video_client *vc = NULL;

    while (client) {
        struct video_client *this_vc = (struct video_client*) dlist__data(client);
        if ((this_vc->addr.sin_addr.s_addr == s->sin_addr.s_addr)
            && (this_vc->addr.sin_port == s->sin_port)) {
                vc = this_vc;
                break;
            }
        client = dlist__next(client);
    };

    return vc;
}

int videostreamer__init()
{
    // create socket.
    struct sockaddr_in bind_addr;

    server_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (server_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return -1;
    }

    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(8002);
    bind_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr*)&bind_addr, sizeof(struct sockaddr_in))!=0) {
        ESP_LOGE(TAG, "Unable to bind socket: errno %d", errno);
        return -1;
    }

    xTaskCreate(videostreamer__server_task, "streamer_task", VIDEOSTREAMER_TASK_STACK_SIZE, NULL, VIDEOSTREAMER_TASK_PRIORITY, NULL);

    return 0;
}
