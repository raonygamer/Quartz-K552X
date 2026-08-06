#include "usb.h"

#include "gpio.h"
#include "SN32F240B.h"
#include "timer.h"
#include "utils.h"

#define USB_INT_MASK        0xF0000010
#define USB_CFG_DEFAULT     0xF8000000
#define USB_PHY_ENABLE      0x80000000
#define USB_PHY_DEFAULT     0x00004004

#define USB_CFG_VREG33_EN   0b10000000000000000000000000000000
#define USB_CFG_PHY_EN      0b01000000000000000000000000000000
#define USB_CFG_DPPU_EN     0b00100000000000000000000000000000
#define USB_CFG_SIE_EN      0b00010000000000000000000000000000
#define USB_CFG_ESD_EN      0b00001000000000000000000000000000

#define USB_CFG_BASE \
     (USB_CFG_VREG33_EN | \
     USB_CFG_PHY_EN    | \
     USB_CFG_SIE_EN    | \
     USB_CFG_ESD_EN)

#define USB_CFG_CONNECTED \
    (USB_CFG_BASE | USB_CFG_DPPU_EN)

#define USB_EPCTL_ENABLE       (1u << 31)
#define USB_EPCTL_STATE_NAK    (0u << 29)
#define USB_EPCTL_STATE_ACK    (1u << 29)
#define USB_EPCTL_STATE_STALL  (3u << 29)

void usb_init() {
    // Enable clock for USB.
    SN_SYS1->AHBCLKEN |= 0x10;

    // Keep USB interrupts disabled.
    SN_USB->INTEN = 0;

    // Keep USB interface disabled.
    SN_USB->SGCTL = 0x0;
    SN_USB->CFG = USB_CFG_BASE;

    // Set USB PHY Parameter register to 0x80000000
    SN_USB->PHYPRM  = USB_PHY_ENABLE;

    // Set USB PHY Parameter register 2 to 0x00004004
    SN_USB->PHYPRM2 = USB_PHY_DEFAULT;

    // Enable Internal VREG33 output.
    // Enable PHY Transceiver function.
    // Enable D+ Pull-up resistor.
    // Enable USB serial interface.
    // Enable USB ESD protection.
    SN_USB->CFG = USB_CFG_DEFAULT;

    // Wait for stabilization.
    wait(1);

    // Reset USB Bus.
    usb_bus_reset();

    // Enable Bus Event Interrupt.
    // Enable SOF Interrupt.
    // Enable USB Event Interrupt.
    // Enable Bus Wake Up Interrupt Enable.
    // Enable all of the endpoints ACK Interrupt.
    SN_USB->INTEN = USB_INT_MASK;

    // Clear pending interrupt for USB.
    // Enable interrupt for USB.
    NVIC_ClearPendingIRQ(USB_IRQn);
    NVIC_EnableIRQ(USB_IRQn);

    SN_USB->CFG = USB_CFG_CONNECTED;
}

#define INSTSC_CLEAR         0xFEFBFFFF
#define USB_CFG_EP1_OUT      (1u << 0)
#define USB_CFG_EP2_OUT      (1u << 1)
#define USB_CFG_EP3_OUT      (1u << 2)
#define USB_CFG_EP4_OUT      (1u << 3)

#define USB_CFG_DIS_PDEN     (1u << 26)
#define USB_CFG_ESD_EN       (1u << 27)
#define USB_CFG_SIE_EN       (1u << 28)
#define USB_CFG_DPPU_EN      (1u << 29)
#define USB_CFG_PHY_EN       (1u << 30)
#define USB_CFG_VREG33_EN    (1u << 31)

#define USB_REQUEST_TYPE_MASK      0x60u
#define USB_REQUEST_TYPE_STANDARD  0x00u
#define USB_REQUEST_TYPE_CLASS     0x20u
#define USB_REQUEST_TYPE_VENDOR    0x40u

struct __attribute__((packed)) usb_setup_packet_t {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
};

enum
{
    USB_INSTS_BUS_RESET     = 1u << 31,
    USB_INSTS_BUS_SUSPEND   = 1u << 30,
    USB_INSTS_BUS_RESUME    = 1u << 29,

    USB_INSTS_USB_SOF       = 1u << 26,
    USB_INSTS_BUS_WAKEUP    = 1u << 25,

    USB_INSTS_EP0_PRESETUP  = 1u << 24,
    USB_INSTS_EP0_SETUP     = 1u << 23,
    USB_INSTS_EP0_IN        = 1u << 22,
    USB_INSTS_EP0_OUT       = 1u << 21,

