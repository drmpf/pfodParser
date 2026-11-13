#!/bin/bash
# (c)2014-2026 Forward Computing and Control Pty. Ltd.
# NSW Australia, www.forward.com.au
# This code is not warranted to be fit for any purpose. You may only use it at your own risk.
# This code may be freely used for both private and commercial use
# Provided this copyright is maintained.
#
# build.sh -- builds this OnOff_Serial example for a PIC32 (PIC32MX / MZ / MK / MM) with
# MPLAB XC32, against the MPLAB Harmony 3 code MCC generated for your board, without
# MPLAB X.
#
# NOT RUN YET -- check it against your XC32 version and MCC output.
#
# Nothing is downloaded. Two things must be set up first, as neither can be scripted:
#   1. MPLAB XC32 installed (free from microchip.com, the installer needs a licence accept).
#   2. An MCC Harmony 3 project for your board, generated in MPLAB X with MCC, with
#        - a UART PLIB in "Ring buffer" operating mode (see main.cpp's TODO, UART1 by default)
#        - the LED pin set to GPIO, direction Out, named LED (see target_Pin.cpp's TODO)
#      MCC writes the generated code into the project's firmware/src/config/<config name>/
#      (definitions.h, initialization.c, interrupts.c, exceptions.c, peripheral/...).
#      This script compiles everything in that config directory, plus this example's files
#      and pfodParserCPP, instead of MCC's firmware/src/main.c (this main.cpp replaces it).
#
# Run from Git Bash (Windows), or bash on Linux / macOS:
#   PIC32_MCC_SRC=/path/to/project/firmware/src PIC32_PROCESSOR=32MZ2048EFM144 bash build.sh
# Output, in build/ in this directory: OnOff_Serial.elf / .map / .hex
# Flash the .hex with MPLAB IPE (or ipecmd) and your PICkit / ICD / on-board debugger.
#
# Environment variables:
#   PIC32_MCC_SRC     required, the MCC project's firmware/src directory
#   PIC32_PROCESSOR   required, the part as XC32's -mprocessor wants it, without the PIC
#                     prefix, e.g. 32MZ2048EFM144 (PIC32MZ EF Curiosity 2.0), 32MX470F512H
#   PIC32_MCC_CONFIG  the MCC configuration name under firmware/src/config, default "default"
#   XC32_BIN          the XC32 bin directory, otherwise xc32-gcc on the PATH, otherwise the
#                     highest installed version in the default install directory
#   PIC32_LDFLAGS     extra link options, e.g. -Wl,--defsym=_min_heap_size=1024
#                     (MPLAB X projects default to no heap; this example does not need one)
#   PFOD_PARSER_CPP   the pfodParserCPP directory, default three directories up from here
#
# XC32's free licence limits optimisation to -O1, which is used here.

set -u

EX="$(cd "$(dirname "$0")" && pwd)"
ROOT="${PFOD_PARSER_CPP:-$EX/../../..}"
OUT="$EX/build"
CONFIG="${PIC32_MCC_CONFIG:-default}"

# toPath -- convert a path for the native tools (C:/... form under Git Bash on Windows)
toPath() {
  if command -v cygpath >/dev/null 2>&1; then cygpath -m "$1"; else echo "$1"; fi
}

# fail -- print an error and stop
fail() {
  echo "ERROR: $*" >&2
  exit 1
}

# ---- settings ----
[ -n "${PIC32_MCC_SRC:-}" ] || fail "set PIC32_MCC_SRC to your MCC Harmony project's firmware/src directory"
[ -n "${PIC32_PROCESSOR:-}" ] || fail "set PIC32_PROCESSOR to your part, e.g. 32MZ2048EFM144"
CFG="$PIC32_MCC_SRC/config/$CONFIG"
[ -f "$CFG/definitions.h" ] || fail "$CFG/definitions.h not found, check PIC32_MCC_SRC and PIC32_MCC_CONFIG"

# ---- toolchain ----
if [ -n "${XC32_BIN:-}" ]; then
  TC="$XC32_BIN/xc32-"
elif command -v xc32-gcc >/dev/null 2>&1; then
  TC="xc32-"
else
  TCDIR="$(ls -d "/c/Program Files/Microchip/xc32"/v*/bin /opt/microchip/xc32/v*/bin \
    /Applications/microchip/xc32/v*/bin 2>/dev/null | sort -V | tail -1)"
  [ -n "$TCDIR" ] || fail "no XC32 found, install it or set XC32_BIN"
  TC="$TCDIR/xc32-"
fi
"${TC}gcc" --version >/dev/null 2>&1 || fail "cannot run ${TC}gcc"
echo "Toolchain: $("${TC}gcc" --version | head -1)"
echo "MCC config: $CFG"

# ---- build ----
rm -rf "$OUT"
mkdir -p "$OUT/obj" "$OUT/mcc"
: > "$OUT/warnings.txt"

P() { toPath "$1"; }
MCU="-mprocessor=$PIC32_PROCESSOR"
INC="-I$(P "$EX") -I$(P "$ROOT/src") -I$(P "$CFG") -I$(P "$PIC32_MCC_SRC")"
COMMON="$MCU $INC -O1 -g -ffunction-sections -fdata-sections"

errors=0
# MCC's generated code, its warnings are not shown. Object names include the directory,
# as MCC has some files with the same name in different directories.
while IFS= read -r f; do
  rel="${f#$CFG/}"
  obj="$OUT/mcc/$(echo "${rel%.*}" | tr '/' '_').o"
  case "$f" in
    *.c) "${TC}gcc" -c -std=gnu99 $COMMON -w "$(P "$f")" -o "$(P "$obj")" || { echo "compile failed: $f"; errors=1; } ;;
    *.S) "${TC}gcc" -c $MCU $INC "$(P "$f")" -o "$(P "$obj")" || { echo "assemble failed: $f"; errors=1; } ;;
  esac
done < <(find "$CFG" -type f \( -name '*.c' -o -name '*.S' \))
# this example and pfodParserCPP, with warnings
for f in "$EX"/*.cpp "$ROOT"/src/*.cpp "$ROOT"/src/dwgs/*.cpp; do
  "${TC}g++" -c -std=gnu++11 $COMMON -Wall -Wextra -fno-exceptions -fno-rtti \
    "$(P "$f")" -o "$(P "$OUT/obj/$(basename "${f%.*}").o")" 2>>"$OUT/warnings.txt" || { echo "compile failed: $f"; errors=1; }
done
for f in "$ROOT"/src/*.c; do
  "${TC}gcc" -c -std=gnu99 $COMMON -Wall -Wextra "$(P "$f")" -o "$(P "$OUT/obj/$(basename "${f%.*}").o")" 2>>"$OUT/warnings.txt" || { echo "compile failed: $f"; errors=1; }
done
[ -s "$OUT/warnings.txt" ] && cat "$OUT/warnings.txt"
[ $errors = 0 ] || fail "compile failed"

"${TC}g++" $MCU -Wl,--gc-sections "-Wl,-Map=$(P "$OUT/OnOff_Serial.map")" ${PIC32_LDFLAGS:-} \
  "$(P "$OUT")"/mcc/*.o "$(P "$OUT")"/obj/*.o -o "$(P "$OUT/OnOff_Serial.elf")" || fail "link failed"
"${TC}size" "$(P "$OUT/OnOff_Serial.elf")"
"${TC}bin2hex" "$(P "$OUT/OnOff_Serial.elf")" || fail "xc32-bin2hex failed"
echo "Built $OUT/OnOff_Serial.elf / .hex"
