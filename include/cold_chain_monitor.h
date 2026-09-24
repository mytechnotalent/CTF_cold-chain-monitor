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
// File:    cold_chain_monitor.h
// Desc:    Declares platform pin mapping, peripheral handles, and playback
//          boundaries for the Cold Chain Agricultural Environmental Monitor.
// Created: 2026

#ifndef COLD_CHAIN_MONITOR_H
#define COLD_CHAIN_MONITOR_H

#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "packet_artifact.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Onboard green LED GPIO pin number.
 *
 * The RP2350 Pico 2 onboard LED is connected to GPIO 25. It is blinked
 * rapidly whenever the edge device completes a successful LoRa transmit.
 */
#define COLD_CHAIN_MONITOR_LED_PIN 25u

/**
 * @brief DHT11 one-wire data GPIO pin number.
 *
 * The DHT11 single data line is driven low for the host-start pulse and
 * then read for the 40-bit response frame.
 */
#define COLD_CHAIN_MONITOR_DHT_PIN 4u

/**
 * @brief I2C peripheral used by the 1602 LCD backpack.
 *
 * The Pico 2 bus B (I2C1) drives the PCF8574 backpack on the display.
 */
#define COLD_CHAIN_MONITOR_I2C i2c1

/**
 * @brief I2C SDA GPIO pin number.
 */
#define COLD_CHAIN_MONITOR_I2C_SDA 2u

/**
 * @brief I2C SCL GPIO pin number.
 */
#define COLD_CHAIN_MONITOR_I2C_SCL 3u

/**
 * @brief I2C bus clock rate in hertz.
 */
#define COLD_CHAIN_MONITOR_I2C_BAUD 100000u

/**
 * @brief I2C address of the 1602 LCD PCF8574 backpack.
 *
 * The most common PCF8574 backpack address is 0x27 with solder-bridge
 * address pins left unbridged. Provisioned through the packet artifact.
 */
#define COLD_CHAIN_MONITOR_LCD_ADDR PACKET_LCD_I2C_ADDRESS

/**
 * @brief UART peripheral used by the RYLR998 transceiver.
 *
 * The RYLR998 connects to UART1 through the Pico 2 header. All AT
 * command traffic flows over this byte stream.
 */
#define COLD_CHAIN_MONITOR_UART uart1

/**
 * @brief UART TX GPIO pin number to the RYLR998 RX input.
 */
#define COLD_CHAIN_MONITOR_UART_TX 8u

/**
 * @brief UART RX GPIO pin number from the RYLR998 TX output.
 */
#define COLD_CHAIN_MONITOR_UART_RX 9u

/**
 * @brief UART baud rate negotiated with the RYLR998.
 *
 * The RYLR998 ships with a 115200 baud default and must be matched on
 * both the edge device and the instructor gateway.
 */
#define COLD_CHAIN_MONITOR_UART_BAUD 115200u

/**
 * @brief RYLR998 network identifier shared by all classroom radios.
 *
 * Every edge device and the instructor hub must program the same network
 * identifier or no frames are delivered over the air.
 */
#define COLD_CHAIN_MONITOR_NETWORK_ID 18u

/**
 * @brief Fixed telemetry frame size in bytes.
 *
 * The JSON payload produced by the edge device is padded with trailing
 * NUL bytes to this fixed size before transmission over LoRa.
 */
#define COLD_CHAIN_MONITOR_FRAME_SIZE PACKET_FRAME_SIZE

/**
 * @brief Minimum spacing between consecutive LoRa transmissions.
 *
 * Deliberately larger than the radio on-air time so the instructor hub
 * sees one frame per edge device per slot.
 */
#define COLD_CHAIN_MONITOR_TX_INTERVAL_MS PACKET_TX_INTERVAL_MS

/**
 * @brief Red breach LED GPIO pin number.
 */
#define COLD_CHAIN_MONITOR_RED_LED_PIN 16u

/**
 * @brief Yellow warning LED GPIO pin number.
 */
#define COLD_CHAIN_MONITOR_YELLOW_LED_PIN 17u

/**
 * @brief Green nominal LED GPIO pin number.
 */
#define COLD_CHAIN_MONITOR_GREEN_LED_PIN 18u

/**
 * @brief Operator acknowledge push-button GPIO pin number.
 */
#define COLD_CHAIN_MONITOR_BUTTON_PIN 15u

/**
 * @brief Cold-store damper servo PWM GPIO pin number.
 */
#define COLD_CHAIN_MONITOR_SERVO_PIN 14u

/**
 * @brief Infrared receiver GPIO pin number.
 */
#define COLD_CHAIN_MONITOR_IR_PIN 5u

/**
 * @brief Safe cold-chain temperature ceiling in tenths of a degree Celsius.
 */
#define COLD_CHAIN_MONITOR_TEMP_WARN_TENTHS (-50)

/**
 * @brief Breach cold-chain temperature ceiling in tenths of a degree Celsius.
 */
#define COLD_CHAIN_MONITOR_TEMP_BREACH_TENTHS 0

#endif // COLD_CHAIN_MONITOR_H