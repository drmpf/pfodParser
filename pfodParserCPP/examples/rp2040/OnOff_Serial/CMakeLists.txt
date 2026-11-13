# (c)2014-2026 Forward Computing and Control Pty. Ltd.
# NSW Australia, www.forward.com.au
# This code is not warranted to be fit for any purpose. You may only use it at your own risk.
# This code may be freely used for both private and commercial use
# Provided this copyright is maintained.
#
# Pico SDK project for the OnOff_Serial example, RP2040 / RP2350.
# Builds the files in this directory plus pfodParserCPP's src/ and src/dwgs/.
# The Pico SDK location comes from PICO_SDK_PATH (cmake -D or environment variable).
# e.g. for the Pico
#   cmake -S . -B build -G Ninja -DPICO_SDK_PATH=/path/to/pico-sdk -DPICO_BOARD=pico
#   cmake --build build
# Use -DPICO_BOARD=pico2 for the Pico 2 (RP2350).
# Set PFOD_PARSER_CPP_DIR if pfodParserCPP is not three directories up from here.

cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED PICO_SDK_PATH)
  if(DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
  else()
    message(FATAL_ERROR "Set PICO_SDK_PATH to your pico-sdk directory")
  endif()
endif()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(OnOff_Serial C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

pico_sdk_init()

if(NOT DEFINED PFOD_PARSER_CPP_DIR)
  set(PFOD_PARSER_CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
endif()
set(PFOD_SRC ${PFOD_PARSER_CPP_DIR}/src)

file(GLOB app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB pfod_sources ${PFOD_SRC}/*.cpp ${PFOD_SRC}/*.c ${PFOD_SRC}/dwgs/*.cpp)

add_executable(OnOff_Serial ${app_sources} ${pfod_sources})
# this directory first, so the generated files' #include <pfodParser.h> finds the adapter here
target_include_directories(OnOff_Serial PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PFOD_SRC})
# warnings for this example and pfodParserCPP only, not for the Pico SDK sources
set_source_files_properties(${app_sources} ${pfod_sources} PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra")
target_link_libraries(OnOff_Serial pico_stdlib hardware_uart hardware_irq hardware_gpio hardware_timer)

# the pfod connection uses uart0 directly, so no Pico SDK stdio on the UART or USB
pico_enable_stdio_uart(OnOff_Serial 0)
pico_enable_stdio_usb(OnOff_Serial 0)

# .elf plus .bin, .hex, .map, .dis and (if picotool is available) .uf2
pico_add_extra_outputs(OnOff_Serial)
