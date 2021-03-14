#include "sjt.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>


int sjt_bind_bool(cJSON *json, int d, int *val, int size)
{
    if(d) {   // json -> struct
        if( json && (json->type == cJSON_True || json->type == cJSON_False)) {
            *(val) = json->type;
        } else {
            return -1;
        }
    } else {  // struct -> json
        json->type = *val ? cJSON_True : cJSON_False;
    }
    
    return 0;
}


int sjt_bind_int(cJSON *json, int d, int *val, int size)
{
    if(d) {   // json -> struct
        if( json && json->type == cJSON_Number) {
            *val = json->valueint;
        } else {
            return -1;
        }
    } else {  // struct -> json
        json->type = cJSON_Number;
        json->valueint = json->valuedouble = *val;
    }
    
    return 0;
}


int sjt_bind_char(cJSON *json, int d, char *val, int size)
{
    if(d) {   // json -> struct
        if( json && json->type == cJSON_Number) {
            *val = (char)json->valueint;
        } else {
            return -1;
        }
    } else {  // struct -> json
        json->type = cJSON_Number;
        json->valueint = json->valuedouble = *val;
    }
    
    return 0;
}


int sjt_bind_short(cJSON *json, int d, short *val, int size)
{
    if(d) {   // json -> struct
        if( json && json->type == cJSON_Number) {
            *val = (short)json->valueint;
        } else {
            return -1;
        }
    } else {  // struct -> json
        json->type = cJSON_Number;
        json->valueint = json->valuedouble = *val;
    }
    
    return 0;
}


int sjt_bind_float(cJSON *json, int d, float *val, int size)
{
    if(d) {   // json -> struct
        if( json && json->type == cJSON_Number) {
            *val = (float)json->valuedouble;
        } else {
            return -1;
        }
    } else {  // struct -> json
        json->type = cJSON_Number;
        json->valuedouble = (float)*val;
    }
    
    return 0;
}


int sjt_bind_double(cJSON *json, int d, double *val, int size)
{
    if(d) {   // json -> struct
        if( json && json->type == cJSON_Number) {
            *val = (double)json->valuedouble;
        } else {
            return -1;
        }
    } else {  // struct -> json
        json->type = cJSON_Number;
        json->valuedouble = *val;
    }
    
    return 0;
}


/* note: long long need string to do it. only support decimal*/
int sjt_bind_int64(cJSON *json, int d, int64 *val, int size)
{
    if(d) {   // json -> struct
        if( json && json->type == cJSON_String) {
            *val = strtoull(json->valuestring, NULL, 10);
        } else {
            return -1;
        }
    } else {  // struct -> json
        json->type = cJSON_String;
        char t[64];
        sprintf(t, "%lld", *val);
        unsigned char *copy = malloc(strlen(t)+1);
        if (copy == NULL) {
            return -1;
        }
        strncpy(copy, t, strlen(t)+1);
        json->valuestring = copy;
    }
    
    return 0;
}


int sjt_bind_string(cJSON *json, int d, char *val, int size)
{
    if(d) {   // json -> struct
        if( json && json->type == cJSON_String) {
            strncpy((char*)val, json->valuestring, size);
        } else {
            return -1;
        }
    } else {  // struct -> json
        json->type = cJSON_String;
        unsigned char *copy = malloc(size);
        if (copy == NULL) {
            return -1;
        }
        strncpy(copy, val, size);
        json->valuestring = copy;
    }
    
    return 0;
}



/* 
 * input: 	in_str_json
 * output:	return sop, sop->handle
 */
sjt_object_parser_t *sjt_object_parser_create(char *in_str_json)
{
	sjt_object_parser_t *sop = NULL;
	
	if (in_str_json == NULL) {
		return NULL;
	}
	
	sop = malloc(sizeof(sjt_object_parser_t));
	if (sop == NULL) {
		return NULL;
	}
	
	sop->handle = cJSON_Parse(in_str_json);
	if (sop->handle == NULL) {
		free(sop);
		return NULL;
	}
	
	return sop;
}


/* 
 * input: 	sop, sop->tag_info[i].name, sop->tag_info[i].array_index
 * output:	data, size, sop->tag_info[i].type
 */
