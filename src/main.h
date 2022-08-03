
#pragma once

#include <pax_gfx.h>
#include <pax_codecs.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>

extern pax_buf_t gbuf;

char *dump_file(const char *path);
bool disp_make_buf(const char *fb_name);
void disp_flush();
void disp_sync();
uint64_t milliTime();
uint64_t microTime();
