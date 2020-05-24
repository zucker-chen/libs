#include "sjt.h"

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


SJT_STRUCT(test1_t)
{
  SJT_FIELD(int, val);
  SJT_FIELD(float, fl);
  SJT_FIELD(short, b2);
  SJT_FIELD(int64, ll);
  SJT_FIELD(char, ch);
  SJT_FIELD(double, db);
};


void sjt_test1(void)
{
    test1_t cfg;
    char buf[1024] = "\0";
    FILE *f;

    
    #if 1
    // save cfg
    cfg.fl = 1.2;
    cfg.db = 20302435.24235;
    cfg.ll = 1234567890L;
    sjt_test1_t(buf, SJT_STRUCT2JSON, &cfg, 0);
    f = fopen("test1.json","w+");
    fprintf(f, "%s", buf);
    pri_dbg("\n%s\n", buf);
    fclose(f);
    #endif

    
    
    #if 1
    // load cfg
    f = fopen("test1.json","rb");
    fseek(f,0,SEEK_END);long len=ftell(f);fseek(f,0,SEEK_SET);
	char *data=(char*)malloc(len+1);fread(data,1,len,f);fclose(f);
    sjt_test1_t(data, SJT_JSON2STRUCT, &cfg, 0);
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


SJT_STRUCT(test2_t)
{
    SJT_STRING(str, 128);
    SJT_ARRAY(int, x, 4);
    SJT_ARRAY(test1_t, t1, 2);
};


void sjt_test2(void)
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
    sjt_test2_t(buf, SJT_STRUCT2JSON, &cfg, 0);
    f = fopen("test2.json","w+");
    fprintf(f, "%s", buf);
    pri_dbg("\n%s\n", buf);
    fclose(f);
    #endif

    
    
    #if 1
    // load cfg
    f = fopen("test2.json","rb");
    fseek(f,0,SEEK_END);len=ftell(f);fseek(f,0,SEEK_SET);
	data=(char*)malloc(len+1);fread(data,1,len,f);fclose(f);
    ret = sjt_test2_t(data, SJT_JSON2STRUCT, &cfg, 0);
    pri_dbg("load:%s\n", data);
    free(data);
    #endif
    
    #if 1
    // save cfg
    sjt_test2_t(buf, SJT_STRUCT2JSON, &cfg, 0);
    f = fopen("test2.json","w+");
    fprintf(f, "%s", buf);
    pri_dbg("save:%s\n", buf);
    fclose(f);
    #endif

    #if 1
    // load cfg
    f = fopen("test2.json","rb");
    fseek(f,0,SEEK_END);len=ftell(f);fseek(f,0,SEEK_SET);
	data=(char*)malloc(len+1);fread(data,1,len,f);fclose(f);
    ret = sjt_test2_t(data, SJT_JSON2STRUCT, &cfg, 0);
    pri_dbg("load:%s\n", data);
    free(data);
    #endif

    
    pri_dbg("str:%s\n", cfg.str);
    pri_dbg("x[1]:%d\n", cfg.x[1]);
    pri_dbg("x[2]:%d\n", cfg.x[2]);
    pri_dbg("t[1].ll:%lld\n", cfg.t1[1].ll);
    pri_dbg("t[0].b2:%d\n", cfg.t1[0].b2);

}


int main (int argc, char *argv[])
{
	//sjt_test1();
	sjt_test2();

	return 0;
}