int sjt_object_get_content(sjt_object_parser_t *sop, char *data, int *size)
{
	int ret = 0, i = 0, type = 0;
	
	cJSON *ajson = NULL, *bjson = NULL;	// ajson is dest obj
	
	if (sop == NULL || data == NULL || size == NULL) {
		return -1;
	}
	
	if (sop->tag_depth < 0) {
		return -2;
	}

	if (sop->handle == NULL) {
		return -3;
	}
	
	ajson = sop->handle;
	for (i = 0; i < sop->tag_depth; i++)
	{
		if (ajson->type != cJSON_Array) {
			bjson = cJSON_GetObjectItem(ajson, sop->tag_info[i].name);
			if (bjson == NULL) {
				return -4;
			}		
		} else {
			sop->tag_info[i].array_size = cJSON_GetArraySize(ajson);
			bjson = cJSON_GetArrayItem(ajson, sop->tag_info[i].array_index <= 0 ? 0 : sop->tag_info[i].array_index);
			if (bjson == NULL) {
				return -5;
			}
		}
		sop->tag_info[i].type = bjson->type;
		ajson = bjson;
	}	
	bjson = NULL;
	
	char *pjson = cJSON_Print(ajson);
	//printf("obj===>:\n %s\n<===obj, len = %d\n", pjson, strlen(pjson));
	strncpy(data, pjson, *size > 0 ? *size : strlen(pjson));
	*size = strlen(pjson);
	free(pjson);
	
	return 0;
}


/* 
 * input: 	sop, data, size, sop->tag_info[i].type, sop->tag_info[i].name, sop->tag_info[i].array_index
 * output:	null
 */
int sjt_object_set_content(sjt_object_parser_t *sop, char *data, int size)
{
	int ret = 0, i = 0, type = 0;
	
	cJSON *ajson = NULL, *bjson = NULL;	// ajson is dest obj
	
	if (sop == NULL || data == NULL) {
		return -1;
	}
	
	if (sop->tag_depth <= 0 || size <= 0) {
		return -2;
	}

	if (sop->handle == NULL) {
		return -3;
	}
	
	ajson = sop->handle;
	for (i = 0; i < sop->tag_depth-1; i++)
	{
		if (ajson->type != cJSON_Array) {
			bjson = cJSON_GetObjectItem(ajson, sop->tag_info[i].name);
			if (bjson == NULL) {
				return -4;
			}		
		} else {
			sop->tag_info[i].array_size = cJSON_GetArraySize(ajson);
			bjson = cJSON_GetArrayItem(ajson, sop->tag_info[i].array_index <= 0 ? 0 : sop->tag_info[i].array_index);
			if (bjson == NULL) {
				return -5;
			}
		}
		sop->tag_info[i].type = bjson->type;
		ajson = bjson;
	}	
	bjson = NULL;

	type = sop->tag_info[i].type;
	if (type == SJT_OBJ_TYPE_NUM) {
		double num = *(double *)data;
		if (NULL != cJSON_GetObjectItem(ajson, sop->tag_info[i].name)) {
			cJSON_ReplaceItemInObject(ajson, sop->tag_info[i].name, cJSON_CreateNumber(num));
		} else {
			cJSON_AddItemToObject(ajson, sop->tag_info[i].name, cJSON_CreateNumber(num));
		}
	} else if (type == SJT_OBJ_TYPE_STRING) {
		char *string = &data[0];
		if (NULL != cJSON_GetObjectItem(ajson, sop->tag_info[i].name)) {
			cJSON_ReplaceItemInObject(ajson, sop->tag_info[i].name, cJSON_CreateString(string));
		} else {
			cJSON_AddItemToObject(ajson, sop->tag_info[i].name, cJSON_CreateString(string));
		}
	} else if (type == SJT_OBJ_TYPE_OBJECT) {
		// TODO
		return -7;
	} else {
		return -6;
	}
		
	//char *pjson = cJSON_Print(sop->handle);
	//printf("obj===>:\n %s\n<===obj\n", pjson);
	//free(pjson);
	return 0;
}


/* 
 * input: 	sop
 * output:	null
 */
int sjt_object_parser_destroy(sjt_object_parser_t *sop)
{
	if (sop == NULL) {
		return -1;
	}
	
	if (sop->handle != NULL) {
		cJSON_Delete((cJSON *)sop->handle);
	}
	
	free(sop);
	
	return 0;
}





