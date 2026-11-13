#!/bin/bash
# (c)2014-2026 Forward Computing and Control Pty. Ltd.
# NSW Australia, www.forward.com.au
# This code is not warranted to be fit for any purpose. You may only use it at your own risk.
# This code may be freely used for both private and commercial use
# Provided this copyright is maintained.
#
# build.sh -- builds this OnOff_Serial example as a Zephyr app, default for the
# nRF52840 DK, using west and the Zephyr SDK's arm-zephyr-eabi toolchain, with the
# CMakeLists.txt and prj.conf in this directory.
#
# Run from Git Bash (Windows), or bash on Linux / macOS:
#   bash build.sh               build
#   bash build.sh --delete-sdk  build, then delete the Zephyr, toolchain and python
#                               packages build.sh downloaded
# Output, in build/app in this directory: zephyr/zephyr.hex, to flash with
#   build/venv/Scripts/west flash -d build/app     (build/venv/bin/west on Linux / macOS)
# or with nrfutil / nRF Connect Programmer.
#
# Needs: git, curl, and python 3.10 or later with venv (on Ubuntu / WSL: apt install python3-venv).
#
# Zephyr comes from (first found):
#   1. ZEPHYR_WS, the path of an existing Zephyr (or nRF Connect SDK) west workspace (the
#      directory holding .west/). Its west, python packages and toolchain are used as
#      already set up, i.e. west must be on the PATH and ZEPHYR_SDK_INSTALL_DIR set, or a
#      Zephyr SDK registered, as for building the Zephyr samples. Nothing is downloaded.
#   2. otherwise downloaded into build/ (about 3.8GB on disk), and kept for the next build
#      unless --delete-sdk is given:
#      - build/venv: a python venv with west, cmake, ninja and Zephyr's build requirements
#      - build/ws: Zephyr ZEPHYR_REV (default v4.5.0) with `west update` (shallow clones)
#        of only the modules this build needs: cmsis, cmsis_6, picolibc, segger and
#        ZEPHYR_HAL (default hal_nordic)
#      - build/zephyr-sdk-<version>: the Zephyr SDK version in zephyr/SDK_VERSION, minimal
#        bundle plus the arm-zephyr-eabi toolchain only
# PFOD_PARSER_CPP overrides the pfodParserCPP directory, default three directories up from here.
# BOARD overrides the board, default nrf52840dk/nrf52840. For a board that is not a Nordic
# one also set ZEPHYR_HAL to its HAL module(s), comma separated, e.g. ZEPHYR_HAL=hal_stm32
# BOARD=nucleo_f401re (and delete build/ws so it is fetched again). The board needs an ARM
# Cortex-M part (only arm-zephyr-eabi is downloaded), and a led0 alias, see target_Pin.cpp.
#
# Windows has a 260 character path limit, which the build's object file paths can reach if
# this example is in a deeply nested directory. Move it to a shorter path if `ar` reports
# "No such file or directory" for a .obj file that exists.

set -u

ZEPHYR_URL=https://github.com/zephyrproject-rtos/zephyr
ZEPHYR_REV="${ZEPHYR_REV:-v4.5.0}"
ZEPHYR_HAL="${ZEPHYR_HAL:-hal_nordic}"
ZEPHYR_SDK_URL=https://github.com/zephyrproject-rtos/sdk-ng/releases/download
BOARD="${BOARD:-nrf52840dk/nrf52840}"

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

# findPython -- echo the first python 3.10+ found on the PATH, or nothing.
# Each candidate is run, so the Windows Store python3 alias (which only opens the Store)
# is skipped.
findPython() {
  for p in python3 python; do
    if "$p" -c 'import sys; sys.exit(0 if sys.version_info >= (3, 10) else 1)' >/dev/null 2>&1; then
      echo "$p"
      return
    fi
  done
}

# hostName -- echo the Zephyr SDK host name for this machine and archive type,
# e.g. "windows-x86_64 7z", "linux-x86_64 tar.xz", "macos-aarch64 tar.xz"
hostName() {
  case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) echo "windows-x86_64 7z" ;;
    Linux)  echo "linux-$(uname -m) tar.xz" ;;
    Darwin) case "$(uname -m)" in arm64) echo "macos-aarch64 tar.xz" ;; *) echo "macos-x86_64 tar.xz" ;; esac ;;
    *) fail "unsupported host $(uname -s)" ;;
  esac
}

# extract <archive> <destination dir> -- unpack a Zephyr SDK .7z (with the venv's py7zr)
# or .tar.xz archive into the destination directory
extract() {
  mkdir -p "$2"
  case "$1" in
    *.7z) "$VPY" -I -c 'import sys, py7zr; py7zr.SevenZipFile(sys.argv[1]).extractall(sys.argv[2])' \
            "$(toPath "$1")" "$(toPath "$2")" ;;
    *) tar -xJf "$1" -C "$2" ;;
  esac
}

if [ -n "${ZEPHYR_WS:-}" ]; then
  # ---- existing workspace, set up by the user ----
  WS="$ZEPHYR_WS"
  DELETE_SDK=0 # never delete an SDK build.sh did not download
  command -v west >/dev/null 2>&1 || fail "ZEPHYR_WS is set but west is not on the PATH"
  WEST=west
