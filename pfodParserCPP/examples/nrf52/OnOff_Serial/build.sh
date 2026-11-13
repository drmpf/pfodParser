#!/bin/bash
# (c)2014-2026 Forward Computing and Control Pty. Ltd.
# NSW Australia, www.forward.com.au
# This code is not warranted to be fit for any purpose. You may only use it at your own risk.
# This code may be freely used for both private and commercial use
# Provided this copyright is maintained.
#
# build.sh -- builds this OnOff_Serial example for the nRF52 DK (nRF52832, PCA10040)
# with the nRF5 SDK 17.1.0 and arm-none-eabi-gcc, without SEGGER Embedded Studio.
# It uses the sdk_config.h, SDK source list, compiler flags and linker script of the
# SDK's own examples/peripheral/uart/pca10040/blank example, which uses app_uart_fifo
# as target_Stream does.
#
# This checks that the example compiles and links against the real SDK. main.cpp still
# has its TODO (the clock / log / power init your SDK example template uses).
#
# Run from Git Bash (Windows), or bash on Linux / macOS:
#   bash build.sh               build
#   bash build.sh --delete-sdk  build, then delete the SDK build.sh downloaded
# Output, in build/ in this directory: OnOff_Serial.elf / .map / .hex
# Flash the .hex with nrfjprog or nRF Connect Programmer, e.g.
#   nrfjprog --program build/OnOff_Serial.hex --chiperase --verify --reset
#
# The SDK comes from (first found):
#   1. NRF5_SDK, the path of an unzipped nRF5_SDK_17.1.0_ddde560 directory
#   2. otherwise Nordic's nRF5_SDK_17.1.0_ddde560.zip (about 130MB, 575MB unzipped)
#      downloaded and unzipped into build/ (needs curl and unzip). It is kept for the
#      next build unless --delete-sdk is given.
# The toolchain comes from (first found):
#   1. NRF52_GCC_BIN, the bin directory of an arm-none-eabi-gcc toolchain
#   2. arm-none-eabi-gcc on the PATH
#   3. the highest packages/*/tools/arm-none-eabi-gcc/*/bin in Arduino15 (e.g. installed
#      with an Arduino SAMD / Adafruit / Seeeduino core); ARDUINO15 overrides its location
# PFOD_PARSER_CPP overrides the pfodParserCPP directory, default three directories up from here.
#
# For another board / nRF52 part: change BOARD, the MCU defines and the SDK example
# (PRJ, which supplies sdk_config.h and the linker script) below, e.g. BOARD_PCA10056 /
# NRF52840_XXAA and examples/peripheral/uart/pca10056/blank for the nRF52840 DK.

set -u

SDK_ZIP_URL=https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/sdks/nrf5/binaries/nrf5_sdk_17.1.0_ddde560.zip
SDK_DIR_NAME=nRF5_SDK_17.1.0_ddde560
BOARD=BOARD_PCA10040
MCU_DEFS="-DNRF52 -DNRF52832_XXAA -DNRF52_PAN_74"
PRJ_REL=examples/peripheral/uart/pca10040/blank   # supplies config/sdk_config.h and armgcc/*.ld
STARTUP=modules/nrfx/mdk/gcc_startup_nrf52.S
SYSTEM=modules/nrfx/mdk/system_nrf52.c
MCU="-mcpu=cortex-m4 -mthumb -mabi=aapcs -mfloat-abi=hard -mfpu=fpv4-sp-d16"

EX="$(cd "$(dirname "$0")" && pwd)"
ROOT="${PFOD_PARSER_CPP:-$EX/../../..}"
OUT="$EX/build"
DELETE_SDK=0
[ "${1:-}" = "--delete-sdk" ] && DELETE_SDK=1

# toPath -- convert a path for the native tools (C:/... form under Git Bash on Windows)
toPath() {
  if command -v cygpath >/dev/null 2>&1; then cygpath -m "$1"; else echo "$1"; fi
}

