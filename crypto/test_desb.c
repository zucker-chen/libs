
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "desb_crypt.h"


#define TEST_DES_CRYPT_DEBUG
#ifdef TEST_DES_CRYPT_DEBUG
#define pri_dbg(format, args...) fprintf(stderr,"%s %d %s() " format, __FILE__, __LINE__, __func__, ## args)
#else
#define pri_dbg(format, args...) do{}while(0)
#endif


typedef struct {
	unsigned char mac[32];
	unsigned char io;
	unsigned char rand[9];
} src_content_t;



int main(int argc, char* argv[]) {
    unsigned char in_data[128] = "\0";
    unsigned char* out_data = (unsigned char*) malloc(198*sizeof(char));
    unsigned char* m_data = (unsigned char*) malloc(198*sizeof(char));
    int raw_len = 0;

#define SRC_DATA_IS_BINARY 1
#if SRC_DATA_IS_BINARY
    src_content_t src_data = {"00:0c:29:b4:de:6e", 0xA5, "12345678"};
    src_content_t *p_out;
    raw_len = sizeof(src_data); // 128
    memcpy(in_data, (unsigned char*)&src_data, raw_len);
#else
    FILE *outf = NULL;
    outf = fopen("./tmp.txt", "wb");  
    unsigned char *src_str = "123456789ABCDEFGR12345678";
    raw_len = strlen(src_str);
    memcpy(in_data, (unsigned char*)src_str, raw_len);
#endif
    
    pri_dbg("in_data_len = %d\n", raw_len);
    desb_data_encrypt(&in_data[0], out_data, &raw_len);
    pri_dbg("encrypt_len = %d\n", raw_len);
    out_data[raw_len]= '\0';
    //fwrite(out_data, 1, raw_len, outf);
    //fclose(outf);
    //pri_dbg("#### = %s\n", out_data);

    desb_data_decrypt(&out_data[0], m_data, &raw_len);
    pri_dbg("decrypt_len = %d\n", raw_len);
    m_data[raw_len]= '\0';

#if SRC_DATA_IS_BINARY
    p_out = (src_content_t *)m_data;
    pri_dbg("1: %s\n", p_out->mac);
    pri_dbg("2: 0x%X\n", p_out->io);
    pri_dbg("3: %s\n", p_out->rand);
#else
    pri_dbg("m_data = %s\n", m_data);
    fwrite(m_data, 1, raw_len, outf);
    fclose(outf);
#endif

    return 0;
}

