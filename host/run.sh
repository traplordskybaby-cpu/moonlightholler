#!/bin/sh
# usage: host/run.sh NAME FRAMES "KEYS" [dump,frames]   -> build/NAME_f<frame>.png
cd "$(dirname "$0")/.."
NAME=$1; FRAMES=$2; KEYS=$3; DUMPS=${4:-$2}; CMDS=${5:-}
GB_CMDS="$CMDS" GB_PREFIX=build/$NAME GB_FRAMES=$FRAMES GB_KEYS="$KEYS" GB_DUMPS="$DUMPS" GB_DATABANK=build/databank.bin ./build/game_host
for d in $(echo "$DUMPS" | tr ',' ' '); do
  python3 host/render_dump.py build/${NAME}_f$d.bin build/${NAME}_f$d.png 3
  cat build/${NAME}_f$d.txt
done
