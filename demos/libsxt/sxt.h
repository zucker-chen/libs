#ifndef __SXT_H__
#define __SXT_H__

#include <xml_node.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* struct xml software version number */
#define SXT_SW_VERSION                "1.0.4"

#ifndef int64
    #define int64 long long
#endif

#define SXT_STRUCT2XML     0
#define SXT_XML2STRUCT     1


int sxt_bind_bool(XMLN *xml, int d, int *val, int size);
int sxt_bind_int(XMLN *xml, int d, int *val, int size);
int sxt_bind_char(XMLN *xml, int d, char *val, int size);
int sxt_bind_short(XMLN *xml, int d, short *val, int size);
int sxt_bind_float(XMLN *xml, int d, float *val, int size);
int sxt_bind_double(XMLN *xml, int d, double *val, int size);
int sxt_bind_int64(XMLN *xml, int d, int64 *val, int size);
int sxt_bind_string(XMLN *xml, int d, char *val, int size);



#define SXT_STRUCT(TYPE) static int sxt_bind_##TYPE(XMLN* xml, int d, TYPE *val, int size);     \
                         static int sxt__##TYPE(char *str_xml, int d, TYPE *val, int size);     \
                         int sxt_##TYPE(char *str_xml, int d, TYPE *val)                        \
                         {                                                                      \
                            XMLN* xml = NULL;                                                   \
                            if (d == SXT_XML2STRUCT) {                                          \
                                xml = xxx_hxml_parse(str_xml, strlen(str_xml));                      \
                            }                                                                   \
                            if (xml == NULL) {                                                  \
                                xml = xml_node_add(NULL, #TYPE);                                \
                            }                                                                   \
                            int ret = sxt_bind_##TYPE(xml, d, val, 0);                       \
                            if (ret >=0 && d == SXT_STRUCT2XML) {   /* struct -> xml */         \
                                int len = xml_calc_buf_len(xml);                                \
                                char *pxml = malloc(len);                                       \
                                xml_write_buf(xml, pxml);                                       \
                                strcpy(str_xml, pxml);                                          \
                                free(pxml); pxml = NULL;                                        \
                            }                                                                   \
                            xml_node_del(xml);                                                  \
                            if (ret < 0) return ret;                                            \
                         }                                                                      \
                         static int sxt_bind_##TYPE(XMLN* xml, int d, TYPE *val, int size)

                         
#define SXT_FIELD(TYPE, ELEMENT)                                                        \
    do {                                                                                \
        int ret = 0;                                                                    \
        if (d == SXT_XML2STRUCT) {   /* xml -> struct */                                \
            XMLN* axml = xml_node_get(xml, #ELEMENT);                                   \
            if (axml) {                                                                 \
                ret = sxt_bind_##TYPE(axml, d, &val->ELEMENT, 4);                       \
            } else { ret = -10; }                                                       \
        } else {   /* struct -> xml */                                                  \
            XMLN* axml = xml_node_add(xml, #ELEMENT);                                   \
            if (axml) {                                                                 \
                ret = sxt_bind_##TYPE(axml, d, &val->ELEMENT, 4);                       \
            } else { ret = -10; }                                                       \
        }                                                                               \
        if (ret < 0) return ret;                                                        \
    } while(0)


#define SXT_STRING(ELEMENT, SIZE)                                                       \
    do {                                                                                \
        int ret = 0;                                                                    \
        if (d == SXT_XML2STRUCT) {   /* xml -> struct */                                \
            XMLN* axml = xml_node_get(xml, #ELEMENT);                                   \
            if (axml) {                                                                 \
                ret = sxt_bind_string(axml, d, &val->ELEMENT, SIZE);                    \
            } else { ret = -20; }                                                       \
        } else {   /* struct -> xml */                                                  \
            XMLN* axml = xml_node_add(xml, #ELEMENT);                                   \
            if (axml) {                                                                 \
                ret = sxt_bind_string(axml, d, &val->ELEMENT, SIZE);                    \
            } else { ret = -20; }                                                       \
        }                                                                               \
        if (ret < 0) return ret;                                                        \
    } while(0)


#define SXT_ARRAY(TYPE, ELEMENT, SIZE)                                                                      \
    do {                                                                                                    \
        int ret = 0, index = 0, array_size = 0;                                                             \
        if (d == SXT_XML2STRUCT) {   /* xml -> struct */                                                    \
            XMLN* axml = xml_node_get(xml, #ELEMENT);                                                       \
            if (axml) {                                                                                     \
                while (axml && axml->name && soap_strcmp(axml->name, #ELEMENT) == 0)                        \
                {                                                                                           \
                    ret = sxt_bind_##TYPE(axml, d, &val->ELEMENT[index], sizeof(val->ELEMENT[index]));      \
                    index ++; axml = axml->next;                                                            \
                }                                                                                           \
            } else { ret = -30; }                                                                           \
        } else {   /* struct -> xml */                                                                      \
            if (xml) {                                                                                      \
                for (index = 0; index < SIZE; index++)                                                      \
                {                                                                                           \
                    XMLN* axml = xml_node_add(xml, #ELEMENT);                                               \
                    ret = sxt_bind_##TYPE(axml, d, &val->ELEMENT[index], SIZE);                             \
                }                                                                                           \
            } else { ret = -30; }                                                                           \
        }                                                                                                   \
        if (ret < 0) return ret;                                                                            \
    } while(0)



#ifdef __cplusplus
}
#endif

#endif /* __SXT_H__ */
