#ifndef __SJT_H__
#define __SJT_H__

#include <cJSON.h>
#include <stdio.h>
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


#define SJT_STRUCT(TYPE) static int sjt_bind_##TYPE(cJSON* json, int d, TYPE *val, int size);   \
                         static int sjt_##TYPE(char *str_json, int d, TYPE *val)                \
                         {                                                                      \
                            cJSON* json = cJSON_Parse(str_json);                                \
                            if (json == NULL) {                                                 \
                                json = cJSON_CreateObject();                                    \
                            }                                                                   \
                            int ret = sjt_bind_##TYPE(json, d, val, 0);                         \
                            if (!d) {   /* struct -> json */                                    \
                                char *pjson = cJSON_Print(json);                                \
                                strcpy(str_json, pjson);                                        \
                                cJSON_free(pjson);                                              \
                            }                                                                   \
                            cJSON_Delete(json);                                                 \
                            if (ret < 0) return ret;                                            \
                            return 0;                                                           \
                         }                                                                      \
                         static int sjt_bind_##TYPE(cJSON* json, int d, TYPE *val, int size)

                         
#define SJT_FIELD(TYPE, ELEMENT)                                                                \
    do {                                                                           	            \
        int ret = 0;                                                               	            \
        if (d) {   /* json -> struct */                                                         \
            cJSON* ajson = cJSON_GetObjectItem(json, #ELEMENT);                                 \
            if (ajson) {                                                                        \
                ret = sjt_bind_##TYPE(ajson, d, (TYPE *)&val->ELEMENT, 4);                      \
            } else { ret = -10; }                                                               \
        } else {   /* struct -> json */                                                         \
            cJSON* ajson = cJSON_CreateObject();                                                \
            if (ajson) {                                                                        \
                ret = sjt_bind_##TYPE(ajson, d, (TYPE *)&val->ELEMENT, 4);                      \
                cJSON_AddItemToObject(json, #ELEMENT, ajson);                                   \
            } else { ret = -10; }                                                               \
        }                                                                                       \
        if (ret < 0) {                                                                          \
        	printf("%s:%d Warnning (%d) %s error.\n", __FUNCTION__, __LINE__, d, #ELEMENT);     \
        	return ret;                                                                         \
        }                                                                                       \
    } while(0)


#define SJT_STRING(ELEMENT, SIZE)                                                               \
    do {                                                                                        \
        int ret = 0;                                                                            \
        if (d) {   /* json -> struct */                                                         \
            cJSON* ajson = cJSON_GetObjectItem(json, #ELEMENT);                                 \
            if (ajson) {                                                                        \
                ret = sjt_bind_string(ajson, d, (char *)&val->ELEMENT, SIZE);                   \
            } else { ret = -20; }                                                               \
        } else {   /* struct -> json */                                                         \
            cJSON* ajson = cJSON_CreateObject();                                                \
            if (ajson) {                                                                        \
                ret = sjt_bind_string(ajson, d, (char *)&val->ELEMENT, SIZE);                   \
                cJSON_AddItemToObject(json, #ELEMENT, ajson);                                   \
            } else { ret = -20; }                                                               \
        }                                                                                       \
        if (ret < 0) {                                                                          \
            printf("%s:%d Warnning (%d) %s error.\n", __FUNCTION__, __LINE__, d, #ELEMENT);     \
            return ret;                                                                         \
        }                                                                                       \
    } while(0)


#define SJT_ARRAY(TYPE, ELEMENT, SIZE)                                                          \
    do {                                                                                        \
        int ret = 0, index = 0, array_size = 0;                                                 \
        if (d) {   /* json -> struct */                                                         \
            cJSON* ajson = cJSON_GetObjectItem(json, #ELEMENT);                                 \
            if (ajson) {                                                                        \
                array_size = cJSON_GetArraySize(ajson);                                         \
                for (index = 0; index < array_size; index++)                                    \
                {                                                                               \
                    cJSON* jani = cJSON_GetArrayItem(ajson, index);                             \
                    if (jani) {                                                                 \
                        ret = sjt_bind_##TYPE(jani, d, &val->ELEMENT[index], SIZE);             \
                    } else { ret = -31; }                                                       \
                }                                                                               \
            } else { ret = -30; }                                                               \
        } else {   /* struct -> json */                                                         \
            cJSON* ajson = cJSON_CreateArray();                                                 \
            if (ajson) {                                                                        \
                for (index = 0; index < SIZE; index++)                                          \
                {                                                                               \
                    cJSON* jani = cJSON_CreateObject();                                         \
                    if (jani) {                                                                 \
                        ret = sjt_bind_##TYPE(jani, d, &val->ELEMENT[index], SIZE);             \
                        cJSON_AddItemToArray(ajson, jani);                                      \
                    } else { ret = -31; }                                                       \
                }                                                                               \
                cJSON_AddItemToObject(json, #ELEMENT, ajson);                                   \
            } else { ret = -30; }                                                               \
        }                                                                                       \
        if (ret < 0) {                                                                          \
            printf("%s:%d Warnning (%d) %s error.\n", __FUNCTION__, __LINE__, d, #ELEMENT);     \
            return ret;                                                                         \
        }                                                                                       \
    } while(0)

#define SJT_FIELD_END() return 0; /* or sjt_status */


/* SJT OBJECT PARSER */        
#define SJT_OBJ_TYPE_INVALID	cJSON_Invalid
#define SJT_OBJ_TYPE_NUM		cJSON_Number
#define SJT_OBJ_TYPE_STRING		cJSON_String
#define SJT_OBJ_TYPE_ARRAY		cJSON_Array
#define SJT_OBJ_TYPE_OBJECT		cJSON_Object

#define SJT_MEMBER_DEPTH_MAX	8
#define SJT_TAG_NAME_LEN_MAX	32
#define SJT_OBJ_ARRAY_SISE_MAX	8


typedef struct sjt_object_parser_s {
	cJSON*		handle;                                                      // internal used
	int			tag_depth;                                                   // JSON's tag depth 1~SJT_MEMBER_DEPTH_MAX
	struct {
        int		type;                                                        // SJT_OBJ_TYPE
        char	name[SJT_TAG_NAME_LEN_MAX];
        int 	array_index;                                                 // Only for array, 0xff = all object content
        int 	array_size;                                                	 // only for array
	} tag_info[SJT_MEMBER_DEPTH_MAX];
} sjt_object_parser_t;


/* 
 * input: 	in_str_json
 * output:	return sop, sop->handle
 */
sjt_object_parser_t *sjt_object_parser_create(char *in_str_json);
/* 
 * input: 	sop, sop->tag_info[i].name, sop->tag_info[i].array_index
 * output:	data, size, sop->tag_info[i].type
 */
int sjt_object_get_content(sjt_object_parser_t *sop, char *data, int *size);
/* 
 * input: 	sop, data, size, sop->tag_info[i].type, sop->tag_info[i].name, sop->tag_info[i].array_index
 * output:	null
 */
int sjt_object_set_content(sjt_object_parser_t *sop, char *data, int size);
/* 
 * input: 	sop
 * output:	null
 */
int sjt_object_parser_destroy(sjt_object_parser_t *sop);


        

#ifdef __cplusplus
}
#endif

#endif /* __SJT_H__ */
