#include "dump.h"
#include "esp_log.h"
#include "defs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "usbh.h"
#include "usbdriver.h"

static xQueueHandle usb_cmd_queue = NULL;
static xQueueHandle usb_cplt_queue = NULL;

struct usbcmd
{
    int cmd;
    void *data;
};

struct usbresponse
{
    int status;
    void *data;
};

static uint8_t usb_address = 0;

#define USB_CMD_INTERRUPT 0
#define USB_CMD_SUBMITREQUEST 1

#define USB_STAT_ATTACHED 1
#define USB_REQUEST_COMPLETED_OK 2
#define USB_REQUEST_COMPLETED_FAIL 3


static void usbh__submit_request_to_ll(struct usb_request *req);
static int usbh__assign_address(struct usb_device *dev);
static int usbh__control_request_completed_reply(uint8_t chan, uint8_t stat, struct usb_request *req);
static int usbh__request_completed_reply(uint8_t chan, uint8_t stat, void *req);

static uint8_t usbh__allocate_address()
{
    usb_address++;
    if (usb_address>127) {
        usb_address = 1;
    }
    return usb_address;
}

void usb_ll__connected_callback()
{
    struct usbresponse resp;

    // Connected device.
    ESP_LOGI(TAG,"USB connection event");
    resp.status = USB_STAT_ATTACHED;

    if (xQueueSend(usb_cplt_queue, &resp, 0)!=pdTRUE) {
        ESP_LOGE(TAG, "Cannot queue!!!");
    }
}

void usbh__ll_task(void *pvParam)
{
    struct usbcmd cmd;
    while (1) {
        if (xQueueReceive(usb_cmd_queue, &cmd, portMAX_DELAY)==pdTRUE)
        {
            if (cmd.cmd==USB_CMD_INTERRUPT) {
                usb_ll__interrupt();
            } else {
                // Handle command, pass to low-level if needed.
                if (cmd.cmd==USB_CMD_SUBMITREQUEST) {
                    struct usb_request *req = (struct usb_request*)cmd.data;
                    usbh__submit_request_to_ll(req);
                }
            }
        }
    }
}

void IRAM_ATTR usb__isr_handler(void* arg)
{
    struct usbcmd cmd;
    cmd.cmd = USB_CMD_INTERRUPT;
    xQueueSendFromISR(usb_cmd_queue, &cmd, NULL);
}


void usbh__reset()
{
    usb_ll__reset();

    vTaskDelay(10 / portTICK_RATE_MS);
}

int usbh__detect_and_assign_address()
{
    ESP_LOGI(TAG, "Resetting BUS");
    struct usb_device dev;
    usb_config_descriptor_t cd;

    dev.ep0_chan = 0;
    dev.ep0_size = 64; // Default
    dev.address = 0;

    usbh__reset();

    ESP_LOGI(TAG, "Requesting device descriptor");

    if (usbh__get_descriptor(&dev,
                             USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
                             USB_DESC_DEVICE,
                             (uint8_t*)&dev.device_descriptor,
                             USB_LEN_DEV_DESC)<0) {
        ESP_LOGE(TAG, "Cannot fetch device descriptor");
        return -1;
    }


    ESP_LOGI(TAG," new USB device, vid=0x%04x pid=0x%04x",
             __le16(dev.device_descriptor.idVendor),
             __le16(dev.device_descriptor.idProduct)
            );
    // Update EP0 size

    dev.ep0_size = dev.device_descriptor.bMaxPacketSize;

    if (usbh__assign_address(&dev)<0)
        return -1;

    ESP_LOGI(TAG," assigned address %d", dev.address);

    // Allocate channel for EP0

    dev.ep0_chan = usb_ll__alloc_channel(dev.address,
                                         EP_TYPE_CONTROL,
                                         dev.device_descriptor.bMaxPacketSize,
                                         0x00
                                        );
    dev.config_descriptor = NULL;

    ESP_LOGI(TAG, "Allocated channel %d for EP0 of %d\n", dev.ep0_chan, dev.address);
    // Get configuration header. (9 bytes)


    if (usbh__get_descriptor(&dev,
                             USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
                             USB_DESC_CONFIGURATION,
                             (uint8_t*)&cd,
                             USB_LEN_CFG_DESC) <0) {
        ESP_LOGE(TAG, "Cannot fetch configuration descriptor");
        return -1;
    }

    if (cd.bDescriptorType!=USB_DESC_TYPE_CONFIGURATION) {
        ESP_LOGE(TAG,"Invalid config descriptor");
        return -1;
    }

    uint16_t configlen = __le16(cd.wTotalLength);

    dev.config_descriptor = malloc(configlen);

    if (dev.config_descriptor==NULL) {
        // TBD
        //free(req);
        ESP_LOGE(TAG,"Cannot allocate memory");
        return -1;
    }

    if (usbh__get_descriptor(&dev,
                             USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_DEVICE,
                             USB_DESC_CONFIGURATION,
                             (uint8_t*)dev.config_descriptor,
                             configlen
                            )<0) {
        ESP_LOGE(TAG, "Cannot read full config descriptor!");
        free(dev.config_descriptor);
        return -1;
    }

    if (usbdriver__probe(&dev)) {

    }  else {
        // No driver.
        free(dev.config_descriptor);
    }
    return 0;
}

