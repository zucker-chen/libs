#ifndef __TEST_SJT_H__
#define __TEST_SJT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sjt.h"


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
		

#ifdef __cplusplus
}
#endif

#endif /* __SJT_H__ */
