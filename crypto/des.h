#ifndef _12345_DES_H_54321_
#define _12345_DES_H_54321_

#ifdef __cplusplus
extern "C" {
#endif



#define ENCRYPTION_MODE 1
#define DECRYPTION_MODE 0

typedef struct {
	unsigned char k[8];
	unsigned char c[4];
	unsigned char d[4];
} key_set;


/*
 * key:   generate 8byte random data.
 */
void generate_key(unsigned char* key);

/*
 * in_data:   input data
 * out_data:  output data, the buffer space must be >= 8 * ((len)/8 + 1)
 * key:       des key, length = 8byte
 * len:       input data length and return output data length
 * is_relay:  input 0:end data, 1:sectional data(is relay)
 * return:	  0:success, <0:fail
 */
int des_encode(unsigned char *in_data, unsigned char *out_data, unsigned char *key, int *len, int is_relay);

/*
 * in_data:   input data
 * out_data:  output data, the buffer space must be >= 8 * (len/8 + ((len%8)?1:0)
 * key:       des key, length = 8byte
 * len:       input data length and return output data length
 * is_relay:  input 0:end data, 1:sectional data(is relay)
 * return:	  0:success, <0:fail
 */
int des_decode(unsigned char *in_data, unsigned char *out_data, unsigned char *key, int *len, int is_relay);


#ifdef __cplusplus
}
#endif

#endif