# fail -- print an error and stop
fail() {
  echo "ERROR: $*" >&2
  exit 1
}

# ---- toolchain ----
if [ -n "${NRF52_GCC_BIN:-}" ]; then
  TC="$NRF52_GCC_BIN/arm-none-eabi-"
elif command -v arm-none-eabi-gcc >/dev/null 2>&1; then
  TC="arm-none-eabi-"
else
  if [ -z "${ARDUINO15:-}" ]; then
    if [ -n "${LOCALAPPDATA:-}" ]; then ARDUINO15="$(toPath "$LOCALAPPDATA")/Arduino15"
    elif [ "$(uname)" = "Darwin" ]; then ARDUINO15="$HOME/Library/Arduino15"
    else ARDUINO15="$HOME/.arduino15"; fi
  fi
  TCDIR="$(ls -d "$ARDUINO15"/packages/*/tools/arm-none-eabi-gcc/*/bin 2>/dev/null | sort -V | tail -1)"
  [ -n "$TCDIR" ] || fail "no arm-none-eabi-gcc found, set NRF52_GCC_BIN"
  TC="$TCDIR/arm-none-eabi-"
fi
"${TC}gcc" --version >/dev/null 2>&1 || fail "cannot run ${TC}gcc"
echo "Toolchain: $("${TC}gcc" --version | head -1)"

# ---- SDK ----
SDK_DOWNLOAD="$OUT/nrf5sdk"
if [ -n "${NRF5_SDK:-}" ]; then
  SDK="$NRF5_SDK"
  DELETE_SDK=0 # never delete an SDK build.sh did not download
else
  SDK="$SDK_DOWNLOAD/$SDK_DIR_NAME"
  if [ ! -d "$SDK/components" ]; then
    echo "Downloading the nRF5 SDK 17.1.0 into $SDK_DOWNLOAD"
    mkdir -p "$SDK_DOWNLOAD"
    curl -fL -sS -o "$SDK_DOWNLOAD/sdk.zip" "$SDK_ZIP_URL" || fail "download of the nRF5 SDK failed"
    (cd "$SDK_DOWNLOAD" && unzip -q -o sdk.zip) || fail "unzip of the nRF5 SDK failed"
    rm -f "$SDK_DOWNLOAD/sdk.zip"
  fi
fi
[ -d "$SDK/components" ] && [ -d "$SDK/$PRJ_REL" ] || fail "$SDK is not an nRF5 SDK 17.1.0 directory"
echo "SDK: $SDK"
PRJ="$SDK/$PRJ_REL"

# ---- build ----
rm -rf "$OUT/obj" "$OUT/sdk" "$OUT"/OnOff_Serial.*
mkdir -p "$OUT/obj" "$OUT/sdk"
: > "$OUT/warnings.txt"

P() { toPath "$1"; }
DEFS="-D$BOARD -DBSP_DEFINES_ONLY -DCONFIG_GPIO_AS_PINRESET -DFLOAT_ABI_HARD $MCU_DEFS -D__HEAP_SIZE=8192 -D__STACK_SIZE=8192"
INC="-I$(P "$EX") -I$(P "$ROOT/src") -I$(P "$PRJ/config")"
# the include folders of the SDK uart example Makefile
for d in components modules/nrfx/mdk components/libraries/fifo components/libraries/strerror \
  components/toolchain/cmsis/include components/libraries/util components/libraries/balloc \
  components/libraries/ringbuf modules/nrfx/hal components/libraries/bsp components/libraries/uart \
  components/libraries/log modules/nrfx components/libraries/experimental_section_vars \
  integration/nrfx/legacy components/libraries/delay integration/nrfx components/drivers_nrf/nrf_soc_nosd \
  components/libraries/atomic components/boards components/libraries/memobj modules/nrfx/drivers/include \
  external/fprintf components/libraries/log/src; do
  INC="$INC -I$(P "$SDK/$d")"
