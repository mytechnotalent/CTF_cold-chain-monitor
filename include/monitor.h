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
// File:    monitor.h
// Desc:    Declares the classroom monitor state machine tying sensor,
//          display, radio, and the Green LED together.
// Created: 2026

#ifndef MONITOR_H
#define MONITOR_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Number of rapid Green LED blinks on one successful transmit.
 */
#define MONITOR_LED_BLINKS 3u

/**
 * @brief Green LED blink duty in microseconds.
 */
#define MONITOR_LED_BLINK_US 50000u

/**
 * @brief Authorized maintenance remote address accepted by the node.
 *
 * The infrared command handler rejects any decoded NEC frame whose address
 * does not match this value before it can move the damper.
 */
#define MONITOR_IR_REMOTE_ADDRESS 0x1Du

/**
 * @brief Maintenance remote command that vents the cold-store damper.
 *
 * Security note: the infrared port is unauthenticated, so any NEC remote
 * can issue this command. Closing that door is the infrared replay lab.
 */
#define MONITOR_IR_VENT_COMMAND 0x47u

/**
 * @brief Maintenance remote command that seals the cold-store damper.
 */
#define MONITOR_IR_SEAL_COMMAND 0x45u

/**
 * @brief Initialize the classroom monitor state machine.
 *
 * Configures the sensor GPIO, the RYLR998 UART, and the 1602 LCD and
 * resets the sequence counter.
 *
 * @param void No parameters.
 * @return bool true when all submodules initialized.
 */
bool monitor_init(void);

/**
 * @brief Clear the monitor-ready flag.
 *
 * Test and recovery hook that returns the state machine to the
 * uninitialized policy state.
 *
 * @param void No parameters.
 * @return void
 */
void monitor_deinit(void);

/**
 * @brief Execute one monitor state-machine tick.
 *
 * Samples the DHT11, formats and transmits the telemetry frame on the
 * interval, blinks the Green LED on success, renders the display lines,
 * and pumps inbound +RCV lines.
 *
 * @param void No parameters.
 * @return bool true when the tick completed without a policy error.
 */
bool monitor_step(void);

#endif // MONITOR_H