    USB_INSTS_EP0_IN_STALL  = 1u << 20,
    USB_INSTS_EP0_OUT_STALL = 1u << 19,

    USB_INSTS_ERR_SETUP     = 1u << 18,
    USB_INSTS_ERR_TIMEOUT   = 1u << 17,

    USB_INSTS_EP4_ACK       = 1u << 11,
    USB_INSTS_EP3_ACK       = 1u << 10,
    USB_INSTS_EP2_ACK       = 1u << 9,
    USB_INSTS_EP1_ACK       = 1u << 8,

    USB_INSTS_EP4_NAK       = 1u << 3,
    USB_INSTS_EP3_NAK       = 1u << 2,
    USB_INSTS_EP2_NAK       = 1u << 1,
    USB_INSTS_EP1_NAK       = 1u << 0,
};

#define USB_MAX_EP 4

void usb_connect() {
    // Configure USB
    SN_USB->CFG =
          USB_CFG_VREG33_EN
        | USB_CFG_PHY_EN
        | USB_CFG_DPPU_EN
        | USB_CFG_SIE_EN
        | USB_CFG_ESD_EN;

    // Set BUS_RESUME
    SN_USB->INSTSC = USB_INSTS_BUS_RESUME;
}

void usb_ep_stall(const uint8_t ep) {
    if (ep > USB_MAX_EP)
        return;
    if ((SN_USB->INSTS & USB_INSTS_EP0_PRESETUP) != 0)
        return;
    // Enable endpoint function
    // Set endpoint INOUT stall
    (&SN_USB->EP0CTL)[ep] = 0xE0000000;
}

void usb_disable_endpoint(const uint8_t ep) {
    if (ep > USB_MAX_EP)
        return;
    // Disable endpoint
    (&SN_USB->EP0CTL)[ep] = 0x0;
}

void usb_enable_endpoint(const uint8_t ep) {
    if (ep > USB_MAX_EP)
        return;
    // Disable endpoint
    (&SN_USB->EP0CTL)[ep] = 1u << 31;
}

void usb_bus_reset() {
    // Set USB device address to 0.
    SN_USB->ADDR = 0x0;
    SN_USB->EP0CTL =
        USB_EPCTL_ENABLE |
        USB_EPCTL_STATE_ACK;
    for (int i = 1; i < USB_MAX_EP; i++)
        usb_disable_endpoint(i);
}

#define USB_EP0_SETUP_CLEAR_MASK \
    (USB_INSTS_EP0_PRESETUP  | \
     USB_INSTS_EP0_SETUP     | \
     USB_INSTS_EP0_IN_STALL  | \
     USB_INSTS_EP0_OUT_STALL)

#define USB_REQ_GET_DESCRIPTOR     0x06
#define USB_REQ_SET_ADDRESS        0x05
#define USB_REQ_SET_CONFIGURATION  0x09

#define USB_DESC_DEVICE            0x01
#define USB_DESC_CONFIGURATION     0x02

uint32_t usb_sram_read32(uint32_t offset)
{
    SN_USB->RWADDR = offset;

    // Start a read transaction.
    SN_USB->RWSTATUS = 2;

    // Wait until the controller clears the busy/read bit.
    while (SN_USB->RWSTATUS & 2) {
    }

    return SN_USB->RWDATA;
}

static const uint8_t usb_device_descriptor[] = {
    18,         // bLength
    0x01,       // bDescriptorType = DEVICE
    0x00, 0x02, // bcdUSB = 2.00

    0xFF,       // bDeviceClass = vendor-specific
    0x00,       // bDeviceSubClass
    0x00,       // bDeviceProtocol

    64,         // bMaxPacketSize0

    0x45, 0x0c, // idVendor  = 0x1234
    0x28, 0x31, // idProduct = 0x5678

    0x00, 0x01, // bcdDevice = 1.00

    0,          // iManufacturer
    0,          // iProduct
    0,          // iSerialNumber

    1,          // bNumConfigurations
};

static const uint8_t usb_configuration_descriptor[] = {
    /*
     * Configuration descriptor
     */
    9,          // bLength
    0x02,       // bDescriptorType = CONFIGURATION
    18, 0,      // wTotalLength = 18 bytes
    1,          // bNumInterfaces
    1,          // bConfigurationValue
    0,          // iConfiguration
    0x80,       // bus-powered
    50,         // 100 mA

    /*
     * Interface descriptor
     */
    9,          // bLength
    0x04,       // bDescriptorType = INTERFACE
    0,          // bInterfaceNumber
    0,          // bAlternateSetting
    0,          // bNumEndpoints
    0xFF,       // vendor-specific class
    0x00,
    0x00,
    0,
};

