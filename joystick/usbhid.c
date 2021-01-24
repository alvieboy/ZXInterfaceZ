/*
  ******************************************************************************
  * (C) 2021 Alvaro Lopes <alvieboy@alvie.com>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of Alvaro Lopes nor the names of contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
#include "usbhid.h"
#include <stdbool.h>

static uint8_t usbhid__init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t usbhid__deinit(USBD_HandleTypeDef *pdev,uint8_t cfgidx);
static uint8_t usbhid__setup(USBD_HandleTypeDef *pdev,USBD_SetupReqTypedef *req);
static uint8_t *usbhid__getcfgdesc(uint16_t *length);
static uint8_t *usbhid__getdevicequalifierdesc(uint16_t *length);
static uint8_t usbhid__datain(USBD_HandleTypeDef *pdev, uint8_t epnum);
//static uint8_t usbhid__txsent(USBD_HandleTypeDef *pdev);

static USBD_ClassTypeDef usbhid_class =
{
  usbhid__init,
  usbhid__deinit,
  usbhid__setup,
  NULL, //usbhid__txsent, /*EP0_TxSent*/
  NULL, /*EP0_RxReady*/
  usbhid__datain, /*DataIn*/
  NULL, /*DataOut*/
  NULL, /*SOF */
  NULL,
  NULL,      
  usbhid__getcfgdesc,
  usbhid__getcfgdesc, 
  usbhid__getcfgdesc,
  usbhid__getdevicequalifierdesc,
};

#define USB_HID_CONFIG_DESC_SIZ 41

#define HID_DESCRIPTOR_TYPE 33
#define HID_REPORT_DESC_SIZE sizeof(usbhid__reportDescriptor)
#define HID_FS_BINTERVAL 10

#define USB_HID_DESC_SIZ 9

static uint8_t usbhid__reportDescriptor[]  =
{
  0x05, 0x01, // Item(Global): Usage Page, data= [ 0x01 ] 1 Generic Desktop Controls
  0x09, 0x04, // Usage, data= [ 0x04 ] 4 Joystick
  0xA1, 0x01, // Collection, data= [ 0x01 ] 1 Application
  0xA1, 0x02, // Collection, data= [ 0x02 ] 2 Logical
      0x85, 0x01, // Report ID (1)
      0x75, 0x02, // Report Size 2
      0x95, 0x02, // Report Count 2
      0x15, 0xFF, // Logical Minimum (-1)
      0x25, 0x01, // Logical Maximum (1)
      0x35, 0xFF, // Physical Minimum -1
      0x45, 0x01, // Physical Maximum (1)
      0x09, 0x30, // Usage, data= [ 0x30 ] 48 Direction-X
      0x09, 0x31, // Usage, data= [ 0x31 ] 49 Direction-Y
      0x81, 0x02, // Input, data= [ 0x02 ] 2 Data Variable Absolute No_Wrap Linear Preferred_State No_Null_Position Non_Volatile Bitfield

      0x75, 0x01, // Report Size, data= [ 0x01 ] 1
      0x95, 0x03, // Report Count, data= [ 0x03 ] 3
      0x15, 0x00, // Logical Minimum (0)
      0x25, 0x01, // Logical Maximum, data= [ 0x01 ] 1
      0x35, 0x00, // Physical Minimum 0
      0x45, 0x01, // Physical Maximum, data= [ 0x01 ] 1
      0x05, 0x09, // Usage Page, data= [ 0x09 ] 9 Buttons
      0x19, 0x01, // Usage Minimum, data= [ 0x01 ] 1 Button 1 (Primary)
      0x29, 0x03, // Usage Maximum, data= [ 0x03 ] 1 Button 3
      0x81, 0x02, // Input, data= [ 0x02 ] 2 Data Variable Absolute No_Wrap Linear Preferred_State No_Null_Position Non_Volatile Bitfield

      0x95, 0x01, // Report Count 1
      0x81, 0x03, // Input, const
    0xC0,       // End collection
  0xC0,  // End collection
  0xA1, 0x01, // Collection, data= [ 0x01 ] 1 Application
    0x05, 0x01, // Usage Page, data= [ 0x09 ] 1 Generic Desktop Controls
    0xA1, 0x02, // Collection, data= [ 0x02 ] 2 Logical
      0x85, 0x02, // Report ID (2)
      0x75, 0x02, // Report Size 2
      0x95, 0x02, // Report Count 2
      0x15, 0xFF, // Logical Minimum (-1)
      0x25, 0x01, // Logical Maximum (1)
      0x35, 0xFF, // Physical Minimum -1
      0x45, 0x01, // Physical Maximum (1)
      0x09, 0x30, // Usage, data= [ 0x30 ] 48 Direction-X
      0x09, 0x31, // Usage, data= [ 0x31 ] 49 Direction-Y
      0x81, 0x02, // Input, data= [ 0x02 ] 2 Data Variable Absolute No_Wrap Linear Preferred_State No_Null_Position Non_Volatile Bitfield

      0x75, 0x01, // Report Size, data= [ 0x01 ] 1
      0x95, 0x03, // Report Count, data= [ 0x03 ] 3
      0x15, 0x00, // Logical Minimum (0)
      0x25, 0x01, // Logical Maximum, data= [ 0x01 ] 1
      0x35, 0x00, // Physical Minimum 0
      0x45, 0x01, // Physical Maximum, data= [ 0x01 ] 1
      0x05, 0x09, // Usage Page, data= [ 0x09 ] 9 Buttons
      0x19, 0x01, // Usage Minimum, data= [ 0x01 ] 1 Button 1 (Primary)
      0x29, 0x03, // Usage Maximum, data= [ 0x03 ] 1 Button 3
      0x81, 0x02, // Input, data= [ 0x02 ] 2 Data Variable Absolute No_Wrap Linear Preferred_State No_Null_Position Non_Volatile Bitfield

      0x95, 0x01, // Report Count 1
      0x81, 0x03, // Input, const
    0xC0,       // End collection
  0xC0  // End collection

};


