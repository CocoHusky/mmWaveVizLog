# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mr60bha2_console_zephyr VERSION 0.1.0)

target_include_directories(app PRIVATE
  src
  drivers/mr60bha2
)

target_sources(app PRIVATE
  src/main.c
  src/app_state.c
  src/json_stream.c
  drivers/mr60bha2/mr60bha2.c
  drivers/mr60bha2/mr60bha2_parser.c
)
