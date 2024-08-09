
# SPDX-License-Identidier: MIT

MAKEFLAGS += --silent --no-print-directory

.PHONY: all build clean run

all: build

build:
	mkdir -p build
	cmake -B build
	cmake --build build

clean:
	rm -rf build

run: build
	./build/pax-fbdev

gdb: build
	gdb ./build/pax-fbdev