int usbh__wait_completion(struct usb_request *req)
{
    struct usbresponse resp;
    if (xQueueReceive(usb_cplt_queue, &resp, portMAX_DELAY)==pdTRUE)
    {
        ESP_LOGI(TAG,"USB completed status %d", resp.status);
        if (resp.status == USB_REQUEST_COMPLETED_OK) {
            if (resp.data == req) {
                return 0;
            } else {
                ESP_LOGE(TAG, "NOT FOR US!");
                return -1;
            }
        } else {
            // Error response
            if (resp.data == req) {
                ESP_LOGE(TAG, "Status error!");
                return -1;
            } else {
                ESP_LOGE(TAG, "NOT FOR US!");
                return -1;
            }
        }
    }
    return -1;
}

void usbh__main_task(void *pvParam)
{
    struct usbresponse resp;

    while (1) {
        ESP_LOGI(TAG, "Wait for completion responses");
        if (xQueueReceive(usb_cplt_queue, &resp, portMAX_DELAY)==pdTRUE)
        {
            ESP_LOGI(TAG, " >>> got status %d", resp.status);
            if (resp.status == USB_STAT_ATTACHED) {
                // Attached.
                usbh__detect_and_assign_address();
            }
        } else {
            ESP_LOGE(TAG, "Queue error!!!");
        }
    }
}








static void usbh__request_completed(struct usb_request *req);


static void usbh__request_failed(struct usb_request *req)
{
    //switch (req->device->state) {
    //}
    //    free(req);
    struct usbresponse resp;
    resp.status = USB_REQUEST_COMPLETED_FAIL;
    resp.data = req;
    xQueueSend(usb_cplt_queue, &resp, portMAX_DELAY);
    taskYIELD();
}

static inline int usbh__request_data_remain(const struct usb_request*req)
{
    // TODO: ensure this is smaller than EP size
    int remain = req->length - req->size_transferred;

    if (remain > req->epsize) {
        remain = req->epsize;
    }
    ESP_LOGI(TAG, "Chan %d: remain %d length req %d transferred %d", req->channel,
             remain,
             req->length,
             req->size_transferred);

    return remain;
}

static int usbh__send_ack(struct usb_request *req)
{
    // Send ack
    return usb_ll__submit_request(req->channel,
                                  req->epmemaddr,
                                  PID_OUT,
                                  1,      // TODO: Check seq.
                                  NULL,
                                  0,
                                  usbh__request_completed_reply,
                                  req);
}

static int usbh__wait_ack(struct usb_request *req)
{
    return usb_ll__submit_request(req->channel,
                                  req->epmemaddr,
                                  PID_IN,
                                  1,      // TODO: Check seq.
                                  NULL,
                                  0,
                                  usbh__request_completed_reply,
                                  req);

}

