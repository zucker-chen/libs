#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "jpeglib.h"
#include "jpg_img_trans.h"
#include "libbmp.h"		/* Common decls for cjpeg/djpeg applications */


typedef struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
} *my_error_ptr;

static void my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}



// input:           rgb_info, nQuality
// output:          file(jpeg file handle)
// rgb_info->data:  rgb, byte0=r, byte1=g, byte2=b
int jit_rgb2jpeg(FILE *file, jit_img_info_t *rgb_info, int quality)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];        /* pointer to JSAMPLE row[s] */
    int     row_stride;             /* physical row width in image buffer */
    
    cinfo.err = jpeg_std_error(&jerr);
    /* Step 1: allocate and initialize JPEG compression object */
    /* Now we can initialize the JPEG compression object. */
    jpeg_create_compress(&cinfo);
    
    /* Step 2: specify data destination (eg, a file) */
    jpeg_stdio_dest(&cinfo, file);

    /* Step 3: set parameters for compression */
    /* First we supply a description of the input image.
     * Four fields of the cinfo struct must be filled in:
     */
    cinfo.image_width = rgb_info->w; 	/* image width and height, in pixels */
    cinfo.image_height = rgb_info->h;
    cinfo.input_components = 3;		/* # of color components per pixel */
    cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

    /* Step 4: Start compressor */
    jpeg_start_compress(&cinfo, TRUE);

    /* Step 5: while (scan lines remain to be written) */
    /*           jpeg_write_scanlines(...); */
    row_stride = rgb_info->wstride;	/* JSAMPLEs per row in image_buffer */
 
    while (cinfo.next_scanline < cinfo.image_height) {
        /* jpeg_write_scanlines expects an array of pointers to scanlines.
         * Here the array is only one element long, but you could pass
         * more than one scanline at a time if that's more convenient.
         */
        row_pointer[0] = &rgb_info->data[cinfo.next_scanline * row_stride];
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    /* Step 6: Finish compression */
    jpeg_finish_compress(&cinfo);

    /* Step 7: release JPEG compression object */
    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_compress(&cinfo);
    
    return 0;
}



// input:           file(jpeg file handle)
// output:          rgb_info
// rgb_info->data:  rgb, byte0=r, byte1=g, byte2=b
// rgb_info->data:  will be alloc mem, so need free it by jit_rgb_free()
int jit_jpeg2rgb(FILE *file, jit_img_info_t *rgb_info)
{
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    unsigned char *rgbdata;		/* Output rgb buffer */
    JSAMPARRAY buffer;		/* Output row buffer */
    int row_stride;		/* physical row width in output buffer */

    /* Step 1: allocate and initialize JPEG decompression object */
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    /* Establish the setjmp return context for my_error_exit to use. */
    if (setjmp(jerr.setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error.
         * We need to clean up the JPEG object, close the input file, and return.
         */
        jpeg_destroy_decompress(&cinfo);
        return -1;
    }
    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(&cinfo);

    /* Step 2: specify data source (eg, a file) */
    jpeg_stdio_src(&cinfo, file);

    /* Step 3: read file parameters with jpeg_read_header() */
    (void) jpeg_read_header(&cinfo, TRUE);
    
    /* Step 4: set parameters for decompression */


    /* Step 5: Start decompressor */
    (void) jpeg_start_decompress(&cinfo);
    
    /* we need to make an output work buffer of the right size.*/
    row_stride = cinfo.output_width * cinfo.output_components;
    /* Make a one-row-high sample array that will go away when done with image */
    rgbdata = malloc(row_stride * cinfo.output_height);
    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);    
    /* Step 6: while (scan lines remain to be read) */
    /*           jpeg_read_scanlines(...); */

    /* Here we use the library's state variable cinfo.output_scanline as the
     * loop counter, so that we don't have to keep track ourselves.
     */
    while (cinfo.output_scanline < cinfo.output_height) {
        /* jpeg_read_scanlines expects an array of pointers to scanlines.
         * Here the array is only one element long, but you could ask for
         * more than one scanline at a time if that's more convenient.
         */
        /* Here cinfo.output_scanline start at 0 */
        (void) jpeg_read_scanlines(&cinfo, buffer, 1);
        /* Assume put_scanline_someplace wants a pointer and sample count. */
        /* Here cinfo.output_scanline start at 1 */
        memcpy(&rgbdata[0] + ((cinfo.output_scanline-1) * row_stride), buffer[0], row_stride);
    }

    rgb_info->w = cinfo.output_width;
    rgb_info->h = cinfo.output_height;
    rgb_info->wstride = row_stride;
    rgb_info->data = rgbdata;
    
    /* Step 7: Finish decompression */
    (void) jpeg_finish_decompress(&cinfo);
    /* We can ignore the return value since suspension is not possible
     * with the stdio data source.
     */

    /* Step 8: Release JPEG decompression object */
    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_decompress(&cinfo);
    printf("======jpeg_file2rgb success=========\n");
   
    return 0;
}

// free rgb_info->data mem
int jit_rgb_free(jit_img_info_t *rgb_info)
{
    if (NULL != rgb_info->data) {
        free(rgb_info->data);
    } else {
        return -1;
    }
    
    return 0;
}


// input:           rgb_info
// output:          file(bmp file handle)
// rgb_info->data:  rgb, byte0=r, byte1=g, byte2=b
int jit_rgb2bmp(FILE *file, jit_img_info_t *rgb_info)
{
	bmp_img img;
    int ret, i, j;
    unsigned char *pixel;
    
	bmp_img_init_df (&img, rgb_info->w, rgb_info->h);
    for (i = 0; i < rgb_info->h; i++)
    {
        for (j = 0; j < rgb_info->w; j++)
        {
            pixel = rgb_info->data + (rgb_info->wstride * i) + j * 3;
            // rgb -> bgr
            bmp_pixel_init(&img.img_pixels[i][j], *(pixel + 0), *(pixel + 1), *(pixel + 2));
        }
    }
	ret = bmp_img_write(&img, file);
    if (ret < 0) {
        printf("bmp_header_init_df error !\n");
        return -1;
    }
	bmp_img_free (&img);
    
    return 0;
}


