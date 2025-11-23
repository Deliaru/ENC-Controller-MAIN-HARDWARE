#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include "tinyusb.h"

// Ontroller设备描述符 - 匹配VID_0E8F, PID_1216
#define USBD_VID 0x0E8F
#define USBD_PID 0x1216

// 设备描述符
static const tusb_desc_device_t winusb_device_descriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USBD_VID,
    .idProduct = USBD_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};

#endif // USB_DESCRIPTORS_H 