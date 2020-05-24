#ifndef __SJT_H__
#define __SJT_H__

#include <cJSON.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* struct json software version number */
#define SJT_SW_VERSION                "1.0.4"

#ifndef int64
    #define int64 long long
#endif

#define SJT_STRUCT2JSON     0
#define SJT_JSON2STRUCT     1


int sjt_bind_bool(cJSON *json, int d, int *val, int size);
int sjt_bind_int(cJSON *json, int d, int *val, int size);
int sjt_bind_char(cJSON *json, int d, char *val, int size);
int sjt_bind_short(cJSON *json, int d, short *val, int size);
int sjt_bind_float(cJSON *json, int d, float *val, int size);
int sjt_bind_double(cJSON *json, int d, double *val, int size);
int sjt_bind_int64(cJSON *json, int d, int64 *val, int size);
int sjt_bind_string(cJSON *json, int d, char *val, int size);



#define SJT_STRUCT(TYPE) extern int sjt_bind_##TYPE(cJSON* json, int d, TYPE *val, int size);   \
                         extern int sjt__##TYPE(char *str_json, int d, TYPE *val, int size);    \
                         int sjt_##TYPE(char *str_json, int d, TYPE *val, int size)             \
                         {                                                                      \
                            cJSON* json = cJSON_Parse(str_json);                                \
                            if (json == NULL) {                                                 \
                                json = cJSON_CreateObject();                                    \
                            }                                                                   \
                            int ret = sjt_bind_##TYPE(json, d, val, size);                      \
                            if (!d) {   /* struct -> json */                                    \
                                char *pjson = cJSON_Print(json);                                \
                                strcpy(str_json, pjson);                                        \
                                free(pjson);                                                    \
                            }                                                                   \
                            cJSON_Delete(json);                                                 \
                            if (ret < 0) return ret;                                            \
                         }                                                                      \
                         int sjt_bind_##TYPE(cJSON* json, int d, TYPE *val, int size)

                         
#define SJT_FIELD(TYPE, ELEMENT)                                                        \
    do {                                                                                \
        int ret = 0;                                                                    \
        if (d) {   /* json -> struct */                                                 \
            cJSON* ajson = cJSON_GetObjectItem(json, #ELEMENT);                         \
            if (ajson) {                                                                \
                ret = sjt_bind_##TYPE(ajson, d, &val->ELEMENT, 4);                      \
            } else { ret = -10; }                                                       \
        } else {   /* struct -> json */                                                 \
            cJSON* ajson = cJSON_CreateObject();                                        \
            if (ajson) {                                                                \
                ret = sjt_bind_##TYPE(ajson, d, &val->ELEMENT, 4);                      \
                cJSON_AddItemToObject(json, #ELEMENT, ajson);                           \
            } else { ret = -10; }                                                       \
        }                                                                               \
        if (ret < 0) return ret;                                                        \
    } while(0)


#define SJT_STRING(ELEMENT, SIZE)                                                       \
    do {                                                                                \
        int ret = 0;                                                                    \
        if (d) {   /* json -> struct */                                                 \
            cJSON* ajson = cJSON_GetObjectItem(json, #ELEMENT);                         \
            if (ajson) {                                                                \
                ret = sjt_bind_string(ajson, d, &val->ELEMENT, SIZE);                   \
            } else { ret = -20; }                                                       \
        } else {   /* struct -> json */                                                 \
            cJSON* ajson = cJSON_CreateObject();                                        \
            if (ajson) {                                                                \
                ret = sjt_bind_string(ajson, d, &val->ELEMENT, SIZE);                   \
                cJSON_AddItemToObject(json, #ELEMENT, ajson);                           \
            } else { ret = -20; }                                                       \
        }                                                                               \
        if (ret < 0) return ret;                                                        \
    } while(0)


#define SJT_ARRAY(TYPE, ELEMENT, SIZE)                                                  \
    do {                                                                                \
        int ret = 0, index = 0, array_size = 0;                                         \
        if (d) {   /* json -> struct */                                                 \
            cJSON* ajson = cJSON_GetObjectItem(json, #ELEMENT);                         \
            if (ajson) {                                                                \
                array_size = cJSON_GetArraySize(ajson);                                 \
                for (index = 0; index < array_size; index++)                            \
                {                                                                       \
                    cJSON* jani = cJSON_GetArrayItem(ajson, index);                     \
                    if (jani) {                                                         \
                        ret = sjt_bind_##TYPE(jani, d, &val->ELEMENT[index], SIZE);     \
                    } else { ret = -31; }                                               \
                }                                                                       \
            } else { ret = -30; }                                                       \
        } else {   /* struct -> json */                                                 \
            cJSON* ajson = cJSON_CreateArray();                                         \
            if (ajson) {                                                                \
                for (index = 0; index < SIZE; index++)                                  \
                {                                                                       \
                    cJSON* jani = cJSON_CreateObject();                                 \
                    if (jani) {                                                         \
                        ret = sjt_bind_##TYPE(jani, d, &val->ELEMENT[index], SIZE);     \
                        cJSON_AddItemToArray(ajson, jani);                              \
                    } else { ret = -31; }                                               \
                }                                                                       \
                cJSON_AddItemToObject(json, #ELEMENT, ajson);                           \
            } else { ret = -30; }                                                       \
        }                                                                               \
        if (ret < 0) return ret;                                                        \
    } while(0)



#ifdef __cplusplus
}
#endif

#endif /* __SJT_H__ */
