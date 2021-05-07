#ifndef _hzk_font_h_
#define _hzk_font_h_

#ifdef __cplusplus
extern "C" {
#endif


#define HZK_STR_MAX_LEN	128
#define HZK_FILE_PATH	"./"


typedef struct hzk_font_s hzk_handle_t;				// 码流缓冲读句柄

typedef enum {
	BITMAP_GRAY,		/* GRAY, 1 ch */
	BITMAP_ARGB8888,	/* ARGB8888 byte3 = A, byte2 = R, byte1 = G, byte0 = B*/
	BITMAP_BGRA8888,	/* BGRA8888 byte3 = B, byte2 = G, byte1 = R, byte0 = A*/
	BITMAP_ARGB1555,	/* ARGB = 1555 */
	BITMAP_BGRA5551,	/* BGRA = 5551 */
	BITMAP_RGB888,		/* RGB = 888 default*/
	BITMAP_BGR888,		/* BGR = 888 default*/
} BITMAP_FORMAT_E;

typedef struct {
    int            	w;          // width, unit is pixel.
    int            	h;          // heigh, unit is pixel.
	BITMAP_FORMAT_E	pixel_fomat;// ARGB15555
	int            	color;
    int            	pbytes;     // one pixel bytes.
	int				space;		// pixel.
    int           	wstride;    // width lengh, unit is byte.
    char 			*data;      // bitmap data, unit is byte.
} hzk_bitmap_info_t;

typedef struct {
    char            str[HZK_STR_MAX_LEN+1];         // 需要转换的字符串.
	BITMAP_FORMAT_E	pixel_fomat;					// 转换后点阵数据格式，如ARGB15555
    int            	font_size;          			// 转换后点阵的字号(长宽像素), 仅支持8的倍数
    int            	color;     						// 转换后颜色rgb格式
    int           	outline;    					// 0=不带描边, 1=带1个像素描边...
} hzk_disp_attr_t;



// 计算字符串转后bitmap的w/h pixel单位
int hzk_calc_bitmap_size(char *str, int font_size, int *width, int *height);
int hzk_str2bitmap(hzk_disp_attr_t *ctx, hzk_bitmap_info_t *out);
// must do it after hzk_str2bitmap()
int hzk_bitmap_free(hzk_bitmap_info_t *bitmap);
// 字符的点阵数据按bitmap格式添加, 对外只支持RGB格式一致的bitmap叠加
int hzk_bitmap_add(hzk_bitmap_info_t *in, int x, int y, hzk_bitmap_info_t *bitmap);
int hzk_bitmap_print(hzk_bitmap_info_t *in);
int hzk_init(void);
int hzk_deinit(void);


#ifdef __cplusplus
}
#endif
#endif /* !_hzk_font_h_ */
