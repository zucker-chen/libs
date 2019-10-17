#include <stdio.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "ttf_osd_text.h"



int sdl_ttf_test()
{
    char * pstr = "1你2Heelo";
    SDL_PixelFormat *fmt;
    TTF_Font *font;  
    SDL_Surface *text, *temp;  

    if (TTF_Init() < 0 ) 
    {  
        fprintf(stderr, "Couldn't initialize TTF: %s\n", SDL_GetError());  
        SDL_Quit();
    }  

    font = TTF_OpenFont("./font.ttf", 48); 
    if ( font == NULL ) 
    {  
        fprintf(stderr, "Couldn't load %d pt font from %s: %s\n", 18, "ptsize", SDL_GetError());  
    }  
    SDL_Color forecol = { 0xff, 0x00, 0x00, 0xff };  
    text = TTF_RenderUTF8_Solid(font, pstr, forecol);
    
    #if 0   // outline轮廓开关
    int outline_size = 1;
    TTF_Font *font_outline; 
    SDL_Color backcol = { 0xff, 0xff, 0xff, 0xff };  
    font_outline = TTF_OpenFont("./font.ttf", 72); 
    TTF_SetFontOutline(font_outline, outline_size);
    SDL_Surface *outline_surface = TTF_RenderUTF8_Blended(font_outline, pstr, backcol);     // 不能用Solid和Shaded,不然后面无法透明叠加
    SDL_Rect rect = {outline_size, outline_size, outline_surface->w, outline_surface->h};   // 要进行偏移(outline_size,outline_size)，因为outline会扩大字体面积
    SDL_SetSurfaceBlendMode(outline_surface, SDL_BLENDMODE_BLEND);
    SDL_BlitSurface(outline_surface, &rect, text, NULL); 
    #endif 

    #if 1
    int w = 0, h = 0;
    TTF_SizeUTF8(font, pstr, &w, &h);
    printf("str = %s, pixel w = %d, h = %d\n", pstr, w, h);
    #endif
    
    fmt = (SDL_PixelFormat*)malloc(sizeof(SDL_PixelFormat));
    memset(fmt,0,sizeof(SDL_PixelFormat));
    fmt->BitsPerPixel = 16;
    fmt->BytesPerPixel = 2;
    //fmt->colorkey = 0xffffffff;
    //fmt->alpha = 0xff;

    temp = SDL_ConvertSurface(text,fmt,0);
    SDL_SaveBMP(temp, "save.bmp"); 
    printf("SDL_ConvertSurface pitch = %d, h = %d\n", temp->pitch, temp->h);

    free(fmt);
    SDL_FreeSurface(text);  
    SDL_FreeSurface(temp);
    TTF_CloseFont(font);  
    TTF_Quit(); 
    
    return 0;
}

int ttf_osd_text_test()
{
    tot_ctx_t *ctx;
    tot_bitmap_info_t out;
    int ret;
    
    ctx = tot_open("./font.ttf", 72);
    if (NULL == ctx) {
        printf("tot_open error!\n");
        return -1;
    }
    
    tot_pixel_format_set(ctx, TOT_PIXEL_BGR888);
    tot_color_set(ctx, 0xef, 0x15, 0xdf);
    tot_outline_set(ctx, 1);
    
    ret = tot_str2bitmap(ctx, "Hello 你好 World!", &out);
    if (0 > ret) {
        printf("tot_str2bitmap error!\n");
        return -1;
    }
    printf("bitmap info: w = %d, h = %d, wstride = %d, pbytes = %d\n", out.w, out.h, out.wstride, out.pbytes);
    
    if (1)  // text overlay
    {
        tot_ctx_t *ctx_osd;
        tot_bitmap_info_t osd_out;

        ctx_osd = tot_open("./font.ttf", 32);
        tot_pixel_format_set(ctx_osd, TOT_PIXEL_BGR888);
        tot_color_set(ctx_osd, 0x2, 0x1, 0xee);
        tot_outline_set(ctx_osd, 0);
        tot_str2bitmap(ctx_osd, "321", &osd_out);
        printf("bitmap info: w = %d, h = %d, wstride = %d, pbytes = %d\n", osd_out.w, osd_out.h, osd_out.wstride, osd_out.pbytes);
        tot_draw_text(&out, 0, 2, &osd_out);
        if (0 > ret) {
            printf("tot_draw_text error!\n");
            return -1;
        }
        tot_save_bmp(ctx_osd, "3.bmp");
        tot_reset(ctx_osd);
        tot_close(ctx_osd);
    }
    
    
    
    ret = tot_save_bmp(ctx, "2.bmp");
    if (0 > ret) {
        printf("tot_save_bmp error!\n");
        return -1;
    }
    tot_reset(ctx);
    tot_close(ctx);

    return 0;
}






int main(int argc, const char *argv[])
{
    ttf_osd_text_test();

    return 0;
}
