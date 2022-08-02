
#include "main.h"

pax_buf_t       gbuf;
static FILE    *fbdev;
static uint8_t *fbmem;
static size_t   fbsize;
static int      stride;

int main(int argc, char **argv) {
    
    if (!disp_make_buf("/dev/fb0")) return 1;
    
    // Hide cursor (so it doesn't overwrite the framebuffer).
    fputs("\033[?25l", stdout);
    fflush(stdout);
    
    // Dummy thing in the background.
    pax_background(&gbuf, 0xff007fff);
    pax_draw_circle(&gbuf, 0xffff0000, gbuf.width / 2, gbuf.height / 2, gbuf.height / 4);
    
    // Wait for enter press.
    char dummy;
    fread(&dummy, 1, 1, stdin);
    
    // Reveal cursor.
    fputs("\033[?25h", stdout);
    
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
    pax_buf_init(&gbuf, fbmem, vinfo.xres, vinfo.yres, PAX_BUF_32_8888ARGB);
    
    if (pax_last_error) {
        fprintf(stderr, "Cannot initialise graphics context");
        return false;
    }
    
    return true;
    
}

void disp_flush() {
    pax_join();
    
    size_t pax_index = 0;
    size_t dev_delta = stride - gbuf.width * 4;
    
    union {
        pax_col_t raw;
        struct {
            uint8_t red;
            uint8_t green;
            uint8_t blue;
            uint8_t alpha;
        };
        uint8_t   arr[4];
    } pixel;
    const uint8_t dummy = 0;
    
    fseek(fbdev, 0, SEEK_SET);
    for (int y = 0; y < gbuf.height; y++) {
        for (int x = 0; x < gbuf.width; x++, pax_index++) {
            // Convert 32bpp ARGB...
            pixel.raw = gbuf.buf_32bpp[pax_index];
            // To 32bpp BGRx...
            fwrite(&pixel.blue,  1, 1, fbdev);
            fwrite(&pixel.green, 1, 1, fbdev);
            fwrite(&pixel.red,   1, 1, fbdev);
            fwrite(&dummy,       1, 1, fbdev);
        }
        // Seek with the STRIDE.
        fseek(fbdev, dev_delta, SEEK_CUR);
    }
    fflush(fbdev);
}

void disp_sync() {
    pax_join();
}
