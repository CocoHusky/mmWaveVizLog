# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mmwavevizlog_runtime VERSION 0.1.0 LANGUAGES C CXX)

set_source_files_properties(src/net_services.c PROPERTIES LANGUAGE CXX)

target_include_directories(app PRIVATE
  src
  drivers/mr60bha2
)

target_sources(app PRIVATE
  src/main.c
  src/app_state.c
  src/ambient_light.c
  src/led_control.c
  src/ota_control.c
  src/json_stream.c
  src/net_services.c
  drivers/mr60bha2/mr60bha2.c
  drivers/mr60bha2/mr60bha2_parser.c
)
