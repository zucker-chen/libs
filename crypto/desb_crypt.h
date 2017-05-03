
#ifndef _12345_DESB_CRYPT_H_54321_
#define _12345_DESB_CRYPT_H_54321_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * key:     generate 8byte random data. seed is time().
 * return:  0:successed, <0:failed
 */
int desb_generate_key(unsigned char* des_key);

/*
 * description: desb_data_encrypt
 * in_data:     input data
 * out_data:    output data, the buffer space must be >= ¡Ölen*4/3+27+16, 16byte left
 * len:         input data length(<=128 byte) and return output data length
 */
int desb_data_encrypt(unsigned char *in_data, unsigned char *out_data, int *len);

/*
 * description: description
 * in_data:     input data
 * out_data:    output data, the buffer space must be >= 128+16, 16byte left
 * len:         input data length(<=198 byte) and return output data length
 */
int desb_data_decrypt(unsigned char *in_data, unsigned char *out_data, int *len);


#ifdef __cplusplus
}
#endif

#endif

