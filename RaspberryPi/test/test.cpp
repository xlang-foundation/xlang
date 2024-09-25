/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include <stdio.h>
#include <vector>
#include <string>

class Base {
    public:
    virtual ~Base() {}
};

class Derived : public Base {};

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
    
    Derived* d = dynamic_cast<Derived*>(new Base());

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
