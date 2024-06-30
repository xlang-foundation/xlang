/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include <stdio.h>
#include <vector>
#include <string>

int main() {
#ifndef PICO_DEFAULT_LED_PIN
#warning blink example requires a board with a regular LED
#else
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    std::string xyz ="123";
    std::vector<int> ary;
    ary.push_back(1);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    while (true) {
        int x = ary[0];
        x=x+10;
        gpio_put(LED_PIN, x);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        printf("xxx");
        sleep_ms(250);
    }
#endif
}
