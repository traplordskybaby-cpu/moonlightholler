# Moonlight Holler -- Game Boy Color
#
#   make            build build/moonlight-holler.gb   (needs GBDK-2020 4.2 or newer and Python 3)
#   make host-test  run the desktop unit tests
#
# GBDK_HOME must point at the GBDK folder, e.g.  make GBDK_HOME=/opt/gbdk/   (keep the trailing slash)

GBDK_HOME ?= /opt/gbdk/
PYTHON ?= python3
LCC := $(GBDK_HOME)bin/lcc
TITLE ?= MOONLIGHTHOLLER
TRACK ?= 0
EXTRA ?=
OUT := build/game.gb
ROM := build/moonlight-holler.gb
SOURCES := $(wildcard src/*.c) $(wildcard src/*/*.c)

CFLAGS := -msm83:gb -Isrc -Wf--opt-code-size
# MBC5 cartridge, 4 ROM banks: banks 0-1 hold the code, banks 2-3 hold art, maps and text
LDFLAGS := -msm83:gb -Wl-m -Wl-j -Wm-yn"$(TITLE)" -Wm-yt0x19 -Wm-yo4 -Wm-yc

.PHONY: all release assets test jukebox host-test clean

all: release

assets:
	mkdir -p build
	$(PYTHON) tools/make_assets.py

release: assets
	$(LCC) $(CFLAGS) $(EXTRA) $(LDFLAGS) -o $(OUT) $(SOURCES)
	$(PYTHON) tools/pack_rom.py $(OUT) build/databank.bin $(ROM)
	$(PYTHON) tools/verify_rom.py $(ROM)

test: assets
	$(LCC) $(CFLAGS) -DTEST_PROBE $(EXTRA) $(LDFLAGS) -o $(OUT) $(SOURCES)
	$(PYTHON) tools/pack_rom.py $(OUT) build/databank.bin $(ROM)

jukebox: assets
	$(LCC) $(CFLAGS) -DJUKEBOX_TRACK=$(TRACK) $(LDFLAGS) -o $(OUT) $(SOURCES)
	$(PYTHON) tools/pack_rom.py $(OUT) build/databank.bin $(ROM)

host-test:
	mkdir -p build
	gcc -std=c89 -Wall -Wextra -Werror -pedantic -Isrc -Isrc/shared tests/test_breeding.c src/shared/breeding.c -o build/test_breeding && ./build/test_breeding
	gcc -std=c89 -Wall -Wextra -Werror -pedantic -Isrc -Isrc/shared tests/test_farm_animals.c src/shared/breeding.c src/shared/farm_animals.c -o build/test_farm_animals && ./build/test_farm_animals

clean:
	rm -rf build
