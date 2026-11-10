#!/bin/sh
# Mac / Linux:   ./build.sh /path/to/gbdk
set -e
cd "$(dirname "$0")"
GBDK="${1:-$GBDK_HOME}"
if [ -z "$GBDK" ]; then echo "Usage: ./build.sh /path/to/gbdk   (or set GBDK_HOME)"; exit 1; fi
case "$GBDK" in */) ;; *) GBDK="$GBDK/";; esac
if [ ! -x "${GBDK}bin/lcc" ]; then echo "Could not find ${GBDK}bin/lcc - is that the GBDK-2020 folder?"; exit 1; fi
make GBDK_HOME="$GBDK"
echo
echo "Done: build/moonlight-holler.gb"
