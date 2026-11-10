#!/bin/sh
# Desktop test build: compiles the real game sources against a stand-in for the GBDK library.
set -e
cd "$(dirname "$0")/.."
gcc -std=gnu11 -O1 -g -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-char-subscripts \
    -DHOST_TEST -Ihost/mock -Isrc $(ls src/*.c src/*/*.c) host/mock/mock.c -o build/game_host
