#!/bin/bash
# (c)2014-2026 Forward Computing and Control Pty. Ltd.
# NSW Australia, www.forward.com.au
# This code is not warranted to be fit for any purpose. You may only use it at your own risk.
# This code may be freely used for both private and commercial use
# Provided this copyright is maintained.
#
# build.sh -- builds this OnOff_Serial example for the NXP FRDM-K64F (MK64FN1M0VLL12) with
# the MCUXpresso SDK 2.x and arm-none-eabi-gcc, without MCUXpresso IDE, using the SDK's
# own device files (headers, system_MK64F12.c, gcc startup and linker script), drivers,
# FRDM-K64F board files (clock_config.c, board.h) and the compiler / linker flags of its
# frdmk64f/driver_examples/uart/interrupt example, whose pin_mux.c (UART0 on PTB16 / PTB17,
# the OpenSDA virtual COM port) is used.
#
# The FRDM-K64F is not in the current MCUXpresso SDK (mcuxsdk); the last SDK supporting it
# is the 2.x SDK in github.com/nxp-mcuxpresso/legacy-mcux-sdk (and legacy-mcux-sdk-examples).
#
# Run from Git Bash (Windows), or bash on Linux / macOS:
#   bash build.sh               build
#   bash build.sh --delete-sdk  build, then delete anything build.sh downloaded
# Output, in build/ in this directory: OnOff_Serial.elf / .map / .hex / .bin
# Flash by copying OnOff_Serial.bin to the board's OpenSDA (DAPLink) drive, or with
# pyOCD / LinkServer.
#
# The SDK comes from (first found):
#   1. NXP_SDK, the root of a legacy-mcux-sdk (2.x) clone, or an SDK 2.x package for the
#      FRDM-K64F laid out the same way (devices/, drivers/, boards/ at the top), with
#      NXP_PIN_MUX the directory holding your pin_mux.c / pin_mux.h (default: downloaded
#      from the SDK's uart interrupt example, as below)
#   2. otherwise a sparse git clone (about 6MB) of just the needed parts of
#      legacy-mcux-sdk into build/download (needs git), plus the uart interrupt example's
#      pin_mux.c / pin_mux.h (needs curl)
# The CMSIS core headers come from the highest packages/*/tools/CMSIS/*/CMSIS/Include in
# Arduino15, otherwise they are downloaded from github.com/ARM-software/CMSIS_5.
# The toolchain comes from (first found):
#   1. NXP_GCC_BIN, the bin directory of an arm-none-eabi-gcc toolchain
#   2. arm-none-eabi-gcc on the PATH
#   3. the highest packages/*/tools/arm-none-eabi-gcc/*/bin in Arduino15; ARDUINO15 overrides
#      its location
# PFOD_PARSER_CPP overrides the pfodParserCPP directory, default three directories up from here.

set -u

SDK_URL=https://github.com/nxp-mcuxpresso/legacy-mcux-sdk.git
PIN_MUX_URL=https://raw.githubusercontent.com/nxp-mcuxpresso/legacy-mcux-sdk-examples/main/frdmk64f/driver_examples/uart/interrupt
CMSIS_URL=https://raw.githubusercontent.com/ARM-software/CMSIS_5/develop/CMSIS/Core/Include
DEVICE=MK64F12
BOARD=frdmk64f
LDSCRIPT_NAME=MK64FN1M0xxx12_flash.ld
DEFS="-DNDEBUG -DCPU_MK64FN1M0VLL12 -DFRDM_K64F -DFREEDOM -DMCUXPRESSO_SDK"
MCU="-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16"

EX="$(cd "$(dirname "$0")" && pwd)"
ROOT="${PFOD_PARSER_CPP:-$EX/../../..}"
OUT="$EX/build"
DL="$OUT/download"
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

