
#include "main.h"

pax_buf_t       gbuf;
static FILE    *fbdev;
static uint8_t *fbmem;
static size_t   fbsize;
static int      stride;

int main(int argc, char **argv) {
    
    if (!disp_make_buf("/dev/fb0")) return 1;
    
    // Hide cursor (so it doesn't overwrite the framebuffer).
    // fputs("\033[?25l", stdout);
    fflush(stdout);
    pax_mark_dirty0(&gbuf);
    
    // Load the funny background image.
    FILE *background_fd = fopen("/home/julian/Pictures/epic.png", "rb");
    bool do_bg = true;
    pax_buf_t background;
    if (!pax_decode_png_fd(&background, background_fd, PAX_BUF_32_8888ARGB, CODEC_FLAG_OPTIMAL)) {
        printf("Background error.");
        do_bg = false;
    }
    pax_enable_multicore(0);
    
    uint64_t lastMicros = microTime();
    uint64_t dm = lastMicros;
    int numArcs = 3;
    float arcDx = 120;
    for (int i = 0; i < 150000000; i++) {
		uint64_t  micros = microTime() - dm;
        uint64_t  millis = micros / 1000;
		
		pax_col_t color0 = pax_col_ahsv(127, millis * 255 / 8000 + 127, 255, 255);
		pax_col_t color1 = pax_col_ahsv(127, millis * 255 / 8000, 255, 255);
        if (do_bg) {
            pax_draw_image(&gbuf, &background, 0, 0);
        } else {
		    pax_background(&gbuf, 0xff000000);
        }
		
		// Epic arcs demo.
		float a0 = millis / 5000.0 * M_PI;
		float a1 = fmodf(a0, M_PI * 4) - M_PI * 2;
		float a2 = millis / 8000.0 * M_PI;
        
        for (int x = 0; x < numArcs; x++) {
            pax_push_2d(&gbuf);
            if (numArcs > 1) pax_apply_2d(&gbuf, matrix_2d_translate(-arcDx/2*(numArcs-1)+arcDx*x, 0));
            pax_apply_2d(&gbuf, matrix_2d_translate(gbuf.width * 0.5f, gbuf.height * 0.5f));
            pax_apply_2d(&gbuf, matrix_2d_scale(200, 200));
            
            pax_apply_2d(&gbuf, matrix_2d_rotate(-a2));
            pax_draw_arc(&gbuf, color0, 0, 0, 1, a0 + a2, a0 + a1 + a2);
            
            pax_apply_2d(&gbuf, matrix_2d_rotate(a0 + a2));
            pax_push_2d(&gbuf);
            pax_apply_2d(&gbuf, matrix_2d_translate(1, 0));
            pax_draw_rect(&gbuf, color1, -0.25f, -0.25f, 0.5f, 0.5f);
            pax_pop_2d(&gbuf);
            
            pax_apply_2d(&gbuf, matrix_2d_rotate(a1));
            pax_push_2d(&gbuf);
            pax_apply_2d(&gbuf, matrix_2d_translate(1, 0));
            pax_apply_2d(&gbuf, matrix_2d_rotate(-a0 - a1 + M_PI * 0.5));
            pax_draw_tri(&gbuf, color1, 0.25f, 0.0f, -0.125f, 0.2165f, -0.125f, -0.2165f);
            pax_pop_2d(&gbuf);
            
            pax_pop_2d(&gbuf);
        }
        
        // Make an FPS counter.
        uint64_t spent = micros - lastMicros;
        lastMicros     = micros;
        float fps      = 1000000.0 / spent;
        
        char temp[32];
        snprintf(temp, 32, "%5.1f FPS\n", fps);
        // fputs(temp, stdout);
        pax_draw_text(&gbuf, 0xffffffff, pax_font_sky_mono, 36, 5, 5, temp);
        
        disp_flush();
    }
    
    // Reveal cursor.
    // fputs("\033[?25h", stdout);
    
    return 0;
}

char *dump_file(const char *path) {
    // Open file.
    FILE *fd = fopen(path, "r");
    if (!fd) return NULL;
    
    // Determine length.
    fseek(fd, 0, SEEK_END);
    long len = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    
    // Allocate memory.
    char *tmp = malloc(len+1);
    if (!tmp) {
        // Failed to allocate.
        fclose(fd);
        return NULL;
        
    } else {
        // Copy into buffer.
        len = fread(tmp, 1, len, fd);
        fclose(fd);
        tmp[len] = 0;
        return tmp;
        
    }
}

bool disp_make_buf(const char *fb_name) {
    // Open framebuffer file.
    fbdev = fopen(fb_name, "r+b");
    if (!fbdev) {
        perror("Cannot open framebuffer device");
        return false;
    }
    
    // Detect buffer type.
    struct fb_var_screeninfo vinfo;
    int res = ioctl(fbdev->_fileno, FBIOGET_VSCREENINFO, &vinfo);
    if (res) {
        perror("Cannot get framebuffer info");
        fclose(fbdev);
        return false;
    }
    
    // Memory map the framebuffer.
    fbsize = vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8);
    stride = vinfo.xres * (vinfo.bits_per_pixel / 8);
    
    fbmem = mmap(0, fbsize, PROT_READ | PROT_WRITE, MAP_SHARED, fbdev->_fileno, 0);
    if (!fbmem) {
        perror("Cannot memory map framebuffer");
        fclose(fbdev);
        return false;
    }
    
    // Create a PAX buffer with it.
    pax_buf_init(&gbuf, NULL, vinfo.xres, vinfo.yres, PAX_BUF_32_8888ARGB);
    
    if (pax_last_error) {
        fprintf(stderr, "Cannot initialise graphics context");
        return false;
    }
    
    return true;
    
}

void disp_flush() {
    pax_join();
    
    memcpy(fbmem, gbuf.buf, fbsize);
}

void disp_sync() {
    pax_join();
}

uint64_t milliTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

uint64_t microTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}
