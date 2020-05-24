#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
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

volatile int client_socket = -1;
static volatile unsigned interrupt_count = 0;

static uint8_t fb[SPECTRUM_FRAME_SIZE];
static uint8_t fb_prev[SPECTRUM_FRAME_SIZE];

#define MAX_FRAME_PAYLOAD 1024

#define MAX_PACKET_SIZE   1080

struct frame {
    uint8_t seq:7;
    uint8_t val:1;    /* 1: */
    uint8_t frag:4;
    uint8_t fragsize_bits:4;
    uint8_t payload[MAX_FRAME_PAYLOAD];
};

static uint8_t video_packet[MAX_PACKET_SIZE];
static uint8_t *videodata;

static inline bool has_room_for_frame(unsigned payloadsize)
{
    payloadsize += 2;
    return ((videodata+payloadsize) < &video_packet[MAX_PACKET_SIZE]);
}

static inline int transmit_video_data(int socket)
{
    int retries = 3;
    int r;
    do {
        //ESP_LOGI(TAG, "Send frame %d bytes", videodata - &video_packet[0]);
        r = send( socket, video_packet, videodata - &video_packet[0], 0);
        if (r<0) {
           // ESP_LOGI(TAG, "ERR TX %d\n", r);
            if ((retries--)==0)
                return r; // TBD.
        }
    } while (r<0);

    return r;
}

static int fill_and_send_frames(int client_socket,
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

        if (seq & 1) {
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
            r = transmit_video_data(client_socket);
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
            r = transmit_video_data(client_socket);
            if (r<0) {
                break;
            }
            videodata = &video_packet[0];
        }
#if 0
        int r;
        do {
            if (need_send) {
                r = send( client_socket, &f, chunk, 0);
            } else {
                // Send header only
                r = send( client_socket, &f, 2, 0);
            }
            if (r<0) {
                retries--;
                if (retries==0)
                    return r;
            }
        } while (r<0);
#endif
        frag++;
    }
    return 0;
}

const uint8_t *videostreamer__getlastfb()
{
    return fb;
}

unsigned videostreamer__getinterrupts()
{
    return interrupt_count;
}

static void videostreamer__server_task(void *pvParameters)
{
    uint32_t io_num;
    uint8_t seqno = 0;
    int error_counter = 0;
    interrupt_count = 0;

    unsigned framecounter = 0;
    ESP_LOGI(TAG, "VideoStreamer task initialised");
    do {
        io_num = spectint__getinterrupt();
        //ESP_LOGI(TAG, "I %d", io_num);
        if(io_num)
        {
            if (io_num==PIN_NUM_SPECT_INTERRUPT) {
                
                framecounter++;
                interrupt_count++;

                if (framecounter == 1) {
                    framecounter = 0;
                    memcpy(fb_prev, fb, sizeof(fb_prev));
                    fpga__get_framebuffer(fb);
                    if (client_socket>0) {
                        ESP_LOGI(TAG, "Sending frames seq %d", seqno);
                        int r = fill_and_send_frames( client_socket, seqno, &fb[4], &fb_prev[4]);
                        if (r<0) {
                            error_counter++;
                        } else {
                            error_counter = 0;
                        }
                        if (error_counter>32) {
                            ESP_LOGE(TAG, "Error sending to client socket: %d errno %d", r,errno);
                            shutdown(client_socket,3);
                            close(client_socket);
                            client_socket = -1;
                        }
                    } else {
                        error_counter = 0;
                    }
                    seqno++;
                } else {
                    framecounter++;
                }
            } else if (io_num==PIN_NUM_CMD_INTERRUPT) {
                // Command request
                spectcmd__request();
            } else if (io_num==PIN_NUM_IO0) {
                // Switch request
                ESP_LOGI(TAG, "IO0 pressed");
                gpio__press_event(PIN_NUM_IO0);
            } else if (io_num==PIN_NUM_SWITCH) {
                // Switch request
                ESP_LOGI(TAG, "Switch pressed");
                gpio__press_event(PIN_NUM_SWITCH);
            } /*else if (io_num==PIN_NUM_USB_INTERRUPT) {
                usb_ll__interrupt();
            }   */
        }
    } while (1);
}


void videostreamer__init()
{
    xTaskCreate(videostreamer__server_task, "streamer_task", 4096, NULL, 10, NULL);
}

int videostreamer__start_stream(struct in_addr addr, uint16_t port)//command_t *cmdt, int argc, char **argv)
{
    struct sockaddr_in s;
    int r = -1;

    do {
        // create socket.
        int tsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (tsock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        s.sin_family = AF_INET;
        s.sin_port = htons(port);

        s.sin_addr = addr;

        int err = connect(tsock, (struct sockaddr *)&s, sizeof(s));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }

        ESP_LOGI(TAG, "Socket created, %s:%d", inet_ntoa(s.sin_addr.s_addr), (int)port);
        client_socket = tsock;
        r = 0;

    } while (0);

    return r;
}