static int usbh__issue_setup_data_request(struct usb_request *req)
{
    int remain = usbh__request_data_remain(req);

    if (req->direction==REQ_DEVICE_TO_HOST) {
        // send IN request.
        return usb_ll__submit_request(req->channel,
                                      req->epmemaddr,
                                      PID_IN,
                                      1, // TODO
                                      req->rptr,
                                      remain,
                                      usbh__request_completed_reply,
                                      req);
    } else {
        // send OUT request.
        return usb_ll__submit_request(req->channel,
                                      req->epmemaddr,
                                      PID_OUT,
                                      1, // TODO
                                      req->rptr,
                                      remain,
                                      usbh__request_completed_reply,
                                      req);
    }
}

/*
 Example transfers:

 H2D, no data phase:
                             SM state
 * -> SETUP                  send, enter CONTROL_STATE_SETUP
 * <- ACK                    Interrupt, send IN request, enter CONTROL_STATE_STATUS
 * -> IN
 * <- DATAX(0len)
 * -> ACK                    Interrupt, completed

 H2D, data phase:

 * -> SETUP                  send, enter CONTROL_STATE_SETUP
 * <- ACK                    Interrupt, send OUT request, enter CONTROL_STATE_DATA
 * -> OUT  ]
 * <- ACK  ] times X         Interrupt, if more send OUT, stay, , else send IN, enter CONTROL_STATE_STATUS
 * -> IN
 * <- DATAX(0len)
 * <- ACK                    Interrupt, completed

 D2H, data phase:

 * -> SETUP                  send, enter CONTROL_STATE_SETUP
 * <- ACK                    Interrupt, send IN request, enter CONTROL_STATE_DATA
 * -> IN  ]
 * <- ACK  ] times X         Interrupt, if more send IN, stay , else send OUT(0), enter CONTROL_STATE_STATUS
 * -> OUT (0len)
 * <- ACK                    Interrupt, completed.

 */


static int usbh__control_request_completed_reply(uint8_t chan, uint8_t stat, struct usb_request *req)
{
    uint8_t rxlen = 0;

    if (stat & 1) {

        switch (req->control_state) {
        case CONTROL_STATE_SETUP:
            /* Setup completed. If we have a data phase, send data */
            req->size_transferred = 0;
            ESP_LOGI(TAG,"Requested size: %d\n", req->length);
            if (req->length) {
                req->control_state = CONTROL_STATE_DATA;
                usbh__issue_setup_data_request(req);



            } else  {
                // No data phase.
                req->control_state = CONTROL_STATE_STATUS;

                if (req->direction==REQ_HOST_TO_DEVICE) {
                    // Wait ack
                    return usbh__wait_ack(req);
                } else {
                    return usbh__send_ack(req);
                }

            }
            break;


        case CONTROL_STATE_DATA:

            // IN request
            if (req->direction==REQ_DEVICE_TO_HOST) {
                usb_ll__read_in_block(chan, req->rptr, &rxlen);
            } else {

            }
            req->rptr += rxlen;
            req->size_transferred += rxlen;

            if (usbh__request_data_remain(req)>0) {
                ESP_LOGI(TAG, "Still data to go");
                return usbh__issue_setup_data_request(req);
            } else {
                ESP_LOGI(TAG, "Entering status phase");

                req->control_state = CONTROL_STATE_STATUS;

                if (req->direction==REQ_HOST_TO_DEVICE) {
                    // Wait ack
                    ESP_LOGI(TAG, "Wait ACK");
                    return usbh__wait_ack(req);
                } else {
                    ESP_LOGI(TAG, "Send ACK");
                    return usbh__send_ack(req);
                }

            }

            break;

        case CONTROL_STATE_STATUS:
            usbh__request_completed(req);
        }



    } else {
        ESP_LOGE(TAG, "Request failed");
        usbh__request_failed(req);
        return -1;
    }
    return 0;

}
int usbh__request_completed_reply(uint8_t chan, uint8_t stat, void *data)
{
    struct usb_request *req = (struct usb_request*)data;

    if (req->control) {
        return usbh__control_request_completed_reply(chan,stat,req);
    }

    return 0;
}

void usbh__submit_request(struct usb_request *req)
{
    struct usbcmd cmd;
    cmd.cmd = USB_CMD_SUBMITREQUEST;
    cmd.data = req;
    xQueueSend( usb_cmd_queue, &cmd, portMAX_DELAY);
    taskYIELD();
}


