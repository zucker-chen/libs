
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "des.h"
#include "base64.h"
#include "desb_crypt.h"

#define DES_CRYPT_DEBUG
#ifdef DES_CRYPT_DEBUG
#define pri_dbg(format, args...) fprintf(stderr,"%s %d %s() " format, __FILE__, __LINE__, __func__, ## args)
#else
#define pri_dbg(format, args...) do{}while(0)
#endif

// DES key is 8 bytes long
#define DES_KEY_SIZE 8


    
    
/*
 * key:     generate 8byte random data. seed is time().
 * return:  0:successed, <0:failed
 */
int desb_generate_key(unsigned char* des_key)
{
    unsigned int iseed = (unsigned int)time(NULL);
    srand (iseed);

    generate_key(des_key);
    
    return 0;
}

/*
 * description: 1,generate 8byte DES_KEY;
 *              2,des encode data; 
 *              3,pad the DES_KEY to des data; 
 *              4,pad desb version number; 
 *              5,base64 encode data
 * in_data:     input data
 * out_data:    output data, the buffer space must be >= ¡Ölen*4/3+27+16, 16byte left
 * len:         input data length(<=128 byte) and return output data length
 */
int desb_data_encrypt(unsigned char *in_data, unsigned char *out_data, unsigned int *len)
{
    if (in_data == NULL || out_data == NULL || len == NULL  ) {
        fprintf(stderr, "%s %d %s() error: pointer is NULL.\n", __FILE__, __LINE__, __func__);
        return -1;
    }
    if (*len > 128 || *len <= 0) {
        fprintf(stderr, "%s %d %s() error: input data length must be <=128 and >0 byte.\n", __FILE__, __LINE__, __func__);
        *len = 0;
        return -2;
    }

    unsigned char des_key[DES_KEY_SIZE] = "12345678";
    unsigned int in_len = *len;
    unsigned int des_out_len = 0, base64_out_len = 0;
    //unsigned char* des_buff = (unsigned char*) malloc((in_len+8+DES_KEY_SIZE+16)*sizeof(char));    // >= len+8+DES_KEY_SIZE+16, 16byte left
    unsigned char* des_buff = (unsigned char*) malloc((128+8+DES_KEY_SIZE+16)*sizeof(char));    // >= len+8+DES_KEY_SIZE+16, 16byte left
    unsigned char* data_block = in_data;
    unsigned char* processed_block = des_buff;

    desb_generate_key(des_key);
    pri_dbg("des key = 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\n", \
            des_key[0], des_key[1], des_key[2], des_key[3], des_key[4], des_key[5], des_key[6], des_key[7]);
    des_encode(data_block, processed_block, des_key, &in_len);

    des_out_len = in_len;
    pri_dbg("des_out_len = %d\n", des_out_len);

    // 3,pad the DES_KEY to des data
    memcpy(processed_block + des_out_len, des_key, DES_KEY_SIZE);
    // 4,pad desb version number, in order to facilitate extension
    unsigned char desb_ver = 0x10;  // v1.0
    processed_block[des_out_len + DES_KEY_SIZE] = desb_ver;
    des_out_len = des_out_len + DES_KEY_SIZE + 1;
    
    base64_encode(processed_block, &des_out_len, out_data);
    base64_out_len = des_out_len;
    pri_dbg("base64_out_len = %d\n", base64_out_len);
    
    *len = base64_out_len;
    
    //memcpy(out_data, des_buff, des_out_len);
	free(des_buff);

    return 0;
}


/*
 * description: 1,base64 decode; 
 *              2,get desb version number and check
 *              3,get out 8byte DES_KEY; 
 *              4,des decode the data.
 * in_data:     input data
 * out_data:    output data, the buffer space must be >= 128+16, 16byte left
 * len:         input data length(<=198 byte) and return output data length
 */
int desb_data_decrypt(unsigned char *in_data, unsigned char *out_data, unsigned *len)
{
    if (in_data == NULL || out_data == NULL || len == NULL  ) {
        fprintf(stderr, "%s %d %s() error: pointer is NULL.\n", __FILE__, __LINE__, __func__);
        return -1;
    }
    if (*len > 198 || *len <= 0) {   // 128*4/3+27, 128 is max encrypt input lenght
        fprintf(stderr, "%s %d %s() error: input data length must be <=198 and >0 byte.\n", __FILE__, __LINE__, __func__);
        *len = 0;
        return -2;
    }
    
    unsigned char des_key[DES_KEY_SIZE] = "12345678";
    unsigned int in_len = *len;
    unsigned int des_out_len = 0, base64_out_len = 0;
    unsigned char* des_buff = (unsigned char*) malloc((150+16)*sizeof(char));    // (198*3)/4+1+16, 16byte left
    unsigned char* data_block = in_data;
    unsigned char* processed_block = des_buff;

    base64_decode(data_block, &in_len, processed_block);
    base64_out_len = in_len;
    pri_dbg("base64_out_len = %d\n", base64_out_len);

    // 2,get desb version number and check
    unsigned char desb_ver = 0x10;  // v1.0
    desb_ver = processed_block[base64_out_len - 1];
    pri_dbg("desb_ver = v%X\n", desb_ver);
    if (desb_ver != 0x10) {
        fprintf(stderr, "%s %d %s() error: v%X is a incorrect version.\n", __FILE__, __LINE__, __func__, desb_ver);
        *len = 0;
        return -3;
    }
    // 3,get out 8byte DES_KEY
    memcpy(des_key, processed_block + base64_out_len - DES_KEY_SIZE - 1, DES_KEY_SIZE);
    base64_out_len = base64_out_len - DES_KEY_SIZE - 1;
    pri_dbg("base64_out_len = %d\n", base64_out_len);
    //pri_dbg("des key = 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\n", \
    //        des_key[0], des_key[1], des_key[2], des_key[3], des_key[4], des_key[5], des_key[6], des_key[7]);

    des_decode(processed_block, out_data, des_key, &base64_out_len);
    des_out_len = base64_out_len;
    pri_dbg("des_out_len = %d\n", des_out_len);
    *len = des_out_len;
    
    //memcpy(out_data, des_buff, des_out_len);
	free(des_buff);

    return 0;

}






