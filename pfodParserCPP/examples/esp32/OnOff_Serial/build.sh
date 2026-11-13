#!/bin/bash
# (c)2014-2026 Forward Computing and Control Pty. Ltd.
# NSW Australia, www.forward.com.au
# This code is not warranted to be fit for any purpose. You may only use it at your own risk.
# This code may be freely used for both private and commercial use
# Provided this copyright is maintained.
#
# build.sh -- builds this OnOff_Serial example for an ESP32 (ESP32 Dev Module:
# 4MB flash, DIO, 40MHz) WITHOUT installing ESP-IDF, using the prebuilt ESP-IDF
# libraries, headers, linker scripts, xtensa toolchain and esptool that come with
# the Arduino ESP32 core (Arduino IDE Boards Manager, "esp32 by Espressif", 3.x).
# None of the Arduino core code is used; app_main() comes from main.cpp.
#
# Run from Git Bash (Windows), or bash on Linux / macOS:
#   bash build.sh
# Output, in build/ in this directory:
#   OnOff_Serial.elf / .map         the linked program
#   OnOff_Serial.merged.bin         bootloader + partitions + app, flash at 0x0:
#     esptool --chip esp32 -p COMx write-flash 0x0 build/OnOff_Serial.merged.bin
#   bootloader.bin (0x1000), partitions.bin (0x8000), boot_app0.bin (0xe000),
#   OnOff_Serial.bin (0x10000)      the separate images
#
# Optional environment variables:
#   ARDUINO15          the Arduino15 directory, default %LOCALAPPDATA%/Arduino15 (Windows),
#                      ~/.arduino15 (Linux), ~/Library/Arduino15 (macOS)
#   ESP32_CORE_VERSION the Arduino ESP32 core version to use, default the highest installed
#   PFOD_PARSER_CPP    the pfodParserCPP directory, default three directories up from here
#
# The ESP-IDF configuration (sdkconfig) is the one the Arduino core was built with,
# e.g. the ESP-IDF log output is on UART0, pfod is on UART1 (see main.cpp).
# Building with idf.py in an ESP-IDF project instead also works, see main.cpp.

set -u

EX="$(cd "$(dirname "$0")" && pwd)"
ROOT="${PFOD_PARSER_CPP:-$EX/../../..}"
OUT="$EX/build"

# toPath -- convert a path for the native tools (C:/... form under Git Bash on Windows)
toPath() {
  if command -v cygpath >/dev/null 2>&1; then cygpath -m "$1"; else echo "$1"; fi
}

# fail -- print an error and stop
fail() {
  echo "ERROR: $*" >&2
  exit 1
}

# latest -- the highest version sub-directory of $1
latest() {
  ls -1 "$1" 2>/dev/null | sort -V | tail -1
}

if [ -z "${ARDUINO15:-}" ]; then
  if [ -n "${LOCALAPPDATA:-}" ]; then
    ARDUINO15="$(toPath "$LOCALAPPDATA")/Arduino15"
  elif [ "$(uname)" = "Darwin" ]; then
    ARDUINO15="$HOME/Library/Arduino15"
  else
    ARDUINO15="$HOME/.arduino15"
  fi
fi
PKG="$ARDUINO15/packages/esp32"
[ -d "$PKG" ] || fail "Arduino ESP32 core not found in $PKG (install it, or set ARDUINO15)"

VER="${ESP32_CORE_VERSION:-$(latest "$PKG/hardware/esp32")}"
CORE="$PKG/hardware/esp32/$VER"
SDK="$PKG/tools/esp32-libs/$VER"
[ -d "$SDK/flags" ] || fail "prebuilt ESP-IDF libs not found in $SDK (needs Arduino ESP32 core 3.x)"
TCDIR="$PKG/tools/esp-x32/$(latest "$PKG/tools/esp-x32")/bin"
TC="$TCDIR/xtensa-esp32-elf-"
[ -x "${TC}g++" ] || [ -x "${TC}g++.exe" ] || fail "xtensa toolchain not found in $TCDIR"
ESPTOOL="$(ls "$PKG"/tools/esptool_py/*/esptool* 2>/dev/null | grep -E 'esptool(\.exe|\.py)?$' | sort -V | tail -1)"
[ -n "$ESPTOOL" ] || fail "esptool not found in $PKG/tools/esptool_py"
# runPy -- run a tool that is an .exe on Windows or a .py script elsewhere
runPy() {
  local tool="$1"; shift
  case "$tool" in
    *.py) python3 "$tool" "$@" ;;
    *) "$tool" "$@" ;;
  esac
}
if [ -e "$CORE/tools/gen_esp32part.exe" ]; then GENPART="$CORE/tools/gen_esp32part.exe"; else GENPART="$CORE/tools/gen_esp32part.py"; fi

