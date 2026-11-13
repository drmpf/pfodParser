# (c)2014-2026 Forward Computing and Control Pty. Ltd.
# NSW Australia, www.forward.com.au
# This code is not warranted to be fit for any purpose. You may only use it at your own risk.
# This code may be freely used for both private and commercial use
# Provided this copyright is maintained.
#
# nRF Connect SDK Bare Metal application for the OnOff_Serial example.
# Builds the files in this directory plus pfodParserCPP's src/ and src/dwgs/.
# Set PFOD_PARSER_CPP_DIR if pfodParserCPP is not three directories up from here, e.g.
#   west build -b bm_nrf54l15dk/nrf54l15/cpuapp/s115_softdevice -- -DPFOD_PARSER_CPP_DIR=/path/to/pfodParserCPP

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pfod_onoff_serial)

if(NOT DEFINED PFOD_PARSER_CPP_DIR)
  set(PFOD_PARSER_CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
endif()
set(PFOD_SRC ${PFOD_PARSER_CPP_DIR}/src)

file(GLOB app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB pfod_sources ${PFOD_SRC}/*.cpp ${PFOD_SRC}/*.c ${PFOD_SRC}/dwgs/*.cpp)

target_sources(app PRIVATE ${app_sources} ${pfod_sources})
# this directory first, so the generated files' #include <pfodParser.h> finds the adapter here
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PFOD_SRC})
