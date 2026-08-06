#pragma once
#include <stdbool.h>
#include "SN32F240B.h"

typedef enum {
    GPIO0 = 0,
    GPIO1 = 1,
    GPIO2 = 2,
    GPIO3 = 3,
} gpio_port_t;

void gpio_mode(gpio_port_t port, uint8_t pin, bool output);
void gpio_set(gpio_port_t port, uint8_t pin);
void gpio_clear(gpio_port_t port, uint8_t pin);
bool gpio_read(gpio_port_t port, uint8_t pin);
void gpio_toggle(gpio_port_t port, uint8_t pin);
void gpio_value(gpio_port_t port, uint8_t pin, bool value);
void gpio_mode_multiple(gpio_port_t port, uint32_t pins, bool output);