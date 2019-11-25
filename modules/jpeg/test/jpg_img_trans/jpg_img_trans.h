/******************************************************************************
 * Copyright (C) 2018-2020
 * file:    jpg_img_trans.h
 * author:  zucker.chen
 * created: 2019-10-10 18:00
 * updated: 2018-10-10 18:00
 ******************************************************************************/
#ifndef JPEG_ADDOSD_H
#define JPEG_ADDOSD_H


#ifdef __cplusplus
extern "C" {
#endif


typedef struct jit_img_info {
    int                w;          // width, unit is pixel.
    int                h;          // heigh, unit is pixel.
    int                wstride;    // width lengh, unit is byte.
    unsigned char      *data;      // rgb data.
} jit_img_info_t;


// input:           rgb_info, nQuality
// output:          file(jpeg file handle)
// rgb_info->data:  rgb, byte0=r, byte1=g, byte2=b
int jit_rgb2jpeg(FILE *file, jit_img_info_t *rgb_info, int quality);

// input:           file(jpeg file handle)
// output:          rgb_info
// rgb_info->data:  rgb, byte0=r, byte1=g, byte2=b
// rgb_info->data:  will be alloc mem, so need free it by jit_rgb_free()
int jit_jpeg2rgb(FILE *file, jit_img_info_t *rgb_info);

// free rgb_info->data mem
int jit_rgb_free(jit_img_info_t *rgb_info);

// input:           rgb_info
// output:          file(bmp file handle)
// rgb_info->data:  rgb, byte0=r, byte1=g, byte2=b
int jit_rgb2bmp(FILE *file, jit_img_info_t *rgb_info);



#ifdef __cplusplus
}
#endif
#endif
