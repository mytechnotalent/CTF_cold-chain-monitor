// MIT License
//
// Copyright (c) 2026 Kevin Thomas
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// Author:  Kevin Thomas
// Email:   kevin@mytechnotalent.com
// GitHub:  https://github.com/mytechnotalent/cold-chain-monitor-c-rp2350
// File:    button.c
// Desc:    Implements the debounced operator acknowledge push-button input.
// Created: 2026

#include "pico/stdlib.h"
#include "pico/time.h"
#include "button.h"
#include "cold_chain_monitor.h"
#include "hardware/gpio.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Time in microseconds of the most recent accepted press.
 */
static uint64_t g_button_last_press_us;

/**
 * @brief True once at least one press has been accepted since reset.
 */
static bool g_button_seen;

/**
 * @brief True when the current press has already been consumed.
 */
static bool g_button_consumed;

/**
 * @brief Report whether a fresh debounced press edge is pending.
 *
 * Clears the consumed latch while the button is released so the next
 * press can be observed.
 *
 * @param void No parameters.
 * @return bool true when an unconsumed press edge is present.
 */
static bool button_edge_pending(void) {
    if (!button_pressed()) {
        g_button_consumed = false;
        return false;
    }
    return !g_button_consumed;
}

/**
 * @brief Apply the debounce window to the pending press.
 *
 * @param void No parameters.
 * @return bool true when the debounce window has elapsed.
 */
static bool button_debounce_passed(void) {
    uint64_t now = time_us_64();
    if (g_button_seen && (now - g_button_last_press_us) < BUTTON_DEBOUNCE_US) {
        return false;
    }
    g_button_last_press_us = now;
    g_button_seen = true;
    return true;
}

bool button_init(void) {
    gpio_init(COLD_CHAIN_MONITOR_BUTTON_PIN);
    gpio_set_dir(COLD_CHAIN_MONITOR_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(COLD_CHAIN_MONITOR_BUTTON_PIN);
    return true;
}

bool button_pressed(void) {
    return gpio_get(COLD_CHAIN_MONITOR_BUTTON_PIN) == 0;
}

bool button_consume_press(void) {
    if (!button_edge_pending()) {
        return false;
    }
    g_button_consumed = true;
    return button_debounce_passed();
}

void button_reset(void) {
    g_button_last_press_us = 0u;
    g_button_seen = false;
    g_button_consumed = false;
}
