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
// File:    monitor.c
// Desc:    Implements the classroom monitor state machine that ties the
//          DHT11 sensor, the 1602 LCD, and the RYLR998 radio together.
// Created: 2026

#include "cold_chain_monitor.h"
#include "monitor.h"
#include "sensor.h"
#include "display.h"
#include "radio.h"
#include "status_led.h"
#include "button.h"
#include "servo.h"
#include "ir_remote.h"
#include "crypto_aead.h"
#include "crypto_kdf.h"
#include "envelope.h"
#include "field_secrets.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "pico/time.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Module-ready flag.
 *
 * Set to true by monitor_init() once all peripherals are configured.
 * monitor_step() returns false while this flag is clear.
 */
static bool g_ready;

/**
 * @brief Initialized I2C peripheral handle for the LCD backpack.
 */
static i2c_inst_t *g_i2c;

/**
 * @brief Initialized I2C backpack address for the LCD.
 */
static uint8_t g_i2c_addr;

/**
 * @brief Monotonic transmit sequence number.
 */
static uint16_t g_seq;

/**
 * @brief Absolute time in microseconds of the next allowed transmit.
 */
static uint64_t g_next_tx_us;

/**
 * @brief First LCD render line buffer.
 */
static char g_line1[DISPLAY_LINE_LEN];

/**
 * @brief Second LCD render line buffer.
 */
static char g_line2[DISPLAY_LINE_LEN];

/**
 * @brief Inbound radio line accumulator.
 */
static char g_rx_line[RADIO_LINE_BUF_LEN];

/**
 * @brief Number of bytes currently held in the inbound line accumulator.
 */
static size_t g_rx_len;

/**
 * @brief Derived XChaCha20-Poly1305 session key for telemetry.
 */
static uint8_t g_key[CRYPTO_AEAD_KEY_LEN];

/**
 * @brief True once the telemetry session key has been derived.
 */
static bool g_key_ready;

/**
 * @brief Blink the Green LED a fixed number of times.
 *
 * @param times Number of on-off blink cycles to perform.
 * @return void
 */
static void blink_led(uint8_t times) {
    uint8_t i;
    for (i = 0u; i < times; ++i) {
        gpio_put(COLD_CHAIN_MONITOR_LED_PIN, 1);
        sleep_us(MONITOR_LED_BLINK_US);
        gpio_put(COLD_CHAIN_MONITOR_LED_PIN, 0);
        sleep_us(MONITOR_LED_BLINK_US);
    }
}

/**
 * @brief Probe one I2C address and report whether it acknowledges.
 *
 * @param i2c Pointer to the I2C peripheral to probe.
 * @param addr The 7-bit address to probe.
 * @return bool true when the address acknowledged.
 */
static bool i2c_probe(i2c_inst_t *i2c, uint8_t addr) {
    uint8_t dummy = 0u;
    if (i2c_write_blocking(i2c, addr, &dummy, 1u, false) < 0) {
        return false;
    }
    printf("  found 0x%02X\n", (unsigned)addr);
    return true;
}

/**
 * @brief Probe the I2C bus and print every device that acknowledges.
 *
 * @param i2c Pointer to the I2C peripheral to scan.
 * @return void
 */
static void i2c_bus_scan(i2c_inst_t *i2c) {
    uint8_t addr;
    uint8_t found = 0u;
    printf("I2C scan:\n");
    for (addr = 0x08u; addr < 0x78u; ++addr) {
        found += i2c_probe(i2c, addr) ? 1u : 0u;
    }
    if (found == 0u) {
        printf("  no devices\n");
    }
}

/**
 * @brief Initialize the I2C bus pins and scan the bus.
 *
 * @param void No parameters.
 * @return void
 */
