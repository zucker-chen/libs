/******************************************************************************
 * Copyright (C) 2018-2020
 * file:    jpg_rgb_trans.h
 * author:  zucker.chen
 * created: 2019-10-10 18:00
 * updated: 2018-10-10 18:00
 ******************************************************************************/
#ifndef JPEG_ADDOSD_H
#define JPEG_ADDOSD_H


#ifdef __cplusplus
extern "C" {
#endif


typedef struct jbt_rgb_info {
    int                w;          // width, unit is pixel.
    int                h;          // heigh, unit is pixel.
    int                wstride;    // width lengh, unit is byte.
    unsigned char      *data;      // rgb data.
} jbt_rgb_info_t;


// input:file(jpeg file handle), rgb_info, nQuality
// output: disk file
int jbt_rgb2jpeg(FILE *file, jbt_rgb_info_t *rgb_info, int quality);

// input:file(jpeg file handle)
// output: rgb_info
int jbt_jpeg2rgb(FILE *file, jbt_rgb_info_t *rgb_info);




#ifdef __cplusplus
}
#endif
#endif
