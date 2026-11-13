#!/bin/bash
# (c)2014-2026 Forward Computing and Control Pty. Ltd.
# NSW Australia, www.forward.com.au
# This code is not warranted to be fit for any purpose. You may only use it at your own risk.
# This code may be freely used for both private and commercial use
# Provided this copyright is maintained.
#
# build.sh -- builds this OnOff_Serial example for the NXP FRDM-MCXN947 (MCXN947VDF,
# Cortex-M33 core 0) with the current MCUXpresso SDK and arm-none-eabi-gcc, without
# MCUXpresso IDE / west, using the SDK's own device files (headers, system file, gcc startup
# and linker script), drivers, FRDM-MCXN947 board files (board.h, clock_config.c) and the
# pin_mux.c of its lpuart interrupt example (LPUART4 / LP_FLEXCOMM4 on P1_8 / P1_9, the
# MCU-Link virtual COM port).
#
# The current MCUXpresso SDK is split over several git repositories (normally fetched with
# west, see github.com/nxp-mcuxpresso/mcuxsdk-manifests); this uses only the parts it needs:
#   github.com/nxp-mcuxpresso/mcuxsdk-core       drivers
#   github.com/nxp-mcuxpresso/mcux-devices-mcx   MCXN947 device files
#   github.com/nxp-mcuxpresso/mcuxsdk-examples   _boards/frdmmcxn947 board files and pin_mux
#
# Run from Git Bash (Windows), or bash on Linux / macOS:
#   bash build.sh               build
#   bash build.sh --delete-sdk  build, then delete anything build.sh downloaded
# Output, in build/ in this directory: OnOff_Serial.elf / .map / .hex / .bin
# Flash with LinkServer / pyOCD / MCUXpresso IDE through the board's MCU-Link debug probe.
#
# The SDK comes from (first found):
#   1. NXP_SDK, the root of a west fetched MCUXpresso SDK (holding mcuxsdk/ with drivers/,
#      devices/MCX/ and examples/_boards/), with NXP_PIN_MUX the directory holding your
#      pin_mux.c / pin_mux.h (default: the SDK's lpuart interrupt example's, as below)
#   2. otherwise sparse git clones of just the needed parts (about 17MB) into
#      build/download (needs git), plus the board files and pin_mux.c / pin_mux.h (needs curl)
# The CMSIS core headers come from the highest packages/*/tools/CMSIS/*/CMSIS/Include in
# Arduino15, otherwise they are downloaded from github.com/ARM-software/CMSIS_5.
# The toolchain comes from (first found):
#   1. NXP_GCC_BIN, the bin directory of an arm-none-eabi-gcc toolchain
#   2. arm-none-eabi-gcc on the PATH
#   3. the highest packages/*/tools/arm-none-eabi-gcc/*/bin in Arduino15; ARDUINO15 overrides
#      its location
# PFOD_PARSER_CPP overrides the pfodParserCPP directory, default three directories up from here.

set -u

GITHUB=https://github.com/nxp-mcuxpresso
RAW=https://raw.githubusercontent.com/nxp-mcuxpresso/mcuxsdk-examples/main
CMSIS_URL=https://raw.githubusercontent.com/ARM-software/CMSIS_5/develop/CMSIS/Core/Include
DEVICE=MCXN947
CORE=cm33_core0
BOARD=frdmmcxn947
DEFS="-DNDEBUG -DCPU_MCXN947VDF_cm33_core0 -DMCUXPRESSO_SDK"
MCU="-mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16"

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

