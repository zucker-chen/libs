
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <time.h>


#include "zbar.h"
#include "wand/MagickWand.h"
#include "iconv.h"



int main (int argc, char *argv[])
{
	char file_name[64] = "./files/test.png";
	
	if (argc == 2) {
		stpcpy(file_name, argv[1]);
		printf("file name = %s\n", file_name);
	}

	MagickBooleanType status;
	MagickWand *magick_wand;
	 
	MagickWandGenesis();  
	magick_wand=NewMagickWand();  
	status=MagickReadImage(magick_wand,file_name);  
	if (status == MagickFalse) {
		char *description;
		ExceptionType severity;
		description=MagickGetException(magick_wand,&severity);
		description=(char *) MagickRelinquishMemory(description);
		return -1;
	}
	
	size_t width, height;  
	width = MagickGetImageWidth(magick_wand);  
	height = MagickGetImageHeight(magick_wand);  
	if (status == MagickFalse) {
		char *description;
		ExceptionType severity;
		description=MagickGetException(magick_wand,&severity);
		description=(char *) MagickRelinquishMemory(description);
		return -1;
	} else {  
		printf("width=%d, height=%d\n", width, height);  
	}  

	void *pixel = malloc(width*height);  
	//get gray pixel  
	status = MagickExportImagePixels(magick_wand, 0, 0, width, height, "I", CharPixel, pixel);  
	if (status == MagickFalse) {
		char *description;
		ExceptionType severity;
		description=MagickGetException(magick_wand,&severity);
		description=(char *) MagickRelinquishMemory(description);
		return -1;
	}  
	
	zbar_image_scanner_t *scanner = zbar_image_scanner_create();  
	/* configure the reader */  
	zbar_image_scanner_set_config(scanner, ZBAR_NONE, ZBAR_CFG_ENABLE, 1);  
	
	/* wrap image data */  
	zbar_image_t *image = zbar_image_create();  
	zbar_image_set_format(image, *(int*)"Y800");  
	zbar_image_set_size(image, width, height);  
	zbar_image_set_data(image, pixel, width*height, zbar_image_free_data); 

	/* scan the image for barcodes */  
	int n = zbar_scan_image(scanner, image); //n == 0 is failed  

	if (n > 0) {  
		/* extract results */  
		const zbar_symbol_t *symbol = zbar_image_first_symbol(image);  
		for(; symbol; symbol = zbar_symbol_next(symbol))  
		{  
			/* do something useful with results */  
			zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);  

			const char *data = zbar_symbol_get_data(symbol);  
			printf("decoded %s symbol \"%s\"\n", zbar_get_symbol_name(typ), data);  
		}  
	} else {  
		printf("zbar_scan_image return %d, failed\n", n);  
	}  

	free(pixel);  

	magick_wand=DestroyMagickWand(magick_wand);  
	MagickWandTerminus();

	return 0;	
}