static void usbh__submit_request_to_ll(struct usb_request *req)
{
    if (req->control) {
        usb_ll__submit_request(req->channel,
                               req->epmemaddr,
                               PID_SETUP, 1,
                               req->setup.data,
                               sizeof(req->setup),
                               usbh__request_completed_reply,
                               req);
    } else {
        if (req->direction==REQ_DEVICE_TO_HOST) {
            usb_ll__submit_request(req->channel,
                                   req->epmemaddr,
                                   PID_IN, 0,
                                   req->target,
                                   req->length > 64 ? 64: req->length,
                                   usbh__request_completed_reply,
                                   req);
        } else {
            if (req->length) {
                usb_ll__submit_request(req->channel,
                                       req->epmemaddr,
                                       PID_OUT, 0,
                                       req->target,
                                       req->length > 64 ? 64: req->length,
                                       usbh__request_completed_reply,
                                       req);
            }
        }

    }
}

int usbh__get_descriptor(struct usb_device *dev,
                         uint8_t  req_type,
                         uint16_t value,
                         uint8_t* target,
                         uint16_t length)
{
    struct usb_request req = {0};

    req.device = dev;
    req.target = target;
    req.length = length;
    req.rptr = target;
    req.size_transferred = 0;
    req.direction = REQ_DEVICE_TO_HOST;
    req.control = 1;
    req.channel = dev->ep0_chan;
    req.epsize = dev->ep0_size;


    req.setup.bmRequestType = req_type | USB_DEVICE_TO_HOST;
    req.setup.bRequest = USB_REQ_GET_DESCRIPTOR;
    req.setup.wValue = __le16(value);

    if ((value & 0xff00) == USB_DESC_STRING)
    {
        // Language
        req.setup.wIndex = __le16(0x0409);
    }
    else
    {
        req.setup.wIndex = __le16(0);
    }
    req.setup.wLength = length;
    req.channel =  dev->ep0_chan;
    usbh__submit_request(&req);

    // Wait.
    if (usbh__wait_completion(&req)<0) {
        return -1;
    }

    return 0;
}

static int usbh__assign_address(struct usb_device *dev)
{
    struct usb_request req = {0};

    uint8_t newaddress = usbh__allocate_address();

    req.device = dev;
    req.target = NULL;
    req.length = 0;
    req.rptr = NULL;
    req.size_transferred = 0;
    req.direction = REQ_HOST_TO_DEVICE;
    req.control = 1;

    req.setup.bmRequestType = USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD | USB_HOST_TO_DEVICE;
    req.setup.bRequest = USB_REQ_SET_ADDRESS;
    req.setup.wValue = __le16(newaddress);
    req.setup.wIndex = __le16(0);
    req.setup.wLength = __le16(0);

    req.channel =  dev->ep0_chan;
    ESP_LOGI(TAG,"Submitting address request");
    usbh__submit_request(&req);

    // Wait.
    if (usbh__wait_completion(&req)<0) {
        ESP_LOGE(TAG,"Device not accepting address!");
        return -1;
    }
    ESP_LOGI(TAG, "Submitted address request");
    dev->address = newaddress;

    return 0;
}

void usb_ll__disconnected_callback()
{
    ESP_LOGE(TAG, "Disconnected callback");
}

void usb_ll__overcurrent_callback()
{
    ESP_LOGE(TAG, "Overcurrent");
}

static void usbh__request_completed(struct usb_request *req)
{
    struct usbresponse resp;
    resp.status = USB_REQUEST_COMPLETED_OK;
    resp.data = req;
    xQueueSend(usb_cplt_queue, &resp, portMAX_DELAY);
    taskYIELD();
}

int usbh__init()
{
    usb_cmd_queue  = xQueueCreate(4, sizeof(struct usbcmd));
    usb_cplt_queue = xQueueCreate(4, sizeof(struct usbresponse));

    xTaskCreate(usbh__ll_task, "usbh_ll_task", 4096, NULL, 10, NULL);
    xTaskCreate(usbh__main_task, "usbh_main_task", 4096, NULL, 10, NULL);

    usb_ll__init();

    return 0;
}
