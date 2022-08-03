
# Options
CC            ?=/usr/bin/gcc
BUILD_DIR     ?=build
CCOPTIONS     ?=-c -Isrc -Icomponents/pax-gfx/src -Icomponents/pax-codecs/include
LDOPTIONS     ?=
LIBS          ?=-lm

# Sources
SOURCES        =$(shell find src -type f -name '*.c')
HEADERS        =$(shell find src -type f -name '*.h')

# Outputs
OBJECTS        =$(shell echo $(SOURCES) | sed -e 's/src/$(BUILD_DIR)/g;s/\.c/.c.o/g')
OBJECTS_DEBUG  =$(shell echo $(SOURCES) | sed -e 's/src/$(BUILD_DIR)/g;s/\.c/.c.debug.o/g')
OUT_PATH      ?=paxfbdev

# Actions
.PHONY: all debug clean run components/pax-gfx/build/libpax.so components/pax-codecs/build/libpaxcodecs.so

all: build/app.o
	@mkdir -p build
	@cp build/app.o $(OUT_PATH)

debug: build/app.debug.o
	@mkdir -p build
	@cp build/app.debug.o $(OUT_PATH)

clean:
	$(MAKE) -C components/pax-gfx -f Standalone.mk clean
	$(MAKE) -C components/pax-codecs -f Standalone.mk clean
	rm -rf build
	rm -f $(OUT_PATH)

run: $(OUT_PATH)
	./$(OUT_PATH)

# Regular files
build/app.o: $(OBJECTS) components/pax-gfx/build/libpax.so components/pax-codecs/build/libpaxcodecs.so
	@mkdir -p $(shell dirname $@)
	$(MAKE) -C components/pax-gfx -f Standalone.mk all
	$(MAKE) -C components/pax-codecs -f Standalone.mk all
	$(CC) $(LDOPTIONS) -o $@ $^ $(LIBS)

build/%.o: src/% $(HEADERS)
	@mkdir -p $(shell dirname $@)
	$(CC) $(CCOPTIONS) -o $@ $< $(LIBS)

# Debug files
build/app.debug.o: $(OBJECTS_DEBUG) components/pax-gfx/build/libpax.so components/pax-codecs/build/libpaxcodecs.so
	@mkdir -p $(shell dirname $@)
	$(MAKE) -C components/pax-gfx -f Standalone.mk debug
	$(MAKE) -C components/pax-codecs -f Standalone.mk debug
	$(CC) $(LDOPTIONS) -ggdb -o $@ $^ $(LIBS)

build/%.debug.o: src/% $(HEADERS)
	@mkdir -p $(shell dirname $@)
	$(CC) $(CCOPTIONS) -ggdb -o $@ $< $(LIBS)
