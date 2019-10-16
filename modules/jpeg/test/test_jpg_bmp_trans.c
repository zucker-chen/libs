#include <stdio.h>
#include <unistd.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "jpg_bmp_trans.h"

int main(int argc, const char *argv[])
{
    FILE *input_file, *output_file, *out_bmp;		/* source file */
    jbt_rgb_info_t rgb_info;

    if ((input_file = fopen("1.jpg", "rb")) == NULL) {
        fprintf(stderr, "can't open %s\n", "1.jpg");
        return 0;
    }
    if ((output_file = fopen("2.jpg", "wb")) == NULL) {
        fprintf(stderr, "can't open %s\n", "2.jpg");
        return 0;
    }
    if ((out_bmp = fopen("1.bmp", "wb")) == NULL) {
        fprintf(stderr, "can't open %s\n", "1.bmp");
        return 0;
    }

    jbt_jpeg2rgb(input_file, &rgb_info);
    printf("w = %d, h = %d, wstride = %d\n", rgb_info.w, rgb_info.h, rgb_info.wstride);

    
    jbt_rgb2jpeg(output_file, &rgb_info, 98);
    
    fclose(out_bmp);
    fclose(output_file);
    fclose(input_file);

    return 0;
}
