# SPDX-License-Identifier: Apache-2.0
#
# Zephyr RTOS: LED + Grid-EYE + BLE Demo
# Demonstrates concurrent threads, sensor subsystem, and cross-board portability.

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(led_sensor_demo)

target_sources(app PRIVATE src/main.c)
