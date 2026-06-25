# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mr60bha2_parser_test)

target_include_directories(app PRIVATE
  ../../drivers/mr60bha2
)

target_compile_definitions(app PRIVATE
  TEST_DATA_DIR="${CMAKE_CURRENT_LIST_DIR}/test_data"
)

target_sources(app PRIVATE
  src/main.c
  ../../drivers/mr60bha2/mr60bha2.c
  ../../drivers/mr60bha2/mr60bha2_parser.c
)
