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
    ctx->color.a = 0x0; 
}

// pixels: 0 = no outline, 1 = 1 pixel outline
// only support ARGB format
void tot_outline_set(tot_ctx_t *ctx, const int pixels)
{
    ctx->outline_pixels = pixels;
}

int tot_str2bitmap(tot_ctx_t *ctx, char *str, tot_bitmap_info_t *out)
{
    SDL_PixelFormat fmt = {0};
    SDL_Surface     *tmp_sf;
    
    
    if (ctx->pixel_fomat == TOT_PIXEL_RGB888 || ctx->pixel_fomat == TOT_PIXEL_BGR888) {
        tmp_sf = TTF_RenderUTF8_Solid(ctx->font, str, ctx->color); 
    } else if (ctx->pixel_fomat == TOT_PIXEL_ARGB8888 || ctx->pixel_fomat == TOT_PIXEL_BGRA8888 
            || ctx->pixel_fomat == TOT_PIXEL_ARGB1555 || ctx->pixel_fomat == TOT_PIXEL_BGRA5551){
        tmp_sf = TTF_RenderUTF8_Blended(ctx->font, str, ctx->color); 
    } else {
        return -1;
    }
    if (NULL == tmp_sf) {
        return -1;
    }
    
    if (ctx->outline_pixels > 0 && (ctx->pixel_fomat == TOT_PIXEL_ARGB8888 || ctx->pixel_fomat == TOT_PIXEL_ARGB1555
                                 || ctx->pixel_fomat == TOT_PIXEL_BGRA8888 || ctx->pixel_fomat == TOT_PIXEL_BGRA5551)) {
        int outline_size = ctx->outline_pixels;
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
        fmt.BitsPerPixel = 32;
        fmt.BytesPerPixel = 4;
        fmt.Amask = 0xFF<<24;
        fmt.Rmask = 0xFF<<16;
        fmt.Gmask = 0xFF<<8;
        fmt.Bmask = 0xFF;
    } else if (ctx->pixel_fomat == TOT_PIXEL_BGRA8888) {
        fmt.BitsPerPixel = 32;
        fmt.BytesPerPixel = 4;
        fmt.Bmask = 0xFF<<24;
        fmt.Gmask = 0xFF<<16;
        fmt.Rmask = 0xFF<<8;
        fmt.Amask = 0xFF;
    } else if (ctx->pixel_fomat == TOT_PIXEL_ARGB1555) {
        fmt.BitsPerPixel = 16;
        fmt.BytesPerPixel = 2;
        fmt.Amask = 0x1<<15;
        fmt.Rmask = 0x1F<<10;
        fmt.Gmask = 0x1F<<5;
        fmt.Bmask = 0x1F;
    } else if (ctx->pixel_fomat == TOT_PIXEL_BGRA5551) {
        fmt.BitsPerPixel = 16;
        fmt.BytesPerPixel = 2;
        fmt.Bmask = 0x1F<<11;
        fmt.Gmask = 0x1F<<6;
        fmt.Rmask = 0x1F<<1;
        fmt.Amask = 0x1;
    } else if (ctx->pixel_fomat == TOT_PIXEL_RGB888) {
        fmt.BitsPerPixel = 24;
        fmt.BytesPerPixel = 3;
        fmt.Rmask = 0xFF<<16;
        fmt.Gmask = 0xFF<<8;
        fmt.Bmask = 0xFF;
    } else if (ctx->pixel_fomat == TOT_PIXEL_BGR888) {
        fmt.BitsPerPixel = 24;
        fmt.BytesPerPixel = 3;
        fmt.Bmask = 0xFF<<16;
        fmt.Gmask = 0xFF<<8;
        fmt.Rmask = 0xFF;
    } else {
        fmt.BitsPerPixel = 32;
        fmt.BytesPerPixel = 4;
        fmt.Amask = 0xFF<<24;
        fmt.Rmask = 0xFF<<16;
        fmt.Gmask = 0xFF<<8;
        fmt.Bmask = 0xFF;
    }
   
    //ctx->sdl_sf = SDL_ConvertSurfaceFormat(tmp_sf, SDL_PIXELFORMAT_ARGB8888, 0);
	ctx->sdl_sf = SDL_ConvertSurface(tmp_sf, &fmt, 0);
    
    if (NULL == ctx->sdl_sf) {
        free(tmp_sf);
        return -1;
    }
    
    out->w = ctx->sdl_sf->w;
    out->h = ctx->sdl_sf->h;
    out->wstride = ctx->sdl_sf->pitch;
    out->data = ctx->sdl_sf->pixels;
    out->pixel_fomat = ctx->pixel_fomat;
    out->pbytes = ctx->sdl_sf->format->BytesPerPixel;
    
    free(tmp_sf);

    return 0;
}

