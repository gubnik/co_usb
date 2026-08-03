/**
 * USB mock device used for testing bulk endpoints with co_usb library
 * Copyright (C) 2026  Nikolay Gubankov aka nikgub

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * @file Mock USB device used for testing out co_usb.
 *
 * To use this device, include it in `hw/usb` directory in QEMU source tree.
 *
 * This device is intentionally minimal. In the future, I will extend it for isochronous and bulk
 * stream testing.
 */

// clang-format off
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/cutils.h"
#include "qemu/error-report.h"
#include "qemu/module.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "hw/usb.h"
#include "migration/vmstate.h"
#include "desc.h"
#include "qom/object.h"
#include "trace.h"
#include <stdio.h>
// clang-format on

struct USBMockState
{
    USBDevice dev;
    USBEndpoint *intr;
    char outbuf[16 * 256];
};

#define TYPE_USB_MOCK "usb-mock"
OBJECT_DECLARE_SIMPLE_TYPE(USBMockState, USB_MOCK)

enum
{
    STR_MANUFACTURER = 1,
    STR_PRODUCT,
    STR_SERIALNUMBER,
    STR_CONFIG_FULL,
    STR_CONFIG_HIGH,
    STR_CONFIG_SUPER,
};

static const USBDescStrings desc_strings = {
    [STR_MANUFACTURER] = "nikgub",
    [STR_PRODUCT]      = "Virtual mock device",
    [STR_SERIALNUMBER] = "1",
    [STR_CONFIG_SUPER] = "Super speed config (usb 3.0)",
};

static const USBDescIface desc_iface_super = {.bInterfaceNumber   = 0,
                                              .bNumEndpoints      = 2,
                                              .bInterfaceClass    = 0xff,
                                              .bInterfaceSubClass = 0xff,
                                              .bInterfaceProtocol = 0xff,
                                              .eps                = (USBDescEndpoint[]){
                                                  {
                                                      .bEndpointAddress = USB_DIR_IN | 0x01,
                                                      .bmAttributes     = USB_ENDPOINT_XFER_BULK,
                                                      .wMaxPacketSize   = 1024,
                                                      .bMaxBurst        = 15,
                                                  },
                                                  {
                                                      .bEndpointAddress = USB_DIR_OUT | 0x02,
                                                      .bmAttributes     = USB_ENDPOINT_XFER_BULK,
                                                      .wMaxPacketSize   = 1024,
                                                      .bMaxBurst        = 15,
                                                  },
                                              }};

static const USBDescDevice desc_device_super = {
    .bcdUSB             = 0x0310,
    .bMaxPacketSize0    = 8,
    .bNumConfigurations = 1,
    .confs =
        (USBDescConfig[]){
            {
                .bNumInterfaces      = 1,
                .bConfigurationValue = 1,
                .bmAttributes        = USB_CFG_ATT_ONE | USB_CFG_ATT_WAKEUP,
                .bMaxPower           = 50,
                .nif                 = 1,
                .ifs                 = &desc_iface_super,
            },
        },
};

static const USBDesc desc_mock = {
    .id =
        {
            .idVendor      = 0x9f9f,
            .idProduct     = 0x9f9f,
            .bcdDevice     = 0x9f9f,
            .iManufacturer = STR_MANUFACTURER,
            .iProduct      = STR_PRODUCT,
            .iSerialNumber = STR_SERIALNUMBER,
        },
    .super = &desc_device_super,
    .str   = desc_strings,
};

static void usb_mock_handler_reset (USBDevice *dev)
{
    (void)dev;
}

static void usb_mock_handle_control (USBDevice *dev, USBPacket *p, int request, int value,
                                     int index, int length, uint8_t *data)
{
    USBMockState *s = USB_MOCK(dev);
    USBBus *bus     = usb_bus_from_device(dev);
    int ret;

    (void)s;
    (void)bus;

    ret = usb_desc_handle_control(dev, p, request, value, index, length, data);
    if (ret >= 0)
    {
        return;
    }
}

static void usb_mock_token_in (USBMockState *s, USBPacket *p)
{
    static uint8_t buf[256 * 1024];
    int packet_len = p->iov.size;
    if (packet_len <= 0 || packet_len > sizeof(buf))
    {
        p->status = USB_RET_NAK;
        return;
    }
    memset(buf, 0xFF, packet_len);
    usb_packet_copy(p, buf, packet_len);
    p->status = USB_RET_SUCCESS;
    printf("Received and processed USB packet of size: %d\n", packet_len);
}

static void usb_mock_handle_data (USBDevice *dev, USBPacket *p)
{
    USBMockState *s = USB_MOCK(dev);
    uint8_t devep   = p->ep->nr;
    switch (p->pid)
    {
    case USB_TOKEN_OUT:
        if (devep != 2)
        {
            goto fail;
        }
        if (p->iov.size >= sizeof(s->outbuf))
        {
            p->status = USB_RET_NAK;
            break;
        }
        printf("Received:\n");
        usb_packet_copy(p, s->outbuf, p->iov.size);
        p->status = USB_RET_SUCCESS;
        printf("%.*s\n", (int)p->iov.size, s->outbuf);
        break;

    case USB_TOKEN_IN:
        if (devep != 1)
        {
            goto fail;
        }
        usb_mock_token_in(s, p);
        break;
    default:
    fail:
        p->status = USB_RET_STALL;
        break;
    }
}

static void usb_mock_realize (USBDevice *dev, Error **errp)
{
    USBMockState *s  = USB_MOCK(dev);
    Error *local_err = NULL;

    usb_desc_create_serial(dev);
    usb_desc_init(dev);
    dev->auto_attach = 0;

    usb_check_attach(dev, &local_err);
    if (local_err)
    {
        error_propagate(errp, local_err);
        return;
    }

    usb_mock_handler_reset(dev);
    if (!dev->attached)
    {
        usb_device_attach(dev, &error_abort);
    }
    s->intr = usb_ep_get(dev, USB_TOKEN_IN, 1);
}

static USBDevice *usb_mock_init (void)
{
    USBDevice *dev;
    dev = usb_new("usb-mock");
    return dev;
}

static void usb_mock_initfn (ObjectClass *klass, void *data)
{
    DeviceClass *dc    = DEVICE_CLASS(klass);
    USBDeviceClass *uc = USB_DEVICE_CLASS(klass);

    uc->usb_desc       = &desc_mock;
    uc->product_desc   = "Virtual mock device";
    uc->realize        = usb_mock_realize;
    uc->handle_attach  = usb_desc_attach;
    uc->handle_control = usb_mock_handle_control;
    uc->handle_data    = usb_mock_handle_data;
    uc->handle_reset   = usb_mock_handler_reset;
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
}

static const TypeInfo usb_mock_type_info = {
    .name          = TYPE_USB_MOCK,
    .parent        = TYPE_USB_DEVICE,
    .instance_size = sizeof(USBMockState),
    .abstract      = false,
    .class_init    = usb_mock_initfn,
};

static void usb_mock_register_types (void)
{
    type_register_static(&usb_mock_type_info);
    usb_legacy_register("usb-mock", "mock", usb_mock_init);
}

type_init(usb_mock_register_types)
