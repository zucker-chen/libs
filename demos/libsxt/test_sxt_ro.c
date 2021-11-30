#include "sxt.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>


#define pri_dbg(format, args...) fprintf(stderr,"%s %d %s() " format, __FILE__, __LINE__, __func__, ## args)




typedef struct test1_s {
    int         val;
    float        fl;
    short       b2;
    long long   ll;
    char        ch;
    double      db;
} test1_t;


SXT_STRUCT(test1_t)
{
  SXT_FIELD(int, val);
  SXT_FIELD(int64, ll);
  SXT_FIELD(char, ch);
};


typedef struct test2_s {
    char         str[128];
    int          y[4];
    int          x[4];
    test1_t      t1[2];
} test2_t;


SXT_STRUCT(test2_t)
{
    SXT_STRING(str, 128);
    SXT_ARRAY(int, x, 4);
    SXT_ARRAY(test1_t, t1, 2);
};


void sxt_test2(void)
{
    int ret, len;
    test2_t cfg;
    char buf[1024] = "\0";
    char *data;
    FILE *f;
  
    // load cfg
    f = fopen("test3.xml","rb");
	if (f == NULL) {
		// init and save cfg
		cfg.t1[1].fl = 1.2;
		cfg.t1[1].db = 20302435.24235;
		cfg.t1[1].ll = 1234567890L;
		cfg.t1[0].b2 = 9527;
		cfg.x[1] = 88;
		cfg.x[2] = 98764321;
		sxt_test2_t(buf, SXT_STRUCT2XML, &cfg);
		f = fopen("test3.xml","w+");
		fprintf(f, "%s", buf);
		fflush(f);
	}
    fseek(f,0,SEEK_END);len=ftell(f);fseek(f,0,SEEK_SET);
	data=(char*)malloc(len+1);fread(data,1,len,f);fclose(f);
    pri_dbg("load:\n%s\n", data);
    ret = sxt_test2_t(data, SXT_XML2STRUCT, &cfg);
    free(data);
   
    pri_dbg("str:%s\n", cfg.str);
    pri_dbg("x[1]:%d\n", cfg.x[1]);
    pri_dbg("x[2]:%d\n", cfg.x[2]);
    pri_dbg("t[1].ll:%lld\n", cfg.t1[1].ll);
    pri_dbg("t[1].fl:%f\n", cfg.t1[1].fl);
    pri_dbg("t[0].b2:%d\n", cfg.t1[0].b2);

}


int main (int argc, char *argv[])
{
	sxt_test2();

	return 0;
}



