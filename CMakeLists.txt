# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# MIT License
#
# Copyright (c) 2026 Kevin Thomas
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Author:  Kevin Thomas
# Email:   kevin@mytechnotalent.com
# GitHub:  https://github.com/mytechnotalent
# File:    CMakeLists.txt
# Desc:    Configures the OPERATION COLD IRON CTF RP2350 Pico SDK project
#          (ACT-I) and its module targets.
# Created: 2026

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
	set(USERHOME $ENV{USERPROFILE})
else()
	set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if(EXISTS ${picoVscode})
	include(${picoVscode})
endif()

set(PICO_BOARD pico2 CACHE STRING "Board type")

include(pico_sdk_import.cmake)
project(ACT-I C CXX ASM)

pico_sdk_init()

add_library(crc STATIC src/crc.c)
target_include_directories(crc PUBLIC include)

add_library(sensor STATIC src/sensor.c)
target_include_directories(sensor PUBLIC include)
target_link_libraries(sensor PUBLIC pico_stdlib crc hardware_i2c)

add_library(display STATIC src/display.c)
target_include_directories(display PUBLIC include)
target_link_libraries(display PUBLIC pico_stdlib hardware_i2c)

add_library(radio STATIC src/radio.c)
target_include_directories(radio PUBLIC include)
target_link_libraries(radio PUBLIC pico_stdlib hardware_i2c)

add_library(status_led STATIC src/status_led.c)
target_include_directories(status_led PUBLIC include)
target_link_libraries(status_led PUBLIC pico_stdlib hardware_i2c hardware_uart)

add_library(button STATIC src/button.c)
target_include_directories(button PUBLIC include)
target_link_libraries(button PUBLIC pico_stdlib hardware_i2c hardware_uart)

add_library(servo STATIC src/servo.c)
target_include_directories(servo PUBLIC include)
target_link_libraries(servo PUBLIC pico_stdlib hardware_pwm hardware_i2c hardware_uart)

add_library(ir_remote STATIC src/ir_remote.c)
target_include_directories(ir_remote PUBLIC include)
target_link_libraries(ir_remote PUBLIC pico_stdlib hardware_i2c hardware_uart)

add_library(crypto STATIC
	src/chacha20.c
	src/poly1305.c
	src/crypto_aead.c
	src/blake2b.c
	src/argon2.c
	src/crypto_kdf.c
	src/envelope.c
)
target_include_directories(crypto PUBLIC include)
target_link_libraries(crypto PUBLIC pico_stdlib pico_rand)

add_library(monitor STATIC src/monitor.c)
target_include_directories(monitor PUBLIC include)
target_link_libraries(monitor PUBLIC pico_stdlib sensor display radio status_led button servo ir_remote crypto hardware_i2c hardware_pwm)

add_executable(ACT-I src/main.c)
target_link_libraries(ACT-I PRIVATE monitor pico_stdlib)
pico_enable_stdio_uart(ACT-I 1)
pico_enable_stdio_usb(ACT-I 1)
pico_add_extra_outputs(ACT-I)
