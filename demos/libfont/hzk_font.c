#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "hzk_font.h"


typedef enum {
	HZK_FONTSIZE_HZ16 = 0,
	HZK_FONTSIZE_EN16 = 1,
	HZK_FONTSIZE_HZ12 = 2,
	HZK_FONTSIZE_EN12 = 3,
	HZK_FONTSIZE_MAX = 4,
} HZK_FONTSIZE_TYPE_E;


typedef struct {
	HZK_FONTSIZE_TYPE_E	id;
	int					langue;		// 0 = hz, 1 = en
    int            		w;          // width, unit is pixel.
    int            		h;          // heigh, unit is pixel.
	int					font_size;	// 
    char 				*filename;  // zk filename.
} hzk_zkfile_list_t;

typedef struct hzk_font_s {
    int 				w;
    int 				h;
	int					data_size;
	char				data[0];
} hzk_font_t;

typedef struct {
	hzk_font_t			*zk_font[HZK_FONTSIZE_MAX];	// 有4个字库，分别是2个字号的中英文字库
} hzk_ctx_t;

static hzk_ctx_t 	g_hzk_ctx = {0};
static hzk_zkfile_list_t g_hzk_file_lists[HZK_FONTSIZE_MAX] = {
	{HZK_FONTSIZE_HZ16,	0, 	16, 	16, 	16,	HZK_FILE_PATH"hz_16_16_12.hzk"},
	{HZK_FONTSIZE_EN16,	1, 	8, 		16, 	16,	HZK_FILE_PATH"en_8_16_12.hzk"},
	{HZK_FONTSIZE_HZ12,	0, 	12, 	12, 	12,	HZK_FILE_PATH"hz_12_12_9.hzk"},
	{HZK_FONTSIZE_EN12,	1, 	6, 		12, 	12,	HZK_FILE_PATH"en_6_12_9.hzk"},
};




// 通过汉字区位码查找字库获取点阵数据
static int hzk_qw2bytes(hzk_font_t *hzk, int qu, int wei, char *out)
{
	int font_bytes = 0;
	int font_pos = 0;
	
	font_bytes = hzk->h * ((hzk->w + 7) / 8);
	font_pos = ((qu - 1) * 94 + wei - 1) * font_bytes;
	if (font_pos > hzk->data_size) {
		printf("%s:%d Error.\n", __FUNCTION__, __LINE__);
		return -1;
	}
	memcpy(out, hzk->data + font_pos, font_bytes);

	#if 0	// print test
	printf("%s:%d w = %d, h = %d\n", __FUNCTION__, __LINE__, hzk->w, hzk->h);
    int j, k;
	int stride;
	char *pdata;
	pdata = out;
	stride = (hzk->w + 7) / 8;
    for(j = 0; j < hzk->h; j++)
    {
        for(k = 0; k < hzk->w; k++)
        {
			pdata = out + j * stride + k / 8;
			// 位与运算注意()不能省
			// HZK16的BYTE数据是大端格式，即bit0对应第7像素点
			if(((*pdata) & (0x80 >> k%8) & 0xff) != 0) {
				printf("*");
			} else {
				printf(" ");
			}
        }
        printf("\n");
    }
	#endif
	
	return 0;
}

static int hzk_ascii2bytes(hzk_font_t *hzk, char ascii, char *out)
{
	int font_bytes = 0;
	int font_pos = 0;
	
	font_bytes = hzk->h * ((hzk->w + 7) / 8);
	font_pos = ((int)ascii&0xff) * font_bytes;
	if (font_pos > hzk->data_size) {
		printf("%s:%d Error. font_pos = %d, data_size = %d\n", __FUNCTION__, __LINE__, font_pos, hzk->data_size);
		return -1;
	}
	memcpy(out, hzk->data + font_pos, font_bytes);

	#if 0	// print test
	printf("%s:%d w = %d, h = %d\n", __FUNCTION__, __LINE__, hzk->w, hzk->h);
    int j, k;
	int stride;
	char *pdata;
	pdata = out;
	stride = (hzk->w + 7) / 8;
    for(j = 0; j < hzk->h; j++)
    {
        for(k = 0; k < hzk->w; k++)
        {
			pdata = out + j * stride + k / 8;
			// 位与运算注意()不能省
			// HZK16的BYTE数据是大端格式，即bit0对应第7像素点
			if(((*pdata) & (0x80 >> k%8) & 0xff) != 0) {
				printf("*");
			} else {
				printf(" ");
			}
        }
        printf("\n");
    }
	#endif	
	
	return 0;
}


