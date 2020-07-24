#include "sxt.h"
#include "xml_node.h"

#include <stdlib.h>


int sxt_bind_bool(XMLN *xml, int d, int *val, int size)
{
    if(d) {   // xml -> struct
        if( xml && xml->data) {
            *(val) = atoi(xml->data);
        } else {
            return -1;
        }
    } else {  // struct -> xml
        char str[16];
        sprintf(str, "%d", *val);
        xml_attr_add(xml, xml->name, str);
    }
    
    return 0;
}


int sxt_bind_int(XMLN *xml, int d, int *val, int size)
{
    if(d) {   // xml -> struct
        if( xml && xml->data) {
            sscanf(xml->data, "%d", val);
        } else {
            return -1;
        }
    } else {  // struct -> xml
        char str[16];
        sprintf(str, "%d", *val);
        xml_attr_add(xml, xml->name, str);
    }
    
    return 0;
}


int sxt_bind_char(XMLN *xml, int d, char *val, int size)
{
    if(d) {   // xml -> struct
        if( xml && xml->data) {
            sscanf(xml->data, "%d", val);
        } else {
            return -1;
        }
    } else {  // struct -> xml
        char str[16];
        sprintf(str, "%d", *val);
        xml_attr_add(xml, xml->name, str);
    }
    
    return 0;
}


int sxt_bind_short(XMLN *xml, int d, short *val, int size)
{
    if(d) {   // xml -> struct
        if( xml && xml->data) {
            sscanf(xml->data, "%d", val);
        } else {
            return -1;
        }
    } else {  // struct -> xml
        char str[16];
        sprintf(str, "%d", *val);
        xml_attr_add(xml, xml->name, str);
    }
    
    return 0;
}


int sxt_bind_float(XMLN *xml, int d, float *val, int size)
{
    if(d) {   // xml -> struct
        if( xml && xml->data) {
            sscanf(xml->data, "%f", val);
        } else {
            return -1;
        }
    } else {  // struct -> xml
        char str[16];
        sprintf(str, "%f", *val);
        xml_attr_add(xml, xml->name, str);
    }
    
    return 0;
}


int sxt_bind_double(XMLN *xml, int d, double *val, int size)
{
    if(d) {   // xml -> struct
        if( xml && xml->data) {
            sscanf(xml->data, "%lf", val);
        } else {
            return -1;
        }
    } else {  // struct -> xml
        char str[16];
        sprintf(str, "%lf", *val);
        xml_attr_add(xml, xml->name, str);
    }
    
    return 0;
}


/* note: long long need string to do it. only support decimal*/
int sxt_bind_int64(XMLN *xml, int d, int64 *val, int size)
{
    if(d) {   // xml -> struct
        if( xml && xml->data) {
            sscanf(xml->data, "%lld", val);
        } else {
            return -1;
        }
    } else {  // struct -> xml
        char str[16];
        sprintf(str, "%lld", *val);
        xml_attr_add(xml, xml->name, str);
    }
    
    return 0;
}


int sxt_bind_string(XMLN *xml, int d, char *val, int size)
{
    if(d) {   // xml -> struct
        if( xml && xml->data) { // xml->f_attrib->data
        //if( xml && xml->f_attrib && xml->f_attrib->data) {
            strncpy((char*)val, xml->data, size);
            //strncpy((char*)val, xml->f_attrib->data, size);
        } else {
            return -1;
        }
    } else {  // struct -> xml
        char str[200];
        strncpy((char*)str, val, size);
        xml_attr_add(xml, xml->name, str);
    }
    
    return 0;
}












