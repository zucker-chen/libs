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
  SXT_FIELD(float, fl);
  SXT_FIELD(short, b2);
  SXT_FIELD(int64, ll);
  SXT_FIELD(char, ch);
  SXT_FIELD(double, db);
};


void sxt_test1(void)
{
    test1_t cfg;
    char buf[1024] = "\0";
    FILE *f = NULL;
    int ret = -1;

    
    #if 0
    // save cfg
    cfg.fl = 1.2;
    cfg.db = 20302435.24235;
    cfg.ll = 1234567890L;
    sxt_test1_t(buf, SXT_STRUCT2XML, &cfg);
    f = fopen("test1.xml","w+");
    fprintf(f, "%s", buf);
    fflush(f);
    fclose(f);
    #endif

    
    
    #if 1
    // load cfg
    f = fopen("test1.xml","rb");
    fseek(f,0,SEEK_END);long len=ftell(f);fseek(f,0,SEEK_SET);
	char *data=(char*)malloc(len+1);fread(data,1,len,f);fclose(f);
    pri_dbg("load:\n%s\n", data);
    sxt_test1_t(data, SXT_XML2STRUCT, &cfg);
    free(data);
    #endif
    
    
    pri_dbg("val:%d\n", cfg.val);
    pri_dbg("fl:%f\n", cfg.fl);
    pri_dbg("b2:%d\n", cfg.b2);
    pri_dbg("ll:%lld\n", cfg.ll);
    pri_dbg("ch:%d\n", cfg.ch);
    pri_dbg("db:%lf\n", cfg.db);

}



typedef struct test2_s {
    char         str[128];
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

    
    #if 0
    // save cfg
    strcpy(cfg.str, "Hello World!");
    cfg.x[0] = 5;
    cfg.x[3] = 33;
    sxt_test2_t(buf, SXT_STRUCT2XML, &cfg);
    f = fopen("test2.xml","w+");
    fprintf(f, "%s", buf);
    pri_dbg("\n%s\n", buf);
    fclose(f);
    #endif

    
    
    #if 1
    // load cfg
    f = fopen("test2.xml","rb");
    fseek(f,0,SEEK_END);len=ftell(f);fseek(f,0,SEEK_SET);
	data=(char*)malloc(len+1);fread(data,1,len,f);fclose(f);
    pri_dbg("load:\n%s\n", data);
    ret = sxt_test2_t(data, SXT_XML2STRUCT, &cfg);
    free(data);
    #endif
    
    #if 1
    // save cfg
    cfg.t1[1].fl = 5.3;
    cfg.t1[0].b2 = 4321;
    sxt_test2_t(buf, SXT_STRUCT2XML, &cfg);
    f = fopen("test2.xml","w+");
    fprintf(f, "%s", buf);
    pri_dbg("save:%s\n", buf);
    fclose(f);
    #endif

    #if 1
    // load cfg
    f = fopen("test2.xml","rb");
    fseek(f,0,SEEK_END);len=ftell(f);fseek(f,0,SEEK_SET);
	data=(char*)malloc(len+1);fread(data,1,len,f);fclose(f);
    pri_dbg("load:\n%s\n", data);
    ret = sxt_test2_t(data, SXT_XML2STRUCT, &cfg);
    free(data);
    #endif

    
    pri_dbg("str:%s\n", cfg.str);
    pri_dbg("x[1]:%d\n", cfg.x[1]);
    pri_dbg("x[2]:%d\n", cfg.x[2]);
    pri_dbg("t[1].ll:%lld\n", cfg.t1[1].ll);
    pri_dbg("t[1].fl:%f\n", cfg.t1[1].fl);
    pri_dbg("t[0].b2:%d\n", cfg.t1[0].b2);

}


int main (int argc, char *argv[])
{
	//sxt_test1();
	sxt_test2();

	return 0;
}



