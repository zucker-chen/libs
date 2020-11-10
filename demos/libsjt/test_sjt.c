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
    sjt_test1_t(buf, SJT_STRUCT2JSON, &cfg);
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
    sjt_test1_t(data, SJT_JSON2STRUCT, &cfg);
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

    
    #if 1
    // save cfg
    strcpy(cfg.str, "Hello World!");
    cfg.x[0] = 5;
    cfg.x[3] = 33;
	cfg.t1[1].db = 33333.4445555;
    sjt_test2_t(buf, SJT_STRUCT2JSON, &cfg);
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
    ret = sjt_test2_t(data, SJT_JSON2STRUCT, &cfg);
    pri_dbg("load:%s\n", data);
    free(data);
    #endif
    
    #if 1
    // save cfg
	memset(buf, 0, sizeof(buf));
    sjt_test2_t(buf, SJT_STRUCT2JSON, &cfg);
    f = fopen("test2.json","w+");fseek(f,0,SEEK_SET);
    fprintf(f, "%s", buf);
    pri_dbg("save:%s\n", buf);
    fclose(f);
    #endif

    #if 1
    // load cfg
    f = fopen("test2.json","rb");
    fseek(f,0,SEEK_END);len=ftell(f);fseek(f,0,SEEK_SET);
	data=(char*)malloc(len+1);fread(data,1,len,f);fclose(f);
    ret = sjt_test2_t(data, SJT_JSON2STRUCT, &cfg);
    pri_dbg("load:%s\n", data);
    free(data);
    #endif

    pri_dbg("str:%s\n", cfg.str);
    pri_dbg("x[1]:%d\n", cfg.x[1]);
    pri_dbg("x[2]:%d\n", cfg.x[2]);
    pri_dbg("t[1].ll:%lld\n", cfg.t1[1].ll);
    pri_dbg("t[0].b2:%d\n", cfg.t1[0].b2);
	
	
	#if 1
    // load cfg
    f = fopen("test2.json","rb");
    fseek(f,0,SEEK_END);len=ftell(f);fseek(f,0,SEEK_SET);
	data=(char*)malloc(len+1);fread(data,1,len,f);fclose(f);
	
	sjt_object_parser_t *sop = NULL;
	sop = sjt_object_parser_create(data);
	if (sop != NULL) {
		sop->tag_depth = 1;
		strncpy(sop->tag_info[sop->tag_depth-1].name, "t1", SJT_TAG_NAME_LEN_MAX);
		sop->tag_depth = 2;
		sop->tag_info[sop->tag_depth-1].array_index = 1;
		sop->tag_depth = 3;
		strncpy(sop->tag_info[sop->tag_depth-1].name, "db", SJT_TAG_NAME_LEN_MAX);
		sjt_object_get_content(sop, buf, &len);
		buf[len] = '\0'; printf("buf = %s\n", buf); double db; sscanf(buf, "%lf", &db); printf("db = %lf\n", db);
		pri_dbg("load:%s\n", data);
		
		//sop->tag_info[sop->tag_depth-1].type = SJT_OBJ_TYPE_STRING
		//sjt_object_set_content(sop, "ABCDEFG...", 20);
		sop->tag_info[sop->tag_depth-1].type = SJT_OBJ_TYPE_NUM;
		*(double *)buf = 0.000000123564823423;
		sjt_object_set_content(sop, buf, 4);
		
		sop->tag_depth = 0; len = sizeof(buf);
		sjt_object_get_content(sop, buf, &len);
		pri_dbg("load buf:%s\n", buf);
		sjt_object_parser_destroy(sop);
	}
	
    free(data);
    #endif
    


}


int main (int argc, char *argv[])
{
	sjt_test1();
	sjt_test2();

	return 0;
}



