#include "hzk_font.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>


#define pri_dbg(format, args...) fprintf(stderr,"%s %d %s() " format, __FILE__, __LINE__, __func__, ## args)









int main (int argc, char *argv[])
{
	hzk_init();


	hzk_disp_attr_t hzk_ctx = {0};
	hzk_bitmap_info_t out = {0};
	
	hzk_ctx.pixel_fomat = BITMAP_ARGB1555;
	hzk_ctx.font_size = 16;
	hzk_ctx.color = 0xffff;
	memcpy(hzk_ctx.str, "A»Àbingƒ„S", 10);
	
	hzk_str2bitmap(&hzk_ctx, &out);	
	hzk_bitmap_print(&out);	
	hzk_bitmap_free(&out);
	
	hzk_deinit();
	
	return 0;
}