/* 将单个汉字或英文字符转换成点阵数据, 再转成单色图像bitmap信息输出
 * font_size 仅支持12/16的整数倍大小
 * 如果字号比已有字库大则会进行放大，放大是向右下方方向像素放大
 */
static int hzk_word2bytes(char word[2], int font_size, hzk_bitmap_info_t *out)
{
	hzk_font_t *hf = NULL;
	char wbitmap[48*6] = {0};	// 按最大字库为48x48分配数组大小
	int hzk_size = 0;			// 所用字库大小
	int hzk_zoom_out = 0;		// 需要放大的倍数
	int qu, wei;
	int ret, i;

	if (font_size % 16 == 0) {
		hzk_size = 16;
		hzk_zoom_out = font_size / hzk_size;
	} else if (font_size % 12 == 0) {
		hzk_size = 12;
		hzk_zoom_out = font_size / hzk_size;
	} else {
		hzk_size = 16;
		hzk_zoom_out = 1;
	}
	
	if ((word[0] & 0x80) != 0) {		// 汉字
		// 根据字号选择使用的字库
		for (i = 0; i < HZK_FONTSIZE_MAX; i++)
		{
			if (g_hzk_file_lists[i].langue == 0 && g_hzk_file_lists[i].font_size == hzk_size) {
				hf = g_hzk_ctx.zk_font[g_hzk_file_lists[i].id];
			}
		}
		if (hf == NULL) {
			printf("%s:%d fontsize = %d hzkfile not found, used default 16hzk.\n", __FUNCTION__, __LINE__, hzk_size);
			hf = g_hzk_ctx.zk_font[HZK_FONTSIZE_HZ16];
		}
		qu = (int)((word[0]&0xff) - 160);
		wei = (int)((word[1]&0xff) - 160);
		//printf("%s:%d qu = %d, wei = %d\n", __FUNCTION__, __LINE__, qu, wei);
		ret = hzk_qw2bytes(hf, qu, wei, wbitmap);
		if (ret < 0) {
			printf("%s:%d Error.\n", __FUNCTION__, __LINE__);
			return -1;
		}
	} else {							// 英文字符
		// 根据字号选择使用的字库
		for (i = 0; i < HZK_FONTSIZE_MAX; i++)
		{
			if (g_hzk_file_lists[i].langue == 1 && g_hzk_file_lists[i].font_size == hzk_size) {
				hf = g_hzk_ctx.zk_font[g_hzk_file_lists[i].id];
			}
		}
		if (hf == NULL) {
			printf("%s:%d fontsize = %d hzkfile not found, used default 16hzk.\n", __FUNCTION__, __LINE__, hzk_size);
			hf = g_hzk_ctx.zk_font[HZK_FONTSIZE_EN16];
		}
		ret = hzk_ascii2bytes(hf, word[0], wbitmap);
		if (ret < 0) {
			printf("%s:%d Error.\n", __FUNCTION__, __LINE__);
			return -1;
		}
	}
	
	// 将bitmap转换成单通道(Y)数据
	out->w = hf->w * hzk_zoom_out + (hzk_zoom_out - 1);
	out->h = hf->h * hzk_zoom_out + (hzk_zoom_out - 1);
	out->pixel_fomat = BITMAP_GRAY;
	out->pbytes = 1;
	out->wstride = out->w;
	out->color = 0xffffffff;

    int j, k, l, m;
	int stride;
	char *pdata, *dest_data;
	stride = (hf->w + 7) / 8;		// 获取字库每个字符的长度bytes单位
    for(j = 0; j < hf->h; j++)
    {
        for(k = 0; k < hf->w; k++)
        {
			pdata = wbitmap + j * stride + k / 8;
			dest_data = out->data + j * hzk_zoom_out * out->wstride + k * hzk_zoom_out * out->pbytes;
			// 位与运算注意()不能省
			// HZK16的BYTE数据是大端格式，即bit0对应第7像素点
			if(((*pdata) & (0x80 >> k%8) & 0xff) != 0) {
				for(l = 0; l < hzk_zoom_out; l++)
				{
					for(m = 0; m < hzk_zoom_out; m++)
					{
						*(char *)(dest_data + l * out->wstride + m) = out->color & 0xff;
					}
				}
			} else {
				for(l = 0; l < hzk_zoom_out; l++)
				{
					for(m = 0; m < hzk_zoom_out; m++)
					{
						*(char *)(dest_data + l * out->wstride + m) = 0x0;
					}
				}
			}
        }
    }
	
	//printf("%s:%d\n", __FUNCTION__, __LINE__);
	
	return 0;
}


