
#include "main.h"

static int stride;
pax_buf_t gbuf;
static FILE *fbdev;

int main(int argc, char **argv) {
    if (disp_make_buf("fb0")) {
    
        pax_background(&gbuf, 0xff007fff);
        pax_draw_circle(&gbuf, 0xffff0000, gbuf.width / 2, gbuf.height / 2, gbuf.height / 4);
        //disp_flush();
        
        fseek(fbdev, 0, SEEK_SET);
        char dummy[1024];
        memset(dummy, 0, sizeof(dummy));
        fwrite(dummy, 1, sizeof(dummy), fbdev);
        fflush(fbdev);
        
        while (1);
        
    } else {
        printf("Could not make the funny :(\n");
    }
    
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
    char temp[1024+strlen(fb_name)];
    
    // Read size file.
    snprintf(temp, sizeof(temp)-1, "/sys/class/graphics/%s/virtual_size", fb_name);
    char *str_size   = dump_file(temp);
    
    // Read stride file.
    snprintf(temp, sizeof(temp)-1, "/sys/class/graphics/%s/stride", fb_name);
    char *str_stride = dump_file(temp);
    
    // Make sure we have all of them.
    if (str_size && str_stride) {
        // Parse size.
        int width  = 0;
        int height = 0;
        sscanf(str_size, "%d,%d", &width, &height);
        
        // Parse stride.
        stride     = atoi(str_stride);
        
        // Validate data.
        if (width < 2 || height < 2 || stride < 2) {
            // Too bad, clean up.
            printf("Invalid format.\n");
            free(str_size);
            free(str_stride);
            return false;
            
        } else {
            // Good data; make PAX buffer.
            printf("Initialising buffer %dx%d with stride %d.\n", width, height, stride);
            pax_buf_init(&gbuf, NULL, width, height, PAX_BUF_32_8888ARGB);
            free(str_size);
            free(str_stride);
            
            if (!pax_last_error) {
                // Open device.
                snprintf(temp, sizeof(temp)-1, "/dev/%s", fb_name);
                fbdev = fopen(temp, "rb");
                if (fbdev != NULL) {
                    return true;
                } else {
                    printf("Error opening framebuffer device.\n");
                    return false;
                }
                
            } else {
                // PAX error.
                printf("Error in PAX buffer init.\n");
                return false;
            }
        }
        
    } else {
        // Too bad, clean up.
        if (str_size)   free(str_size);
        if (str_stride) free(str_stride);
        printf("Error loading descriptions.\n");
        return false;
    }
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