done
COMMON="$MCU $DEFS $INC -Os -g -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin -fshort-enums"

errors=0
# the SDK sources of the uart example Makefile, less its main.c and retarget.c -- Nordic's
# code, its warnings are not shown
for f in components/libraries/log/src/nrf_log_frontend.c components/libraries/log/src/nrf_log_str_formatter.c \
  components/boards/boards.c components/libraries/util/app_error.c components/libraries/util/app_error_handler_gcc.c \
  components/libraries/util/app_error_weak.c components/libraries/fifo/app_fifo.c \
  components/libraries/uart/app_uart_fifo.c components/libraries/util/app_util_platform.c \
  components/libraries/util/nrf_assert.c components/libraries/atomic/nrf_atomic.c \
  components/libraries/balloc/nrf_balloc.c external/fprintf/nrf_fprintf.c external/fprintf/nrf_fprintf_format.c \
  components/libraries/memobj/nrf_memobj.c components/libraries/ringbuf/nrf_ringbuf.c \
  components/libraries/strerror/nrf_strerror.c integration/nrfx/legacy/nrf_drv_uart.c \
  modules/nrfx/soc/nrfx_atomic.c modules/nrfx/drivers/src/prs/nrfx_prs.c modules/nrfx/drivers/src/nrfx_uart.c \
  modules/nrfx/drivers/src/nrfx_uarte.c "$SYSTEM"; do
  "${TC}gcc" -c -std=c99 $COMMON -w "$(P "$SDK/$f")" -o "$(P "$OUT/sdk/$(basename "${f%.*}").o")" || { echo "compile failed: $f"; errors=1; }
done
"${TC}gcc" -c -x assembler-with-cpp $MCU $DEFS "$(P "$SDK/$STARTUP")" -o "$(P "$OUT/sdk/startup.o")" || { echo "assemble failed: $STARTUP"; errors=1; }
# this example and pfodParserCPP, with warnings
for f in "$EX"/*.cpp "$ROOT"/src/*.cpp "$ROOT"/src/dwgs/*.cpp; do
  "${TC}g++" -c -std=gnu++11 $COMMON -Wall -Wextra -fno-exceptions -fno-rtti -fno-threadsafe-statics \
    "$(P "$f")" -o "$(P "$OUT/obj/$(basename "${f%.*}").o")" 2>>"$OUT/warnings.txt" || { echo "compile failed: $f"; errors=1; }
done
for f in "$ROOT"/src/*.c; do
  "${TC}gcc" -c -std=gnu11 $COMMON -Wall -Wextra "$(P "$f")" -o "$(P "$OUT/obj/$(basename "${f%.*}").o")" 2>>"$OUT/warnings.txt" || { echo "compile failed: $f"; errors=1; }
done
[ -s "$OUT/warnings.txt" ] && cat "$OUT/warnings.txt"
[ $errors = 0 ] || fail "compile failed"

LDSCRIPT="$(ls "$PRJ"/armgcc/*.ld | head -1)"
"${TC}g++" $MCU "-L$(P "$SDK/modules/nrfx/mdk")" "-T$(P "$LDSCRIPT")" -Wl,--gc-sections --specs=nano.specs \
  "-Wl,-Map=$(P "$OUT/OnOff_Serial.map")" "$(P "$OUT")"/sdk/*.o "$(P "$OUT")"/obj/*.o -lc -lnosys -lm \
  -o "$(P "$OUT/OnOff_Serial.elf")" || fail "link failed"
"${TC}size" "$(P "$OUT/OnOff_Serial.elf")"
"${TC}objcopy" -O ihex "$(P "$OUT/OnOff_Serial.elf")" "$(P "$OUT/OnOff_Serial.hex")"
echo "Built $OUT/OnOff_Serial.elf / .hex"

if [ $DELETE_SDK = 1 ]; then
  rm -rf "$SDK_DOWNLOAD"
  echo "Deleted the downloaded SDK $SDK_DOWNLOAD"
fi