// between tot_str2bitmap to tot_bitmap_free
int tot_save_bmp(tot_ctx_t *ctx, const char *bmp_path)
{
    if (NULL == ctx->sdl_sf) {
        printf("==========sdl_sf========\n");
        return -1;
    }
    
    return SDL_SaveBMP(ctx->sdl_sf, bmp_path); 
}


// free SDL_Surface object, must do it after tot_str2bitmap()
int tot_bitmap_free(tot_ctx_t *ctx)
{
    if (NULL != ctx->sdl_sf) {
        free(ctx->sdl_sf);
        ctx->sdl_sf = NULL;
    }
    
    return 0;
}


int tot_close(tot_ctx_t *ctx)
{
    if (NULL != ctx->font) {
        TTF_CloseFont(ctx->font);  
    }
    
    tot_bitmap_free(ctx);

    if (NULL != ctx) {
        free(ctx);
    }
    
    TTF_Quit();
    
    return 0;
}

/*
 * 针对bitmap图片数据进行OSD文本信息叠加
 * obj(in&out), 你需要叠加的bitmap数据信息
 * x,y(in),     text叠加到obj的起始偏移坐标
 * text,        需要叠加文本bitmap信息
 * obj与text位图格式必须相同
 */
int tot_draw_text(tot_bitmap_info_t *obj, int x, int y, tot_bitmap_info_t *text)
{

    unsigned char *p, *q;
    int pos, i;
    
    if (obj->pixel_fomat != text->pixel_fomat) {
        return -1;
    }
    
    //p = (unsigned char *)obj->data;
    for (pos = 0; pos < text->h; pos++)
    {
        p = obj->data + (obj->pbytes * x) + (obj->wstride * (y + pos));
        q = text->data + (text->wstride * pos);
        
        switch(text->pixel_fomat)
        {
			case TOT_PIXEL_ARGB8888:
            {
                for (i = 0; i < text->w; i++)
                {
                    if (*(q + (i * text->pbytes) + 3) != 0x0) {    // apha
                        memcpy(p + (i * obj->pbytes), q + (i * text->pbytes), obj->pbytes);
                    }
                }
                break;
            }
			case TOT_PIXEL_BGRA8888:
            {
                for (i = 0; i < text->w; i++)
                {
                    if (*(q + (i * text->pbytes)) != 0x0) {    // apha
                        if (obj->pixel_fomat == TOT_PIXEL_BGR888) {
                            // obj->data byte3 = apha, but text->data byte0 = apha
                            memcpy(p + (i * obj->pbytes), q + (i * text->pbytes) + 1, obj->pbytes - 1);
                        } else {
                            memcpy(p + (i * obj->pbytes), q + (i * text->pbytes), obj->pbytes);
                        }
                    }
                }
                break;
            }
			case TOT_PIXEL_RGB888:
			case TOT_PIXEL_BGR888:
            {
                for (i = 0; i < text->w; i++)
                {
                    memcpy(p + (i * obj->pbytes), q + (i * text->pbytes), obj->pbytes);
                }
                break;
            }
			case TOT_PIXEL_ARGB1555:
            {
                for (i = 0; i < text->w; i++)
                {
                    if ((*(q + (i * text->pbytes) + 1) & 0x80) != 0x0) {    // apha
                        memcpy(p + (i * obj->pbytes), q + (i * text->pbytes), obj->pbytes);
                    }
                }
                break;
            }
			case TOT_PIXEL_BGRA5551:
            {
                for (i = 0; i < text->w; i++)
                {
                    if ((*(q + (i * text->pbytes)) & 0x1) != 0x0) {    // apha
                        memcpy(p + (i * obj->pbytes), q + (i * text->pbytes), obj->pbytes);
                    }
                }
                break;
            }
            default:
            {
                return -1;
            }
        }
    }
    
    return 0;
}




/*
 * Note:
 * 1. TTF_RenderUTF8_Blended() 必须要设置ARGB格式，否则转换出来的bitmap不透明，显示不正常
 * 2. bitmap格式转换时有2个函数可用：
 *      SDL_ConvertSurface --> 需要制定具体像素格式位宽等信息；
 *      SDL_ConvertSurfaceFormat ---> 用像素格式的宏定义即可，如 SDL_PIXELFORMAT_ARGB8888
 * 3. 如果用SDL_ConvertSurfaceFormat接口，会有以下特点：
 *      3.1. TOT_PIXEL_RGB888，类RGB格式输出的bitmap点整数据，单像素仍然占4byte
 *      3.2. TOT_PIXEL_RGB888/TOT_PIXEL_BGR888，对应的apha位为byte3，但是apha值固定为0x0
 *      3.3. 字符叠加没法基于RGB888/BGR888叠加，因为没法判断透明像素点
 * 4. 进行像素叠加时需要判断叠加信息该像素apha是否为0（透明），如果为透明像素则不进行像素赋值，不然会导致叠加的内容不是透明叠加
 */