/* USB HID device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t usbhid__CfgDesc[USB_HID_CONFIG_DESC_SIZ]  __ALIGN_END =
{
  0x09, /* bLength: Configuration Descriptor size */
  USB_DESC_TYPE_CONFIGURATION, /* bDescriptorType: Configuration */
  USB_HID_CONFIG_DESC_SIZ,
  /* wTotalLength: Bytes returned */
  0x00,
  0x01,         /*bNumInterfaces: 1 interface*/
  0x01,         /*bConfigurationValue: Configuration value*/
  0x00,         /*iConfiguration: Index of string descriptor describing
  the configuration*/
  0xE0,         /*bmAttributes: bus powered and Support Remote Wake-up */
  0x32,         /*MaxPower 100 mA: this current is used for detecting Vbus*/
  
  /************** Descriptor of Joystick Mouse interface ****************/
  /* 09 */
  0x09,         /*bLength: Interface Descriptor size*/
  USB_DESC_TYPE_INTERFACE,/*bDescriptorType: Interface descriptor type*/
  0x00,         /*bInterfaceNumber: Number of Interface*/
  0x00,         /*bAlternateSetting: Alternate setting*/
  0x02,         /*bNumEndpoints*/

  0x03,         /*bInterfaceClass: HID*/
  0x00,         /*bInterfaceSubClass : 1=BOOT, 0=no boot*/
  0x00,         /*nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse*/
  0,            /*iInterface: Index of string descriptor*/
  /******************** Descriptor of Joystick Mouse HID ********************/
  /* 18 */
  0x09,         /*bLength: HID Descriptor size*/
  HID_DESCRIPTOR_TYPE, /*bDescriptorType: HID*/
  0x11,         /*bcdHID: HID Class Spec release number*/
  0x01,
  0x00,         /*bCountryCode: Hardware target country*/
  0x01,         /*bNumDescriptors: Number of HID class descriptors to follow*/
  0x22,         /*bDescriptorType*/
  HID_REPORT_DESC_SIZE,/*wItemLength: Total length of Report descriptor*/
  0x00,
  /******************** Descriptor of Mouse endpoint ********************/
  /* 27 */
  0x07,          /*bLength: Endpoint Descriptor size*/
  USB_DESC_TYPE_ENDPOINT, /*bDescriptorType:*/
  0x81,     /*bEndpointAddress: Endpoint Address (IN)*/
  0x03,          /*bmAttributes: Interrupt endpoint*/
  0x08, /*wMaxPacketSize: 4 Byte max */
  0x00,
  HID_FS_BINTERVAL,          /*bInterval: Polling Interval (10 ms)*/
  /* 34 */
  0x07,          /*bLength: Endpoint Descriptor size*/
  USB_DESC_TYPE_ENDPOINT, /*bDescriptorType:*/
  0x01,     /*bEndpointAddress: Endpoint Address (OUT)*/
  0x03,          /*bmAttributes: Interrupt endpoint*/
  0x08, /*wMaxPacketSize: 4 Byte max */
  0x00,
  HID_FS_BINTERVAL,          /*bInterval: Polling Interval (10 ms)*/
  /* 41 */
} ;

