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

typedef struct tot_bitmap_info {
    int                w;          // width, unit is pixel.
    int                h;          // heigh, unit is pixel.
    int                pitch;      // lengh, unit is byte.
    void               *data;      // bitmap data.
} tot_bitmap_info_t;


typedef enum {
	TOT_PIXEL_UNKNOWN,		/* error/unspecified */
	TOT_PIXEL_ARGB8888,		/* ARGB 8888 */
	TOT_PIXEL_ARGB1555,		/* ARGB = 1555 */
	TOT_PIXEL_RGB888,		/* RGB = 888 default*/
	TOT_PIXEL_RGB565,		/* RGB = 565 */
} TOT_PIXEL_FORMAT_E;




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


#ifdef __cplusplus
}
#endif
#endif
