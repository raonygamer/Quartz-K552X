#include "gpio.h"


void gpio_mode(const gpio_port_t port, const uint8_t pin, const bool output) {
    switch (port) {
        case 0:
            SN_GPIO0->MODE = (SN_GPIO0->MODE & ~(1u << pin)) | ((uint32_t)output << pin);
            break;
        case 1:
            SN_GPIO1->MODE = (SN_GPIO1->MODE & ~(1u << pin)) | ((uint32_t)output << pin);
            break;
        case 2:
            SN_GPIO2->MODE = (SN_GPIO2->MODE & ~(1u << pin)) | ((uint32_t)output << pin);
            break;
        case 3:
            SN_GPIO3->MODE = (SN_GPIO3->MODE & ~(1u << pin)) | ((uint32_t)output << pin);
            break;
        default:
            break;
    }
}

void gpio_mode_multiple(const gpio_port_t port, const uint32_t pins, const bool output) {
    switch (port) {
        case 0:
            SN_GPIO0->MODE = pins & (output ? 0xFFFFFFFF : 0x0);
            break;
        case 1:
            SN_GPIO1->MODE = pins & (output ? 0xFFFFFFFF : 0x0);
            break;
        case 2:
            SN_GPIO2->MODE = pins & (output ? 0xFFFFFFFF : 0x0);
            break;
        case 3:
            SN_GPIO3->MODE = pins & (output ? 0xFFFFFFFF : 0x0);
            break;
        default:
            break;
    }
}

void gpio_set(const gpio_port_t port, const uint8_t pin) {
    switch (port) {
        case 0:
            SN_GPIO0->BSET = 1u << pin;
            break;
        case 1:
            SN_GPIO1->BSET = 1u << pin;
            break;
        case 2:
            SN_GPIO2->BSET = 1u << pin;
            break;
        case 3:
            SN_GPIO3->BSET = 1u << pin;
            break;
        default:
            break;
    }
}

void gpio_clear(const gpio_port_t port, const uint8_t pin) {
    switch (port) {
        case 0:
            SN_GPIO0->BCLR = 1u << pin;
            break;
        case 1:
            SN_GPIO1->BCLR = 1u << pin;
            break;
        case 2:
            SN_GPIO2->BCLR = 1u << pin;
            break;
        case 3:
            SN_GPIO3->BCLR = 1u << pin;
            break;
        default:
            break;
    }
}

bool gpio_read(const gpio_port_t port, const uint8_t pin) {
    switch (port) {
        case 0:
            return (SN_GPIO0->DATA >> pin) & 1;
        case 1:
            return (SN_GPIO1->DATA >> pin) & 1;
        case 2:
            return (SN_GPIO2->DATA >> pin) & 1;
        case 3:
            return (SN_GPIO3->DATA >> pin) & 1;
        default:
            return false;
    }
}

void gpio_toggle(const gpio_port_t port, const uint8_t pin) {
    if (gpio_read(port, pin))
        gpio_clear(port, pin);
    else
        gpio_set(port, pin);
}

void gpio_value(const gpio_port_t port, const uint8_t pin, const bool value) {
    if (value)
        gpio_set(port, pin);
    else
        gpio_clear(port, pin);
}