else
  # ---- python venv with west etc ----
  VENV="$OUT/venv"
  if [ -d "$VENV/Scripts" ]; then VBIN="$VENV/Scripts"; else VBIN="$VENV/bin"; fi
  if [ ! -f "$VENV/.pfod_ok" ]; then
    PY="$(findPython)"
    [ -n "$PY" ] || fail "python 3.10 or later not found"
    echo "Creating the python venv $VENV"
    rm -rf "$VENV"
    "$PY" -m venv "$VENV" || fail "python -m venv failed (on Ubuntu / WSL: apt install python3-venv)"
    if [ -d "$VENV/Scripts" ]; then VBIN="$VENV/Scripts"; else VBIN="$VENV/bin"; fi
    "$VBIN/python" -m pip install -q --upgrade pip || fail "pip upgrade failed"
    "$VBIN/python" -m pip install -q west cmake ninja py7zr || fail "pip install of west cmake ninja py7zr failed"
  fi
  VPY="$VBIN/python"
  export PATH="$VBIN:$PATH"
  WEST="$VBIN/west"

  # ---- Zephyr workspace, only the modules needed ----
  WS="$OUT/ws"
  if [ ! -f "$WS/.pfod_ok" ]; then
    echo "Downloading Zephyr $ZEPHYR_REV and the cmsis, picolibc, segger, $ZEPHYR_HAL modules into $WS"
    rm -rf "$WS"
    "$WEST" init -m "$ZEPHYR_URL" --mr "$ZEPHYR_REV" "$(toPath "$WS")" || fail "west init failed"
    (cd "$WS" && "$WEST" config manifest.project-filter -- "-.*,+cmsis,+cmsis_6,+picolibc,+segger,+${ZEPHYR_HAL//,/,+}") \
      || fail "west config manifest.project-filter failed"
    (cd "$WS" && "$WEST" update --narrow -o=--depth=1) || fail "west update failed"
    "$VPY" -m pip install -q -r "$WS/zephyr/scripts/requirements-base.txt" \
      || fail "pip install of the Zephyr requirements failed"
    touch "$WS/.pfod_ok" "$VENV/.pfod_ok"
  fi

  # ---- Zephyr SDK, minimal + arm-zephyr-eabi ----
  SDK_VER="$(tr -d ' \r\n' < "$WS/zephyr/SDK_VERSION")"
  ZSDK="$OUT/zephyr-sdk-$SDK_VER"
  set -- $(hostName)
  HOST="$1"; ARC="$2"
  # Zephyr SDK 1.x has the GNU toolchains in gnu/ and names them toolchain_gnu_...
  case "$SDK_VER" in
    0.*) TC_FILE="toolchain_${HOST}_arm-zephyr-eabi.$ARC"; TC_DIR="$ZSDK" ;;
    *)   TC_FILE="toolchain_gnu_${HOST}_arm-zephyr-eabi.$ARC"; TC_DIR="$ZSDK/gnu" ;;
  esac
  if [ ! -f "$ZSDK/.pfod_ok" ]; then
    echo "Downloading the Zephyr SDK $SDK_VER ($HOST, arm-zephyr-eabi only) into $ZSDK"
    rm -rf "$ZSDK" "$OUT/dl"
    mkdir -p "$OUT/dl"
    MIN_FILE="zephyr-sdk-${SDK_VER}_${HOST}_minimal.$ARC"
    curl -fL -sS -o "$OUT/dl/$MIN_FILE" "$ZEPHYR_SDK_URL/v$SDK_VER/$MIN_FILE" || fail "download of $MIN_FILE failed"
    curl -fL -sS -o "$OUT/dl/$TC_FILE" "$ZEPHYR_SDK_URL/v$SDK_VER/$TC_FILE" || fail "download of $TC_FILE failed"
    extract "$OUT/dl/$MIN_FILE" "$OUT" || fail "unpacking $MIN_FILE failed" # unpacks to zephyr-sdk-<ver>/
    extract "$OUT/dl/$TC_FILE" "$TC_DIR" || fail "unpacking $TC_FILE failed" # unpacks to arm-zephyr-eabi/
    rm -rf "$OUT/dl"
    touch "$ZSDK/.pfod_ok"
  fi
  export ZEPHYR_SDK_INSTALL_DIR="$(toPath "$ZSDK")"
  export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
fi
[ -d "$WS/.west" ] && [ -d "$WS/zephyr" ] || fail "$WS is not a west workspace with zephyr"
echo "Workspace: $WS"
export ZEPHYR_BASE="$(toPath "$WS/zephyr")"

# ---- build ----
EXTRA=()
[ -n "${PFOD_PARSER_CPP:-}" ] && EXTRA=(-- "-DPFOD_PARSER_CPP_DIR=$(toPath "$PFOD_PARSER_CPP")")
(cd "$WS" && "$WEST" build -p always -b "$BOARD" -d "$(toPath "$OUT/app")" "$(toPath "$EX")" "${EXTRA[@]}") \
  || fail "build failed"
[ -f "$OUT/app/zephyr/zephyr.hex" ] || fail "build/app/zephyr/zephyr.hex was not produced"
echo "Built $OUT/app/zephyr/zephyr.hex"

if [ $DELETE_SDK = 1 ]; then
  rm -rf "$OUT/ws" "$OUT/venv" "$OUT"/zephyr-sdk-* "$OUT/dl"
  echo "Deleted the downloaded Zephyr, toolchain and python venv from $OUT"
fi