static void usb_sram_write32(uint32_t offset, uint32_t value)
{
    SN_USB->RWADDR = offset;
    SN_USB->RWDATA = value;
    SN_USB->RWSTATUS = 1u;

    while ((SN_USB->RWSTATUS & 1u) != 0) {
    }
}

static void usb_sram_write(
    uint32_t offset,
    const uint8_t *data,
    uint32_t size
)
{
    while (size >= 4u) {
        uint32_t value =
              ((uint32_t)data[0] << 0)
            | ((uint32_t)data[1] << 8)
            | ((uint32_t)data[2] << 16)
            | ((uint32_t)data[3] << 24);

        usb_sram_write32(offset, value);

        offset += 4u;
        data += 4u;
        size -= 4u;
    }

    if (size != 0u) {
        uint32_t value = 0;

        for (uint32_t i = 0; i < size; ++i)
            value |= (uint32_t)data[i] << (i * 8u);

        usb_sram_write32(offset, value);
    }
}

void usb_ep_arm_in(uint8_t endpoint, uint32_t size)
{
    if (endpoint > USB_MAX_EP)
        return;

    (&SN_USB->EP0CTL)[endpoint] = 0xA0000000u | size;
}

static void usb_ep0_send(
    const uint8_t *data,
    uint16_t size,
    uint16_t requested_size
)
{
    if (size > requested_size)
        size = requested_size;

    if (size > 64)
        size = 64;

    usb_sram_write(0, data, size);
    usb_ep_arm_in(0, size);
}

static inline void usb_ep0_send_zlp(void)
{
    SN_USB->EP0CTL = 0xA0000000u;
}

static bool address_pending;
static uint8_t pending_address;

void usb_handle_standard_request(
    const struct usb_setup_packet_t *setup
)
{
    switch (setup->bRequest) {
        case USB_REQ_GET_DESCRIPTOR: {
            const uint8_t descriptor_type =
                (uint8_t)(setup->wValue >> 8);

            if (descriptor_type == USB_DESC_DEVICE) {
                usb_ep0_send(
                    usb_device_descriptor,
                    sizeof(usb_device_descriptor),
                    setup->wLength
                );
                return;
            }

            if (descriptor_type == USB_DESC_CONFIGURATION) {
                usb_ep0_send(
                    usb_configuration_descriptor,
                    sizeof(usb_configuration_descriptor),
                    setup->wLength
                );
                return;
            }

            usb_ep_stall(0);
            return;
        }

        case USB_REQ_SET_ADDRESS:
            pending_address = setup->wValue & 0x7F;
            address_pending = true;
            usb_ep0_send_zlp();
            return;

        case USB_REQ_SET_CONFIGURATION:
            usb_ep0_send_zlp();
            return;

        default:
            usb_ep_stall(0);
            return;
    }
}

void usb_handle_class_request(const struct usb_setup_packet_t* setup) {

}

void usb_ep0_handle_setup() {
    SN_USB->INSTSC = USB_EP0_SETUP_CLEAR_MASK;
    usb_enable_endpoint(0);
    uint32_t status = SN_USB->INSTS;

    struct usb_setup_packet_t setup;
    if (status & USB_INSTS_ERR_SETUP) {
        SN_USB->INSTSC = USB_INSTS_ERR_SETUP;
        return usb_ep_stall(0);
    }

    uint32_t word0 = usb_sram_read32(0);
    uint32_t word1 = usb_sram_read32(4);

    setup.bmRequestType = (uint8_t)(word0 >> 0);
    setup.bRequest      = (uint8_t)(word0 >> 8);
    setup.wValue        = (uint16_t)(word0 >> 16);
    setup.wIndex        = (uint16_t)(word1 >> 0);
    setup.wLength       = (uint16_t)(word1 >> 16);

    switch (setup.bmRequestType & 0x60u) {
        case USB_REQUEST_TYPE_STANDARD:
            return usb_handle_standard_request(&setup);
        case USB_REQUEST_TYPE_CLASS:
            return usb_handle_class_request(&setup);
        default:
            return usb_ep_stall(0);
    }
}

void USB_IRQHandler() {
    // Get USB Interrupt status.
    const uint32_t status = SN_USB->INSTS;

    // Clear USB Interrupt status.
    SN_USB->INSTSC = INSTSC_CLEAR;

    // No status.
    if (!status)
        return;

    // USB Bus reset.
    if (status & USB_INSTS_BUS_RESET) {
        usb_bus_reset();
    }

    if (status & USB_INSTS_EP0_SETUP) {
        usb_ep0_handle_setup();
    }
}