if [ -z "${ARDUINO15:-}" ]; then
  if [ -n "${LOCALAPPDATA:-}" ]; then ARDUINO15="$(toPath "$LOCALAPPDATA")/Arduino15"
  elif [ "$(uname)" = "Darwin" ]; then ARDUINO15="$HOME/Library/Arduino15"
  else ARDUINO15="$HOME/.arduino15"; fi
fi

# ---- toolchain ----
if [ -n "${NXP_GCC_BIN:-}" ]; then
  TC="$NXP_GCC_BIN/arm-none-eabi-"
elif command -v arm-none-eabi-gcc >/dev/null 2>&1; then
  TC="arm-none-eabi-"
else
  TCDIR="$(ls -d "$ARDUINO15"/packages/*/tools/arm-none-eabi-gcc/*/bin 2>/dev/null | sort -V | tail -1)"
  [ -n "$TCDIR" ] || fail "no arm-none-eabi-gcc found, set NXP_GCC_BIN"
  TC="$TCDIR/arm-none-eabi-"
fi
"${TC}gcc" --version >/dev/null 2>&1 || fail "cannot run ${TC}gcc"
echo "Toolchain: $("${TC}gcc" --version | head -1)"

# ---- SDK and pin_mux ----
if [ -n "${NXP_SDK:-}" ]; then
  SDK="$NXP_SDK"
else
  SDK="$DL/sdk"
  if [ ! -f "$SDK/.pfod_ok" ]; then
    echo "Downloading the needed parts of the MCUXpresso SDK 2.x (legacy-mcux-sdk) into $SDK"
    rm -rf "$SDK"
    mkdir -p "$DL"
    git clone -q --depth 1 --filter=blob:none --sparse "$SDK_URL" "$SDK" || fail "git clone of legacy-mcux-sdk failed"
    git -C "$SDK" sparse-checkout set "devices/$DEVICE" drivers/common drivers/uart drivers/gpio \
      drivers/port drivers/smc "boards/$BOARD" || fail "git sparse-checkout failed"
    touch "$SDK/.pfod_ok"
  fi
fi
[ -f "$SDK/devices/$DEVICE/$DEVICE.h" ] || fail "$SDK is not an MCUXpresso SDK 2.x with devices/$DEVICE"
echo "SDK: $SDK"
if [ -n "${NXP_PIN_MUX:-}" ]; then
  PIN_MUX="$NXP_PIN_MUX"
else
  PIN_MUX="$DL/pin_mux"
  if [ ! -f "$PIN_MUX/pin_mux.c" ] || [ ! -f "$PIN_MUX/pin_mux.h" ]; then
    mkdir -p "$PIN_MUX"
    for f in pin_mux.c pin_mux.h; do
      curl -fsSL -o "$PIN_MUX/$f" "$PIN_MUX_URL/$f" || fail "download of the uart example's $f failed"
    done
  fi
fi
[ -f "$PIN_MUX/pin_mux.c" ] || fail "no pin_mux.c in $PIN_MUX"

# ---- CMSIS core headers ----
CMSIS="$(ls -d "$ARDUINO15"/packages/*/tools/CMSIS/*/CMSIS/Include 2>/dev/null | sort -V | tail -1)"
if [ -z "$CMSIS" ] || [ ! -f "$CMSIS/core_cm4.h" ]; then
  CMSIS="$DL/cmsis"
  if [ ! -f "$CMSIS/core_cm4.h" ]; then
    echo "Downloading the CMSIS core headers into $CMSIS"
    mkdir -p "$CMSIS"
    for f in core_cm4.h cmsis_compiler.h cmsis_gcc.h cmsis_version.h mpu_armv7.h; do
      curl -sfL -o "$CMSIS/$f" "$CMSIS_URL/$f" || fail "download of $f failed"
    done
  fi
fi

# ---- build ----
rm -rf "$OUT/obj" "$OUT/sdkobj" "$OUT"/OnOff_Serial.*
mkdir -p "$OUT/obj" "$OUT/sdkobj"
: > "$OUT/warnings.txt"

