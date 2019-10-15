#include <stdio.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "ttf_osd_text.h"


tot_ctx_t *tot_open(const char *ttf_path, const int font_size)
{
    tot_ctx_t *ctx;
    
    ctx = malloc(sizeof(tot_ctx_t));
    if (NULL == ctx) {
        return NULL;
    }
    
    // init SDL_Surface
    ctx->sdl_sf = NULL;
    

    if (TTF_Init() < 0 ) 
    {  
        fprintf(stderr, "Couldn't initialize TTF: %s\n", SDL_GetError());  
        SDL_Quit();
        return NULL;
   }  

    ctx->size = font_size;
    strncpy(ctx->ttf_path, ttf_path, TOT_MAX_PATH_LENGTH);
    ctx->font = TTF_OpenFont(ctx->ttf_path, ctx->size);
    if (NULL == ctx->font) {
        SDL_Quit();
        return NULL;
    }

    ctx->color.r = 0xff;
    ctx->color.g = 0xff;
    ctx->color.b = 0xff;  
    ctx->pixel_fomat = TOT_PIXEL_RGB888;
    
    return ctx;
}

void tot_pixel_format_set(tot_ctx_t *ctx, const TOT_PIXEL_FORMAT_E pixel_fomat)
{
    ctx->pixel_fomat = pixel_fomat;
}

void tot_color_set(tot_ctx_t *ctx, const unsigned char r, const unsigned char g, const unsigned char b)
{
    ctx->color.r = r;
    ctx->color.g = g;
    ctx->color.b = b;  

    return 0;
}

// is_outline: 0 = no outline, 1 = outline
// only support ARGB format
void tot_outline_set(tot_ctx_t *ctx, const int is_outline)
{
    ctx->is_outline = is_outline;
}

int tot_str2bitmap(tot_ctx_t *ctx, char *str, tot_bitmap_info_t *out)
{
    int pixel_fmt;
    SDL_Surface *tmp_sf;
    
    
    if (ctx->pixel_fomat == TOT_PIXEL_RGB888 || ctx->pixel_fomat == TOT_PIXEL_RGB565) {
        tmp_sf = TTF_RenderUTF8_Solid(ctx->font, str, ctx->color); 
    } else if (ctx->pixel_fomat == TOT_PIXEL_ARGB8888 || ctx->pixel_fomat == TOT_PIXEL_ARGB1555){
        tmp_sf = TTF_RenderUTF8_Blended(ctx->font, str, ctx->color); 
    }
    if (NULL == tmp_sf) {
        return -1;
    }
    
    if (ctx->is_outline == 1 && (ctx->pixel_fomat == TOT_PIXEL_ARGB8888 || ctx->pixel_fomat == TOT_PIXEL_ARGB1555)) {
        int outline_size = 1;
        TTF_Font *font; 
        SDL_Surface *sf;
        SDL_Color backcol = {0x0, 0x0, 0x0};  
        
        font = TTF_OpenFont(ctx->ttf_path, ctx->size); 
        TTF_SetFontOutline(font, outline_size);
        sf = TTF_RenderUTF8_Blended(font, str, backcol);                // 不能用Solid和Shaded,不然后面无法透明叠加
        SDL_Rect rect = {outline_size, outline_size, sf->w, sf->h};     // 要进行偏移(outline_size,outline_size)，因为outline会扩大字体面积
        SDL_SetSurfaceBlendMode(sf, SDL_BLENDMODE_BLEND);               // SDL_BLENDMODE_ADD, SDL_BLENDMODE_BLEND, SDL_BLENDMODE_MOD
        SDL_BlitSurface(sf, &rect, tmp_sf, NULL);
        //SDL_SaveBMP(sf, "1.bmp");    // for test.
        
        free(sf);
        TTF_CloseFont(font);
    }
    
    if (ctx->pixel_fomat == TOT_PIXEL_ARGB8888) {
        pixel_fmt = SDL_PIXELFORMAT_ARGB8888;
    } else if (ctx->pixel_fomat == TOT_PIXEL_ARGB1555) {
        pixel_fmt = SDL_PIXELFORMAT_ARGB1555;
    } else if (ctx->pixel_fomat == TOT_PIXEL_RGB888) {
        pixel_fmt = SDL_PIXELFORMAT_RGB888;
    } else if (ctx->pixel_fomat == TOT_PIXEL_RGB565) {
        pixel_fmt = SDL_PIXELFORMAT_RGB565;
    } else {
        pixel_fmt = SDL_PIXELFORMAT_ARGB8888;
    }
   
    ctx->sdl_sf = SDL_ConvertSurfaceFormat(tmp_sf, pixel_fmt, 0);
    if (NULL == ctx->sdl_sf) {
        free(tmp_sf);
        return -1;
    }
    
    out->w = ctx->sdl_sf->w;
    out->h = ctx->sdl_sf->h;
    out->pitch = ctx->sdl_sf->pitch;
    out->data = ctx->sdl_sf->pixels;
    
    free(tmp_sf);

    return 0;
}

// between tot_str2bitmap to tot_reset
int tot_save_bmp(tot_ctx_t *ctx, const char *bmp_path)
{
    if (NULL == ctx->sdl_sf) {
    printf("==========sdl_sf========\n");
        return -1;
    }
    
    return SDL_SaveBMP(ctx->sdl_sf, bmp_path); 
}


// free SDL_Surface object, must do it after tot_str2bitmap()
int tot_reset(tot_ctx_t *ctx)
{
    if (NULL != ctx->sdl_sf) {
        free(ctx->sdl_sf);
    }
    
    return 0;
}


int tot_close(tot_ctx_t *ctx)
{
    if (NULL != ctx->font) {
        TTF_CloseFont(ctx->font);  
    }

    if (NULL != ctx) {
        free(ctx);
    }
    
    TTF_Quit();
    
    return 0;
}

/*
 * Note:
 * 1. TTF_RenderUTF8_Blended() 必须要设置ARGB格式，否则转换出来的bitmap不透明，显示不正常
 * 2. bitmap格式转换时有2个函数可用：
 *      SDL_ConvertSurface --> 需要制定具体像素格式位宽等信息；
 *      SDL_ConvertSurfaceFormat ---> 用像素格式的宏定义即可，如 SDL_PIXELFORMAT_ARGB8888
 */