// 字符的点阵数据按bitmap格式添加, 对外只支持RGB格式一致的bitmap叠加
int hzk_bitmap_add(hzk_bitmap_info_t *in, int x, int y, hzk_bitmap_info_t *bitmap)
{
	int i, j;
	int wbitmap_stride;
	char *dest_data;
	char *bitmap_data;
	
	wbitmap_stride = bitmap->w * bitmap->pbytes;
	for (i = 0; i < bitmap->h; i++)
	{
		for (j = 0; j < bitmap->w; j++)
		{
			// 如ARGB1555, 每个像素占2byte (in->pbytes)
			dest_data = in->data + (x * in->pbytes) + y * (in->w * in->pbytes);
			dest_data = dest_data + (j * in->pbytes) + i * (in->w * in->pbytes);
			bitmap_data = bitmap->data + i * wbitmap_stride + j * bitmap->pbytes;
			if (bitmap->pixel_fomat == BITMAP_GRAY) {
				if (in->pbytes >= 1) {
					*(dest_data + 0) = (char)(*bitmap_data != 0x0 ? in->color & 0xff : 0x0);
				}
				if (in->pbytes >= 2) {
					*(dest_data + 1) = (char)(*bitmap_data != 0x0 ? (in->color >> 8) & 0xff : 0x0);
				}
				if (in->pbytes >= 3) {
					*(dest_data + 2) = (char)(*bitmap_data != 0x0 ? (in->color >> 16) & 0xff : 0x0);
				}
				if (in->pbytes >= 4) {
					*(dest_data + 3) = (char)(*bitmap_data != 0x0 ? (in->color >> 24) & 0xff : 0x0);
				}
			} else if (bitmap->pixel_fomat == in->pixel_fomat) {
				memcpy(dest_data, bitmap_data, in->pbytes);
			} else {
				printf("%s:%d Error.\n", __FUNCTION__, __LINE__);
				return -1;
			}
		}	
	}	

	return 0;
}


int hzk_bitmap_print(hzk_bitmap_info_t *in)
{
	int i, j;
	char *pdata;
	
	for (i = 0; i < in->h; i++)
	{
		for (j = 0; j < in->w; j++)
		{
			// 如ARGB1555, 每个像素占2byte (in->pbytes)
			pdata = in->data + (j * in->pbytes) + i * (in->w * in->pbytes);
			if(*(pdata + 0) == 0x0 && *(pdata + 1) == 0) {
				printf(" ");
			} else {
				printf("*");
			}			
		}	
        printf("\n");
	}	
	printf("%s:%d\n", __FUNCTION__, __LINE__);
	
	return 0;
}

// 计算字符串转后bitmap的w/h pixel单位
int hzk_calc_bitmap_size(char *str, int font_size, int *width, int *height)
{
	int len = 0;
	int i = 0;
	int w = 0, h = 0;
	int hzk_size = 0;			// 所用字库大小
	int hzk_zoom_out = 0;		// 需要放大的倍数

	if (font_size % 16 == 0) {
		hzk_size = 16;
		hzk_zoom_out = font_size / hzk_size;
	} else if (font_size % 12 == 0) {
		hzk_size = 12;
		hzk_zoom_out = font_size / hzk_size;
	} else {
		hzk_size = 16;
		hzk_zoom_out = 1;
	}
	
	h = hzk_size * hzk_zoom_out + (hzk_zoom_out - 1);
	len = strlen(str);
	for (i = 0; i < len; i++)
	{
		if ((str[i] & 0x80) != 0) {	// 汉字2字节
			w += hzk_size * hzk_zoom_out + (hzk_zoom_out - 1);
			i++;
		} else {
			w += (hzk_size / 2) * hzk_zoom_out + (hzk_zoom_out - 1);
		}
	}
	*width = w;
	*height = h;
	
	return 0;
}



