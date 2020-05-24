#include "sjt.h"
#include "cJSON.h"

#include <stdlib.h>





typedef int (sjt_bind_cb_t)(cJSON* json, int d, void *val, int size);


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