P() { toPath "$1"; }
DEV="$SDK/devices/$DEVICE"
INC="-I$(P "$EX") -I$(P "$ROOT/src") -I$(P "$PIN_MUX") -I$(P "$SDK/boards/$BOARD") -I$(P "$DEV") -I$(P "$DEV/drivers")"
for d in common uart gpio port smc; do INC="$INC -I$(P "$SDK/drivers/$d")"; done
INC="$INC -I$(P "$CMSIS")"
COMMON="$MCU $DEFS $INC -Os -g -fno-common -ffunction-sections -fdata-sections -ffreestanding -fno-builtin"

errors=0
# the SDK sources (and pin_mux.c) -- NXP's code, its warnings are not shown
for f in "$DEV/system_$DEVICE.c" "$DEV/drivers/fsl_clock.c" "$SDK/drivers/common/fsl_common.c" \
  "$SDK/drivers/common/fsl_common_arm.c" "$SDK/drivers/uart/fsl_uart.c" "$SDK/drivers/gpio/fsl_gpio.c" \
  "$SDK/drivers/smc/fsl_smc.c" "$SDK/boards/$BOARD/clock_config.c" "$PIN_MUX/pin_mux.c"; do
  "${TC}gcc" -c -std=gnu99 $COMMON -w "$(P "$f")" -o "$(P "$OUT/sdkobj/$(basename "${f%.*}").o")" \
    || { echo "compile failed: $f"; errors=1; }
done
"${TC}gcc" -c -x assembler-with-cpp $MCU -DNDEBUG -D__STARTUP_CLEAR_BSS "$(P "$DEV/gcc/startup_$DEVICE.S")" \
  -o "$(P "$OUT/sdkobj/startup.o")" || { echo "assemble failed: startup_$DEVICE.S"; errors=1; }
# this example and pfodParserCPP, with warnings
for f in "$EX"/*.cpp "$ROOT"/src/*.cpp "$ROOT"/src/dwgs/*.cpp; do
  "${TC}g++" -c -std=gnu++11 $COMMON -Wall -Wextra -fno-exceptions -fno-rtti -fno-threadsafe-statics \
    "$(P "$f")" -o "$(P "$OUT/obj/$(basename "${f%.*}").o")" 2>>"$OUT/warnings.txt" || { echo "compile failed: $f"; errors=1; }
done
for f in "$ROOT"/src/*.c; do
  "${TC}gcc" -c -std=gnu99 $COMMON -Wall -Wextra "$(P "$f")" -o "$(P "$OUT/obj/$(basename "${f%.*}").o")" 2>>"$OUT/warnings.txt" || { echo "compile failed: $f"; errors=1; }
done
[ -s "$OUT/warnings.txt" ] && cat "$OUT/warnings.txt"
[ $errors = 0 ] || fail "compile failed"

"${TC}g++" $MCU "-T$(P "$DEV/gcc/$LDSCRIPT_NAME")" -Wl,--gc-sections --specs=nano.specs --specs=nosys.specs \
  "-Wl,-Map=$(P "$OUT/OnOff_Serial.map")" "$(P "$OUT")"/sdkobj/*.o "$(P "$OUT")"/obj/*.o \
  -o "$(P "$OUT/OnOff_Serial.elf")" || fail "link failed"
"${TC}size" "$(P "$OUT/OnOff_Serial.elf")"
"${TC}objcopy" -O ihex "$(P "$OUT/OnOff_Serial.elf")" "$(P "$OUT/OnOff_Serial.hex")"
"${TC}objcopy" -O binary "$(P "$OUT/OnOff_Serial.elf")" "$(P "$OUT/OnOff_Serial.bin")"
echo "Built $OUT/OnOff_Serial.elf / .hex / .bin"

if [ $DELETE_SDK = 1 ]; then
  rm -rf "$DL"
  echo "Deleted the downloads in $DL"
fi