int hzk_str2bitmap(hzk_disp_attr_t *disp, hzk_bitmap_info_t *out)
{
	hzk_bitmap_info_t 	bitmap;
	int 				i = 0;
	int 				len = 0;
	int 				ret = 0;
	int 				size = 0;
	int 				x = 0, y = 0, w = 0, h = 0;
	char 				*wbitmap = NULL;	// 缓存对应字符转换后的点阵数据, 单通道数据(1个像素占1byte)
	
	
	// 计算初始化bitmap信息及申请空间
	hzk_calc_bitmap_size(disp->str, disp->font_size, &out->w, &out->h);
	//printf("%s:%d str = %s, w = %d, h = %d\n", __FUNCTION__, __LINE__, disp->str, out->w, out->h);
	out->pixel_fomat 	= disp->pixel_fomat;
	out->pbytes 		= 2;	// for ARGB15555
	out->color 			= disp->color;
	out->wstride 		= out->w;
	if (out->data == NULL) {
		size = out->w * out->h * out->pbytes;
		out->data = calloc(1, size);
	} else {
		printf("%s:%d Error.\n", __FUNCTION__, __LINE__);
		return -1;
	}

	// 计算单个汉字转换成点阵数据后的大小, 汉字有编码问题，用两个英文字符代替
	hzk_calc_bitmap_size("AB", disp->font_size, &w, &h);
	wbitmap = calloc(1, w * h * 1);
	
	len = strlen(disp->str);
	for (i = 0; i < len; i++)
	{
		memset(&bitmap, 0, sizeof(hzk_bitmap_info_t));
		bitmap.data = wbitmap;
		ret = hzk_word2bytes(&disp->str[i], disp->font_size, &bitmap);
		if (ret < 0) {
			printf("%s:%d Error.\n", __FUNCTION__, __LINE__);
			free(wbitmap);
			return -1;
		}
		//
		ret = hzk_bitmap_add(out, x, y, &bitmap);
		if (ret < 0) {
			printf("%s:%d Error.\n", __FUNCTION__, __LINE__);
			free(wbitmap);
			return -1;
		}
		x += bitmap.w;
		y = 0;
		
		if ((disp->str[i] & 0x80) != 0) {		
			i++;			// 汉字2字节
		}
	}
	free(wbitmap);
	
	return 0;
}

// must do it after hzk_str2bitmap()
int hzk_bitmap_free(hzk_bitmap_info_t *bitmap)
{
	if (bitmap != NULL && bitmap->data !=NULL) {
		free(bitmap->data);
		memset(bitmap, 0, sizeof(hzk_bitmap_info_t));
	}
	
	return 0;
}


static hzk_font_t *hzk_file_load(char *file_name)
{
    FILE 		*fp = NULL;
	hzk_font_t 	*hf = NULL;
	int			file_size = 0;
	int			ret = 0;
	
	fp = fopen(file_name, "rb+");
	if(fp == NULL)
	{    
		printf("%s:%d fopen hzk file = %s failed\n", __FUNCTION__, __LINE__, file_name);
		return NULL;
	}    
	
	// 获取字库大小
    fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	// 分配空间存储字库数据
	hf = calloc(1, sizeof(hzk_font_t) + file_size);
	ret = fread(hf->data, 1, file_size, fp);
	if (ret != file_size) {
		printf("%s:%d fread hzk file = %s failed\n", __FUNCTION__, __LINE__, file_name);
		return NULL;
	}
	hf->data_size = file_size;
	
	// 通过字库名字规则解析出字库大小
	sscanf(file_name, "%*[^_]_%d_%d", &hf->w, &hf->h);
	printf("%s:%d %s, font w = %d, h = %d\n", __FUNCTION__, __LINE__, file_name, hf->w, hf->h);
	
	fclose(fp);
	
	return hf;
}


int hzk_init(void)
{
	char *filename = NULL;
	int i;
	
	for (i = 0; i < HZK_FONTSIZE_MAX; i++)
	{
		if (g_hzk_ctx.zk_font[g_hzk_file_lists[i].id] == NULL) {
			filename = g_hzk_file_lists[i].filename;
			if (filename == NULL) {
				return -1;
			}
			g_hzk_ctx.zk_font[g_hzk_file_lists[i].id] = hzk_file_load(filename);
			if (g_hzk_ctx.zk_font[g_hzk_file_lists[i].id] == NULL) {
				printf("%s:%d Error.\n", __FUNCTION__, __LINE__);
				return -1;
			}
		}
	}
	
	return 0;
}

int hzk_deinit(void)
{
	int i;

	for (i = 0; i < HZK_FONTSIZE_MAX; i++)
	{
		if (g_hzk_ctx.zk_font[i] != NULL) {
			free(g_hzk_ctx.zk_font[i]);
			g_hzk_ctx.zk_font[i] = NULL;
		}
	}
	
	return 0;
}


