/* USB HID device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t usbhid__Desc[]  __ALIGN_END  =
{
  /* 18 */
  USB_HID_DESC_SIZ,         /*bLength: HID Descriptor size*/
  HID_DESCRIPTOR_TYPE, /*bDescriptorType: HID*/
  0x11,         /*bcdHID: HID Class Spec release number*/
  0x01,
  0x00,         /*bCountryCode: Hardware target country*/
  0x01,         /*bNumDescriptors: Number of HID class descriptors to follow*/
  0x22,         /*bDescriptorType*/
  HID_REPORT_DESC_SIZE,/*wItemLength: Total length of Report descriptor*/
  0x00,
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t usbhid__DeviceQualifierDesc[]  __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};


#define HID_EPIN_ADDR 0x81
#define HID_EPOUT_ADDR 0x01

#define HID_IDLE 0
#define HID_BUSY 1
#define HID_EPIN_SIZE 8


uint8_t usbhid__begin(USBD_HandleTypeDef *device, USBD_DescriptorsTypeDef *desc)
{
    USBD_Init(	device, desc, 0);
    /* Add Supported Class */
    USBD_RegisterClass(device, &usbhid_class);
    USBD_Start(device);
    return 0;
}

static uint8_t usbhid__init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  uint8_t ret = 0;
  
  /* Open EP IN */
  USBD_LL_OpenEP(pdev,
                 HID_EPIN_ADDR,
                 USBD_EP_TYPE_INTR,
                 HID_EPIN_SIZE);  
  
  pdev->pClassData = USBD_malloc(sizeof (usbhid_t));
  
  if(pdev->pClassData == NULL)
  {
    ret = 1; 
  }
  else
  {
    ((usbhid_t *)pdev->pClassData)->state = HID_IDLE;
  }
  return ret;
}

/**
  * @brief  usbhid__Init
  *         DeInitialize the HID layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  usbhid__deinit (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx)
{
  /* Close HID EPs */
  USBD_LL_CloseEP(pdev,
                  HID_EPIN_ADDR);
  
  /* FRee allocated memory */
  if(pdev->pClassData != NULL)
  {
    USBD_free(pdev->pClassData);
    pdev->pClassData = NULL;
  } 
  
  return USBD_OK;
}

/**
  * @brief  usbhid__Setup
  *         Handle the HID specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status



  */


#define HID_REQ_SET_PROTOCOL          0x0B
#define HID_REQ_GET_PROTOCOL          0x03

#define HID_REQ_SET_IDLE              0x0A
#define HID_REQ_GET_IDLE              0x02

#define HID_REQ_SET_REPORT            0x09
#define HID_REQ_GET_REPORT            0x01

//#define HID_DESCRIPTOR_TYPE           0x21
#define HID_REPORT_DESC               0x22

