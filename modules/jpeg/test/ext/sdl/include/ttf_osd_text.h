/******************************************************************************
 * Copyright (C) 2018-2020
 * file:    ttf_osd_text.h
 * author:  zucker.chen
 * created: 2019-10-10 18:00
 * updated: 2018-10-10 18:00
 ******************************************************************************/
#ifndef TTF_OSD_TEXT_H
#define TTF_OSD_TEXT_H

#include "SDL.h"
#include "SDL_ttf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TOT = TTF OSD TEXT */
#define TOT_MAX_PATH_LENGTH 64


typedef enum {
	TOT_PIXEL_UNKNOWN,		/* error/unspecified */
	TOT_PIXEL_ARGB8888,		/* ARGB8888 byte3 = A, byte2 = R, byte1 = G, byte0 = B*/
	TOT_PIXEL_BGRA8888,		/* BGRA8888 byte3 = B, byte2 = G, byte1 = R, byte0 = A*/
	TOT_PIXEL_ARGB1555,		/* ARGB = 1555 */
	TOT_PIXEL_BGRA5551,		/* BGRA = 5551 */
	TOT_PIXEL_RGB888,		/* RGB = 888 default*/
	TOT_PIXEL_BGR888,		/* BGR = 888 default*/
} TOT_PIXEL_FORMAT_E;

typedef struct tot_bitmap_info {
    int                w;          // width, unit is pixel.
    int                h;          // heigh, unit is pixel.
    int                pbytes;     // one pixel bytes.
    int                wstride;    // width lengh, unit is byte.
    TOT_PIXEL_FORMAT_E pixel_fomat;    // eg. RGB888
    unsigned char      *data;      // bitmap data.
} tot_bitmap_info_t;

typedef struct tot_ctx {
    char                    ttf_path[TOT_MAX_PATH_LENGTH];  // ttf font path
    TTF_Font                *font;          // ttf open handle point.
    int                     size;           // font size
    SDL_Color               color;          // RGB, color.r/b/g
    TOT_PIXEL_FORMAT_E      pixel_fomat;    // eg. RGB888
    SDL_Surface             *sdl_sf;        // internal use
    int                     is_outline;     // internal use, suggest font size >= 32
} tot_ctx_t;


tot_ctx_t *tot_open(const char *ttf_path, const int font_size);

void tot_pixel_format_set(tot_ctx_t *ctx, const TOT_PIXEL_FORMAT_E pixel_fomat);

void tot_color_set(tot_ctx_t *ctx, const unsigned char r, const unsigned char g, const unsigned char b);

// is_outline: 0 = no outline, 1 = outline
// only support ARGB format
void tot_outline_set(tot_ctx_t *ctx, const int is_outline);

int tot_str2bitmap(tot_ctx_t *ctx, char *str, tot_bitmap_info_t *out);

// free SDL_Surface object, must do it after tot_str2bitmap()
int tot_reset(tot_ctx_t *ctx);

// between tot_str2bitmap to tot_reset
int tot_save_bmp(tot_ctx_t *ctx, const char *bmp_path);

int tot_close(tot_ctx_t *ctx);

/*
 * 针对bitmap图片数据进行OSD文本信息叠加
 * obj(in&out), 你需要叠加的bitmap数据信息
 * x,y(in),     text叠加到obj的起始偏移坐标
 * text,        需要叠加文本bitmap信息
 * obj位图为RGB8888/BGR8888时text位图应为ARGB8888/BGRA8888
 */
int tot_draw_text(tot_bitmap_info_t *obj, int x, int y, tot_bitmap_info_t *text);
/*
 * 字符位图叠加格式支持
 *      text            --->          obj    
 * TOT_PIXEL_ARGB8888   --->    TOT_PIXEL_ARGB8888
 * TOT_PIXEL_ARGB8888   --->    TOT_PIXEL_RGB888
 * TOT_PIXEL_BGRA8888   --->    TOT_PIXEL_ARGB8888
 * TOT_PIXEL_BGRA8888   --->    TOT_PIXEL_BGR888
 * TOT_PIXEL_ARGB1555   --->    TOT_PIXEL_ARGB1555
 * TOT_PIXEL_BGRA5551   --->    TOT_PIXEL_BGRA5551
 */



#ifdef __cplusplus
}
#endif
#endif
