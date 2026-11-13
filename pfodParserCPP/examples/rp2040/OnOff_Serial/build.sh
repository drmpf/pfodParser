#!/bin/bash
# (c)2014-2026 Forward Computing and Control Pty. Ltd.
# NSW Australia, www.forward.com.au
# This code is not warranted to be fit for any purpose. You may only use it at your own risk.
# This code may be freely used for both private and commercial use
# Provided this copyright is maintained.
#
# build.sh -- builds this OnOff_Serial example for the Raspberry Pi Pico (RP2040), or
# another RP2040 / RP2350 board, with the Pico SDK, using the CMakeLists.txt in this directory.
#
# Run from Git Bash (Windows), or bash on Linux / macOS:
#   bash build.sh               build
#   bash build.sh --delete-sdk  build, then delete anything build.sh downloaded
# Output, in build/app in this directory: OnOff_Serial.elf / .hex / .bin / .uf2
# Copy the .uf2 to the board's RPI-RP2 (RP2350: RP2350) drive, shown when it is plugged
# in with BOOTSEL held down.
#
# Nothing needs to be downloaded if the Arduino IDE has the Raspberry Pi Pico/RP2040/RP2350
# core (earlephilhower arduino-pico) installed, as it includes the Pico SDK and picotool,
# plus an Arduino core that installs a standard arm-none-eabi-gcc (e.g. Adafruit nRF52 /
# SAMD, Arduino SAMD, Seeeduino). Otherwise:
# The Pico SDK comes from (first found):
#   1. PICO_SDK_PATH, the path of a pico-sdk directory
#   2. the highest version packages/rp2040/hardware/rp2040/*/pico-sdk in Arduino15
#   3. otherwise a shallow git clone of pico-sdk PICO_SDK_REV (default 2.3.0) into build/,
#      without its submodules (only needed for USB, wifi and bluetooth)
# The toolchain comes from (first found):
#   1. PICO_TOOLCHAIN_PATH, the directory holding bin/arm-none-eabi-gcc
#   2. arm-none-eabi-gcc on the PATH
#   3. the highest version packages/*/tools/arm-none-eabi-gcc/* in Arduino15
#      (arduino-pico's own packages/rp2040/tools/pqt-gcc is NOT used: its newlib is built
#      with retargetable locks, which only arduino-pico's core supplies, so a plain Pico
#      SDK program fails to link with undefined __retarget_lock_acquire_recursive etc.)
#   otherwise install the Arm GNU Toolchain (arm-none-eabi) from developer.arm.com and
#   set PICO_TOOLCHAIN_PATH. Tested with 9-2019q4, for both RP2040 and RP2350.
# cmake and ninja come from the PATH, otherwise they are pip installed into a python
# venv in build/venv (needs python 3 with venv). The Pico SDK build also needs python 3.
# picotool (for the .uf2) comes from PICOTOOL, the PATH, or packages/rp2040/tools/pqt-picotool/*
# in Arduino15. Without it there is no .uf2; flash the .elf / .hex with a debug probe.
# ARDUINO15 overrides the Arduino15 location.
# PFOD_PARSER_CPP overrides the pfodParserCPP directory, default three directories up from here.
# PICO_BOARD overrides the board, default pico. e.g. PICO_BOARD=pico2 for the Pico 2 (RP2350).

set -u

PICO_SDK_URL=https://github.com/raspberrypi/pico-sdk.git
PICO_SDK_REV="${PICO_SDK_REV:-2.3.0}"
PICO_BOARD="${PICO_BOARD:-pico}"

EX="$(cd "$(dirname "$0")" && pwd)"
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

# findPython -- echo the first python 3 found on the PATH, or nothing.
# Each candidate is run, so the Windows Store python3 alias (which only opens the Store)
# is skipped.
findPython() {
  for p in python3 python; do
    if "$p" -c 'import sys; sys.exit(0 if sys.version_info >= (3, 8) else 1)' >/dev/null 2>&1; then
      echo "$p"
      return
    fi
  done
}

if [ -z "${ARDUINO15:-}" ]; then
  if [ -n "${LOCALAPPDATA:-}" ]; then ARDUINO15="$(toPath "$LOCALAPPDATA")/Arduino15"
  elif [ "$(uname)" = "Darwin" ]; then ARDUINO15="$HOME/Library/Arduino15"
  else ARDUINO15="$HOME/.arduino15"; fi
fi
ARP="$ARDUINO15/packages/rp2040"

# ---- Pico SDK ----
if [ -n "${PICO_SDK_PATH:-}" ]; then
  SDK="$PICO_SDK_PATH"
else
  SDK="$(ls -d "$ARP"/hardware/rp2040/*/pico-sdk 2>/dev/null | sort -V | tail -1)"
  if [ -z "$SDK" ]; then
    SDK="$OUT/pico-sdk"
    if [ ! -f "$SDK/.pfod_ok" ]; then
      echo "Downloading pico-sdk $PICO_SDK_REV into $SDK"
      rm -rf "$SDK"
      git clone -q --depth 1 --branch "$PICO_SDK_REV" "$PICO_SDK_URL" "$SDK" || fail "git clone of pico-sdk failed"
      touch "$SDK/.pfod_ok"
    fi
  fi