static uint8_t usbhid__setup(USBD_HandleTypeDef *pdev,
                             USBD_SetupReqTypedef *req)
{
    uint16_t len = 0;
    uint8_t  *pbuf = NULL;
    usbhid_t     *hhid = (usbhid_t*) pdev->pClassData;

    switch (req->bmRequest & USB_REQ_TYPE_MASK)
    {
    case USB_REQ_TYPE_CLASS :
        switch (req->bRequest)
        {


        case HID_REQ_SET_PROTOCOL:
            hhid->protocol = (uint8_t)(req->wValue);
            break;

        case HID_REQ_GET_PROTOCOL:
            USBD_CtlSendData (pdev,
                              (uint8_t *)&hhid->protocol,
                              1);
            break;

        case HID_REQ_SET_IDLE:
            // Report
            hhid->idlestate = (uint8_t)(req->wValue >> 8);
            break;

        case HID_REQ_GET_IDLE:
            USBD_CtlSendData (pdev,
                              (uint8_t *)&hhid->idlestate,
                              1);
            break;

        default:
            USBD_CtlError (pdev, req);
            return USBD_FAIL;
        }
        break;

    case USB_REQ_TYPE_STANDARD:
        switch (req->bRequest)
        {
        case USB_REQ_GET_DESCRIPTOR:
            if( req->wValue >> 8 == HID_REPORT_DESC)
            {
                len = MIN(HID_REPORT_DESC_SIZE , req->wLength);
                pbuf = usbhid__reportDescriptor;
            }
            else if( req->wValue >> 8 == HID_DESCRIPTOR_TYPE)
            {
                pbuf = usbhid__Desc;
                len = MIN(USB_HID_DESC_SIZ , req->wLength);
            }

            USBD_CtlSendData (pdev,
                              pbuf,
                              len);

            break;

        case USB_REQ_GET_INTERFACE :
            USBD_CtlSendData (pdev,
                              (uint8_t *)&hhid->altsetting,
                              1);
            break;

        case USB_REQ_SET_INTERFACE :
            hhid->altsetting = (uint8_t)(req->wValue);
            break;
        }
    }
    return USBD_OK;
}


/**
 * @brief  usbhid__SendReport
 *         Send HID Report
 * @param  pdev: device instance
 * @param  buff: pointer to report
 * @retval status
 */
uint8_t usbhid__sendreport(USBD_HandleTypeDef  *pdev,
                           uint8_t id,
                           uint8_t *report,
                           uint16_t len)
{
    usbhid_t *hhid = (usbhid_t*)pdev->pClassData;

    if (pdev->dev_state == USBD_STATE_CONFIGURED )
    {
        __disable_irq();

        uint8_t rid = id;
        memcpy(&hhid->report_data[rid], report, len);
        hhid->report_len[rid] = len;
        hhid->report_tx = rid;

        __enable_irq();

        if(hhid->state == HID_IDLE) {
            hhid->state = HID_BUSY;
            USBD_LL_Transmit (pdev,
                              HID_EPIN_ADDR,
                              hhid->report_data[rid],
                              hhid->report_len[rid]);

        }
    }
    return USBD_OK;
}


static uint8_t  *usbhid__getcfgdesc (uint16_t *length)
{
  *length = sizeof (usbhid__CfgDesc);
  return usbhid__CfgDesc;
}


/* Called within USB IRQ context */
static uint8_t usbhid__datain(USBD_HandleTypeDef *pdev,
                              uint8_t epnum)
{
    int i;
    usbhid_t *self  = (usbhid_t *)pdev->pClassData;

    if (epnum == HID_EPIN_ADDR) {

        // Clear report we just sent out.
        self->report_len[ self->report_tx ] = 0;

        for (i=0;i<2;i++) {
            if (self->report_len[i]>0) {
                self->report_tx = i;
                return USBD_LL_Transmit (pdev,
                                         HID_EPIN_ADDR,
                                         self->report_data[i],
                                         self->report_len[i]);
            }
        }
    }
    self->state = HID_IDLE;

    return USBD_OK;
}

static uint8_t  *usbhid__getdevicequalifierdesc (uint16_t *length)
{
  *length = sizeof (usbhid__DeviceQualifierDesc);
  return usbhid__DeviceQualifierDesc;
}


uint8_t usbhid__get_idle(USBD_HandleTypeDef *pdev)
{
    return ((usbhid_t *)pdev->pClassData)->idlestate;
}