# sparseClone <repo> <dir> <paths...> -- shallow, blobless, sparse (cone mode) clone of
# just the given directories of nxp-mcuxpresso/<repo>
sparseClone() {
  local repo="$1" dir="$2"
  shift 2
  rm -rf "$dir"
  git clone -q --depth 1 --filter=blob:none --sparse "$GITHUB/$repo.git" "$dir" || fail "git clone of $repo failed"
  git -C "$dir" sparse-checkout set "$@" || fail "git sparse-checkout of $repo failed"
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

# ---- SDK, board files and pin_mux ----
if [ -n "${NXP_SDK:-}" ]; then
  CORE_DIR="$NXP_SDK/mcuxsdk"
  DEV_DIR="$NXP_SDK/mcuxsdk/devices/MCX"
  BOARD_DIR="$NXP_SDK/mcuxsdk/examples/_boards/$BOARD"
  PIN_MUX="${NXP_PIN_MUX:-$BOARD_DIR/driver_examples/lpuart/interrupt}"
else
  CORE_DIR="$DL/core"
  DEV_DIR="$DL/devices"
  BOARD_DIR="$DL/board"
  PIN_MUX="${NXP_PIN_MUX:-$DL/board}"
  if [ ! -f "$DL/.pfod_ok" ]; then
    echo "Downloading the needed parts of the MCUXpresso SDK into $DL"
    mkdir -p "$DL"
    sparseClone mcuxsdk-core "$CORE_DIR" drivers/common drivers/lpflexcomm drivers/gpio drivers/port drivers/mcx_spc
    sparseClone mcux-devices-mcx "$DEV_DIR" "MCXN/$DEVICE" MCXN/periph MCXN/periph1
    mkdir -p "$BOARD_DIR"
    for f in board.h clock_config.c clock_config.h; do
      curl -fsSL -o "$BOARD_DIR/$f" "$RAW/_boards/$BOARD/$f" || fail "download of $f failed"
    done
    for f in pin_mux.c pin_mux.h; do
      curl -fsSL -o "$BOARD_DIR/$f" "$RAW/_boards/$BOARD/driver_examples/lpuart/interrupt/$f" || fail "download of $f failed"
    done
    touch "$DL/.pfod_ok"
  fi
fi
DEV="$DEV_DIR/MCXN/$DEVICE"
[ -f "$DEV/fsl_device_registers.h" ] || fail "$DEV is not the MCUXpresso SDK $DEVICE device directory"
[ -f "$BOARD_DIR/clock_config.c" ] || fail "no clock_config.c in $BOARD_DIR"
[ -f "$PIN_MUX/pin_mux.c" ] || fail "no pin_mux.c in $PIN_MUX"
echo "SDK drivers: $CORE_DIR"
echo "SDK device:  $DEV"

# ---- CMSIS core headers ----
CMSIS="$(ls -d "$ARDUINO15"/packages/*/tools/CMSIS/*/CMSIS/Include 2>/dev/null | sort -V | tail -1)"
if [ -z "$CMSIS" ] || [ ! -f "$CMSIS/core_cm33.h" ]; then
  CMSIS="$DL/cmsis"
  if [ ! -f "$CMSIS/core_cm33.h" ]; then
    echo "Downloading the CMSIS core headers into $CMSIS"
    mkdir -p "$CMSIS"
    for f in core_cm33.h cmsis_compiler.h cmsis_gcc.h cmsis_version.h mpu_armv8.h; do
      curl -sfL -o "$CMSIS/$f" "$CMSIS_URL/$f" || fail "download of $f failed"
    done
  fi
fi

# ---- build ----
rm -rf "$OUT/obj" "$OUT/sdkobj" "$OUT"/OnOff_Serial.*
mkdir -p "$OUT/obj" "$OUT/sdkobj"
: > "$OUT/warnings.txt"

P() { toPath "$1"; }
DRV="$CORE_DIR/drivers"
INC="-I$(P "$EX") -I$(P "$ROOT/src") -I$(P "$PIN_MUX") -I$(P "$BOARD_DIR") -I$(P "$DEV") -I$(P "$DEV/drivers")"
INC="$INC -I$(P "$DEV_DIR/MCXN/periph") -I$(P "$DEV_DIR/MCXN/periph1")"
for d in common lpflexcomm lpflexcomm/lpuart gpio port mcx_spc; do INC="$INC -I$(P "$DRV/$d")"; done
INC="$INC -I$(P "$CMSIS")"
COMMON="$MCU $DEFS $INC -Os -g -fno-common -ffunction-sections -fdata-sections -ffreestanding -fno-builtin"

errors=0
# the SDK sources (and pin_mux.c) -- NXP's code, its warnings are not shown
for f in "$DEV/system_${DEVICE}_$CORE.c" "$DEV/drivers/fsl_clock.c" "$DEV/drivers/fsl_reset.c" \
  "$DRV/common/fsl_common.c" "$DRV/common/fsl_common_arm.c" "$DRV/lpflexcomm/fsl_lpflexcomm.c" \
  "$DRV/lpflexcomm/lpuart/fsl_lpuart.c" "$DRV/gpio/fsl_gpio.c" "$DRV/mcx_spc/fsl_spc.c" \
  "$BOARD_DIR/clock_config.c" "$PIN_MUX/pin_mux.c"; do
  "${TC}gcc" -c -std=gnu99 $COMMON -w "$(P "$f")" -o "$(P "$OUT/sdkobj/$(basename "${f%.*}").o")" \
    || { echo "compile failed: $f"; errors=1; }
done
"${TC}gcc" -c -x assembler-with-cpp $MCU -DNDEBUG -D__STARTUP_CLEAR_BSS "$(P "$DEV/gcc/startup_${DEVICE}_$CORE.S")" \
  -o "$(P "$OUT/sdkobj/startup.o")" || { echo "assemble failed: startup_${DEVICE}_$CORE.S"; errors=1; }
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

"${TC}g++" $MCU "-T$(P "$DEV/gcc/${DEVICE}_${CORE}_flash.ld")" -Wl,--gc-sections --specs=nano.specs --specs=nosys.specs \
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