static void monitor_bus_init(void) {
    i2c_init(COLD_CHAIN_MONITOR_I2C, COLD_CHAIN_MONITOR_I2C_BAUD);
    gpio_set_function(COLD_CHAIN_MONITOR_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(COLD_CHAIN_MONITOR_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(COLD_CHAIN_MONITOR_I2C_SDA);
    gpio_pull_up(COLD_CHAIN_MONITOR_I2C_SCL);
    i2c_bus_scan(COLD_CHAIN_MONITOR_I2C);
}

/**
 * @brief Initialize the LED, LCD handles, and transmit timing.
 *
 * @param void No parameters.
 * @return void
 */
static void monitor_state_init(void) {
    gpio_init(COLD_CHAIN_MONITOR_LED_PIN);
    gpio_set_dir(COLD_CHAIN_MONITOR_LED_PIN, GPIO_OUT);
    gpio_put(COLD_CHAIN_MONITOR_LED_PIN, 0);
    g_i2c = COLD_CHAIN_MONITOR_I2C;
    g_i2c_addr = COLD_CHAIN_MONITOR_LCD_ADDR;
    g_seq = 0u;
    g_next_tx_us = time_us_64() + (uint64_t)COLD_CHAIN_MONITOR_TX_INTERVAL_MS * 1000u;
    g_ready = true;
}

/**
 * @brief Initialize the human interface and actuator peripherals.
 *
 * @param void No parameters.
 * @return bool true when the LEDs, button, servo, and infrared eye ready.
 */
static bool monitor_peripherals_init(void) {
    return status_led_init() && button_init() && servo_init() &&
           ir_remote_init();
}

/**
 * @brief Derive the telemetry session key from the field secret.
 *
 * LAB-ONLY: production must provision the session key through OTP rather
 * than deriving it from a committed passphrase and salt.
 *
 * @param void No parameters.
 * @return bool true when the session key was derived.
 */
static bool monitor_derive_key(void) {
    bool ok = crypto_kdf_argon2id((const uint8_t *)FIELD_SECRET_PASSPHRASE, strlen(FIELD_SECRET_PASSPHRASE), FIELD_SECRET_SALT, 16u, g_key);
    g_key_ready = ok;
    return ok;
}

bool monitor_init(void) {
    monitor_bus_init();
    if (!monitor_peripherals_init() || !sensor_init() ||
        !radio_init(COLD_CHAIN_MONITOR_UART) ||
        !display_init(COLD_CHAIN_MONITOR_I2C, COLD_CHAIN_MONITOR_LCD_ADDR)) {
        printf("INIT FAIL\n");
        return false;
    }
    monitor_state_init();
    return monitor_derive_key();
}

void monitor_deinit(void) {
    g_ready = false;
}

/**
 * @brief Drive the annunciator LEDs and damper servo from a reading.
 *
 * @param reading Pointer to the decoded DHT11 reading.
 * @return void
 */
static void monitor_apply_actuators(const dht_reading_t *reading) {
    status_led_state_t state = status_led_state_for_temperature(reading->temperature_tenths, reading->valid);
    status_led_show(state);
    if (state == STATUS_LED_BREACH) {
        servo_vent();
    } else {
        servo_seal();
    }
}

/**
 * @brief Seal the reading telemetry body into a hex envelope.
 *
 * @param reading Pointer to the decoded DHT11 reading.
 * @param hex Pointer to the NUL-terminated hex envelope output buffer.
 * @param hex_len Capacity of the hex envelope buffer in bytes.
 * @return bool true when the reading was sealed and encoded.
 */
static bool monitor_seal_frame(const dht_reading_t *reading, char *hex, size_t hex_len) {
    char frame[COLD_CHAIN_MONITOR_FRAME_SIZE];
    uint8_t nonce[ENVELOPE_NONCE_LEN];
    uint8_t ad = (uint8_t)PACKET_NODE_ID;
    sensor_build_frame(reading, g_seq, frame, sizeof(frame));
    envelope_fill_nonce(nonce);
    return envelope_seal_hex(g_key, nonce, &ad, 1u, (const uint8_t *)frame, strlen(frame), hex, hex_len);
}

/**
 * @brief Build and transmit the authenticated telemetry frame.
 *
 * @param reading Pointer to the decoded DHT11 reading.
 * @return void
 */
static void monitor_transmit(const dht_reading_t *reading) {
    char hex[ENVELOPE_MAX_HEX_LEN];
    if (!g_key_ready) {
        return;
    }
    if (monitor_seal_frame(reading, hex, sizeof(hex))) {
        radio_send_frame(COLD_CHAIN_MONITOR_UART, (const uint8_t *)hex, strlen(hex));
    }
}

/**
 * @brief Render and transmit a successful reading.
 *
 * @param reading Pointer to the decoded DHT11 reading.
 * @return void
 */
static void monitor_emit(const dht_reading_t *reading) {
    display_format_lines(reading, g_seq, true, g_line1, g_line2);
    monitor_apply_actuators(reading);
    blink_led(MONITOR_LED_BLINKS);
    monitor_transmit(reading);
    g_seq += 1u;
}

/**
 * @brief Render the failed-read LCD lines.
 *
 * @param void No parameters.
 * @return void
 */
static void monitor_emit_fail(void) {
    dht_reading_t reading;
    reading.temperature_tenths = 0;
    reading.humidity_tenths = 0;
    reading.valid = false;
    display_format_lines(&reading, g_seq, false, g_line1, g_line2);
    status_led_show(STATUS_LED_WARNING);
}

/**
 * @brief Sample the sensor and render or transmit one LCD frame.
 *
 * @param now_us Current monotonic time in microseconds.
 * @return void
 */
static void monitor_tx_tick(uint64_t now_us) {
    dht_reading_t reading;
    if (sensor_read(&reading) == SENSOR_RESULT_OK) {
        monitor_emit(&reading);
    } else {
        monitor_emit_fail();
    }
    display_render_lines(g_i2c, g_i2c_addr, g_line1, g_line2);
    g_next_tx_us = now_us + (uint64_t)COLD_CHAIN_MONITOR_TX_INTERVAL_MS * 1000u;
}

/**
 * @brief Verify one inbound hex envelope with the session key.
 *
 * @param hex Pointer to the NUL-terminated inbound hex envelope.
 * @param body Pointer to the recovered plaintext output buffer.
 * @param body_cap Capacity of the plaintext output buffer in bytes.
 * @param body_len Pointer to store the recovered plaintext length.
 * @return bool true when the inbound envelope authenticated.
 */
static bool monitor_authenticate(const char *hex, uint8_t *body,
                                 size_t body_cap, size_t *body_len) {
    uint8_t ad = (uint8_t)PACKET_NODE_ID;
    return envelope_open_hex(g_key, &ad, 1u, hex, body, body_cap, body_len);
}

/**
 * @brief Authenticate one inbound +RCV report before acting on it.
 *
 * @param rcv Pointer to the decoded inbound radio report.
 * @return void
 */
static void monitor_handle_rcv(const radio_rcv_t *rcv) {
    uint8_t body[ENVELOPE_MAX_PLAINTEXT];
    size_t body_len = 0u;
    if (!g_key_ready ||
        !monitor_authenticate(rcv->payload, body, sizeof(body), &body_len)) {
        printf("UNAUTHENTICATED\n");
        return;
    }
    blink_led(1u);
}

/**
 * @brief Drain inbound radio lines and authenticate each +RCV report.
 *
 * @param void No parameters.
 * @return void
 */
static void monitor_rx_tick(void) {
    radio_rcv_t rcv;
    while (radio_line_pump(COLD_CHAIN_MONITOR_UART, g_rx_line, &g_rx_len)) {
        if (radio_parse_rcv(g_rx_line, &rcv) == RADIO_RESULT_OK) {
            monitor_handle_rcv(&rcv);
        }
    }
}

/**
 * @brief Consume one debounced acknowledge press and re-seal the damper.
 *
 * @param void No parameters.
 * @return void
 */
static void monitor_handle_button(void) {
    if (button_consume_press()) {
        servo_seal();
        printf("ACK\n");
    }
}

/**
 * @brief Apply one decoded maintenance remote command to the actuator.
 *
 * @param cmd Pointer to the decoded infrared command.
 * @return void
 */
static void monitor_ir_command(const ir_command_t *cmd) {
    printf("IR 0x%02X\n", (unsigned)cmd->command);
    if (cmd->address != MONITOR_IR_REMOTE_ADDRESS) {
        return;
    }
    if (cmd->command == MONITOR_IR_VENT_COMMAND) {
        servo_vent();
    }
    if (cmd->command == MONITOR_IR_SEAL_COMMAND) {
        servo_seal();
    }
}

/**
 * @brief Poll the infrared eye for a maintenance remote command.
 *
 * @param void No parameters.
 * @return void
 */
static void monitor_handle_ir(void) {
    ir_command_t cmd;
    if (ir_remote_poll(&cmd)) {
        monitor_ir_command(&cmd);
    }
}

/**
 * @brief Service the radio, the acknowledge button, and the infrared eye.
 *
 * @param void No parameters.
 * @return void
 */
static void monitor_service_inputs(void) {
    monitor_rx_tick();
    monitor_handle_button();
    monitor_handle_ir();
}

bool monitor_step(void) {
    uint64_t now_us;
    if (!g_ready) {
        return false;
    }
    now_us = time_us_64();
    if (now_us >= g_next_tx_us) {
        monitor_tx_tick(now_us);
    }
    monitor_service_inputs();
    return true;
}