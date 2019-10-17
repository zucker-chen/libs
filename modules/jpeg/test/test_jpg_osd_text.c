#include <stdio.h>
#include <unistd.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "jpg_bmp_trans.h"
#include "ttf_osd_text.h"

int main(int argc, const char *argv[])
{
    FILE *input_file, *output_file;		/* source file */
    jbt_rgb_info_t rgb_info;
    int ret;

    if ((input_file = fopen("1.jpg", "rb")) == NULL) {
        fprintf(stderr, "can't open %s\n", "1.jpg");
        return 0;
    }
    if ((output_file = fopen("2.jpg", "wb")) == NULL) {
        fprintf(stderr, "can't open %s\n", "2.jpg");
        return 0;
    }

    jbt_jpeg2rgb(input_file, &rgb_info);
    printf("w = %d, h = %d, wstride = %d\n", rgb_info.w, rgb_info.h, rgb_info.wstride);

    if (1)  // text overlay
    {
        tot_ctx_t *ctx_osd;
        tot_bitmap_info_t dest, osd_out;

        dest.w = rgb_info.w;
        dest.h = rgb_info.h;
        dest.wstride = rgb_info.wstride;
        dest.pbytes = 3;
        dest.pixel_fomat = TOT_PIXEL_BGR888;
        dest.data = rgb_info.data;
        
        ctx_osd = tot_open("./font.ttf", 32);
        tot_pixel_format_set(ctx_osd, TOT_PIXEL_BGR888);
        tot_color_set(ctx_osd, 0x0, 0x0, 0xEE);
        tot_outline_set(ctx_osd, 1);
        tot_str2bitmap(ctx_osd, "321", &osd_out);
        printf("bitmap info: w = %d, h = %d, wstride = %d\n", osd_out.w, osd_out.h, osd_out.wstride);
        ret = tot_draw_text(&dest, 20, 20, &osd_out);
        if (ret < 0) {
            printf("tot_draw_text error!\n");
        }
        tot_save_bmp(ctx_osd, "3.bmp");
        tot_reset(ctx_osd);
        tot_close(ctx_osd);
    }
    
    jbt_rgb2jpeg(output_file, &rgb_info, 98);
    
    fclose(output_file);
    fclose(input_file);

    return 0;
}

/*
 * Note:
 * 1. sdl出来的rgb数据顺序与jpeglib出来的rgb数据顺序描述相反：
 *    sdl出来的RGB888：byte0=b, byte1=g, byte2=r;
 *    jpeglib出来的RGB888：byte0=r, byte1=g, byte2=b
 */