fi
[ -f "$SDK/external/pico_sdk_import.cmake" ] || fail "$SDK is not a pico-sdk directory"
echo "Pico SDK: $SDK"

# ---- toolchain ----
if [ -n "${PICO_TOOLCHAIN_PATH:-}" ]; then
  TCPATH="$PICO_TOOLCHAIN_PATH"
elif command -v arm-none-eabi-gcc >/dev/null 2>&1; then
  TCPATH="$(dirname "$(dirname "$(command -v arm-none-eabi-gcc)")")"
else
  TCPATH="$(ls -d "$ARDUINO15"/packages/*/tools/arm-none-eabi-gcc/* 2>/dev/null | sort -V | tail -1)"
  [ -n "$TCPATH" ] || fail "no arm-none-eabi-gcc found, install the Arm GNU Toolchain and set PICO_TOOLCHAIN_PATH"
fi
"$TCPATH/bin/arm-none-eabi-gcc" --version >/dev/null 2>&1 || fail "cannot run $TCPATH/bin/arm-none-eabi-gcc"
echo "Toolchain: $("$TCPATH/bin/arm-none-eabi-gcc" --version | head -1)"

# ---- python, cmake and ninja ----
PY="$(findPython)"
[ -n "$PY" ] || fail "python 3 not found, the Pico SDK build needs it"
if command -v cmake >/dev/null 2>&1 && command -v ninja >/dev/null 2>&1; then
  CMAKE=cmake
else
  VENV="$OUT/venv"
  if [ ! -f "$VENV/.pfod_ok" ]; then
    echo "Installing cmake and ninja into the python venv $VENV"
    rm -rf "$VENV"
    "$PY" -m venv "$VENV" || fail "python -m venv failed (on Ubuntu / WSL: apt install python3-venv)"
    if [ -d "$VENV/Scripts" ]; then VBIN="$VENV/Scripts"; else VBIN="$VENV/bin"; fi
    "$VBIN/python" -m pip install -q cmake ninja || fail "pip install of cmake ninja failed"
    touch "$VENV/.pfod_ok"
  fi
  if [ -d "$VENV/Scripts" ]; then VBIN="$VENV/Scripts"; else VBIN="$VENV/bin"; fi
  export PATH="$VBIN:$PATH"
  CMAKE="$VBIN/cmake"
fi

# ---- build ----
# PICO_NO_PICOTOOL stops the SDK downloading and building picotool if no picotool 2.3+
# is installed; the .uf2 is made with picotool below instead.
rm -rf "$OUT/app"
EXTRA=()
[ -n "${PFOD_PARSER_CPP:-}" ] && EXTRA=("-DPFOD_PARSER_CPP_DIR=$(toPath "$PFOD_PARSER_CPP")")
"$CMAKE" -S "$(toPath "$EX")" -B "$(toPath "$OUT/app")" -G Ninja \
  "-DPICO_SDK_PATH=$(toPath "$SDK")" "-DPICO_TOOLCHAIN_PATH=$(toPath "$TCPATH")" \
  "-DPICO_BOARD=$PICO_BOARD" -DPICO_NO_PICOTOOL=1 "-DPython3_EXECUTABLE=$(command -v "$PY")" \
  "${EXTRA[@]}" || fail "cmake configure failed"
"$CMAKE" --build "$(toPath "$OUT/app")" || fail "build failed"
"$TCPATH/bin/arm-none-eabi-size" "$(toPath "$OUT/app/OnOff_Serial.elf")"

# ---- .uf2 ----
if [ -n "${PICOTOOL:-}" ]; then
  PT="$PICOTOOL"
elif command -v picotool >/dev/null 2>&1; then
  PT=picotool
else
  PT="$(ls "$ARP"/tools/pqt-picotool/*/picotool* 2>/dev/null | sort -V | tail -1)"
fi
if [ -n "$PT" ]; then
  # for RP2350 add an absolute block, the RP2350-E9 erratum workaround, as the Pico SDK does
  ABS=()
  grep -q "^PICO_PLATFORM:.*=rp2350" "$OUT/app/CMakeCache.txt" && ABS=(--abs-block)
  "$PT" uf2 convert "$(toPath "$OUT/app/OnOff_Serial.elf")" "$(toPath "$OUT/app/OnOff_Serial.uf2")" "${ABS[@]}" \
    || fail "picotool uf2 convert failed"
  echo "Built $OUT/app/OnOff_Serial.uf2"
else
  echo "No picotool found, so no .uf2. Built $OUT/app/OnOff_Serial.elf / .hex"
fi

if [ $DELETE_SDK = 1 ]; then
  rm -rf "$OUT/pico-sdk" "$OUT/venv"
  echo "Deleted anything downloaded (pico-sdk, python venv) from $OUT"
fi