echo "Arduino ESP32 core $VER, $(cat "$SDK/versions.txt" | grep esp-idf)"

MEM=dio_qspi # build.boot=dio for the ESP32 Dev Module
EXP="$(toPath "$EX")"; ROOTP="$(toPath "$ROOT")"; SDKP="$(toPath "$SDK")"; OUTP="$(toPath "$OUT")"
CPPFLAGS="@$SDKP/flags/defines -I$EXP -I$ROOTP/src -iprefix $SDKP/include/ @$SDKP/flags/includes -I$SDKP/$MEM/include"
WARN="-Wall -Wextra -Os -Werror=return-type"

rm -rf "$OUT"
mkdir -p "$OUT/obj"
: > "$OUT/warnings.txt"

errors=0
for f in "$EX"/*.cpp "$ROOT"/src/*.cpp "$ROOT"/src/dwgs/*.cpp; do
  o="$OUTP/obj/$(basename "${f%.*}").o"
  "${TC}g++" -c "@$SDKP/flags/cpp_flags" $WARN $CPPFLAGS "$(toPath "$f")" -o "$o" 2>>"$OUT/warnings.txt" || { echo "compile failed: $f"; errors=1; }
done
for f in "$ROOT"/src/*.c; do
  o="$OUTP/obj/$(basename "${f%.*}").o"
  "${TC}gcc" -c "@$SDKP/flags/c_flags" $WARN $CPPFLAGS "$(toPath "$f")" -o "$o" 2>>"$OUT/warnings.txt" || { echo "compile failed: $f"; errors=1; }
done
[ -s "$OUT/warnings.txt" ] && cat "$OUT/warnings.txt"
[ $errors = 0 ] || fail "compile failed"

# link, as the Arduino core's recipe.c.combine.pattern but without the Arduino core's --wrap options
"${TC}g++" "-Wl,--Map=$OUTP/OnOff_Serial.map" -L$SDKP/lib -L$SDKP/ld -L$SDKP/$MEM \
  "@$SDKP/flags/ld_flags" "@$SDKP/flags/ld_scripts" \
  -Wl,--start-group "$OUTP"/obj/*.o "@$SDKP/flags/ld_libs" -Wl,--end-group -Wl,-EL \
  -o "$OUTP/OnOff_Serial.elf" || fail "link failed"
"${TC}size" "$OUTP/OnOff_Serial.elf"

# flash images
cd "$OUT" || fail "cannot cd to $OUT"
runPy "$ESPTOOL" --chip esp32 elf2image --flash-mode dio --flash-freq 40m --flash-size 4MB \
  --elf-sha256-offset 0xb0 -o OnOff_Serial.bin OnOff_Serial.elf >/dev/null || fail "elf2image (app) failed"
runPy "$ESPTOOL" --chip esp32 elf2image --flash-mode dio --flash-freq 40m --flash-size 4MB \
  -o bootloader.bin "$SDKP/bin/bootloader_dio_40m.elf" >/dev/null || fail "elf2image (bootloader) failed"
cp "$CORE/tools/partitions/default.csv" partitions.csv
runPy "$GENPART" -q partitions.csv partitions.bin || fail "gen_esp32part failed"
cp "$CORE/tools/partitions/boot_app0.bin" .
runPy "$ESPTOOL" --chip esp32 merge-bin -o OnOff_Serial.merged.bin --pad-to-size 4MB \
  --flash-mode keep --flash-freq keep --flash-size keep \
  0x1000 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 OnOff_Serial.bin >/dev/null || fail "merge-bin failed"
echo "Built $OUT/OnOff_Serial.merged.bin -- flash at 0x0"
