/***************************************************************************************
 *
 *  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
 *
 *  By downloading, copying, installing or using the software you agree to this license.
 *  If you do not agree to this license, do not download, install, 
 *  copy or use the software.
 *
 *  Copyright (C) 2014-2019, Happytimesoft Corporation, all rights reserved.
 *
 *  Redistribution and use in binary forms, with or without modification, are permitted.
 *
 *  Unless required by applicable law or agreed to in writing, software distributed 
 *  under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 *  CONDITIONS OF ANY KIND, either express or implied. See the License for the specific
 *  language governing permissions and limitations under the License.
 *
****************************************************************************************/

#ifndef	XML_NODE_H
#define	XML_NODE_H

/***************************************************************************************
 * 
 * XML node define
 *
***************************************************************************************/
#define NTYPE_TAG		0
#define NTYPE_ATTRIB	1
#define NTYPE_CDATA		2

#define NTYPE_LAST		2
#define NTYPE_UNDEF		-1

#ifndef XML_MAX_ATTR_NUM
    #define XML_MAX_ATTR_NUM		200
#endif

typedef struct XMLN
{
	const char *	name;
	unsigned int    type;
	char            data[XML_MAX_ATTR_NUM];
	int				dlen;
	int				finish;
	struct XMLN *	parent;
	struct XMLN *	f_child;
	struct XMLN *	l_child;
	struct XMLN *	prev;
	struct XMLN *	next;
	struct XMLN *	f_attrib;
	struct XMLN *	l_attrib;
} XMLN;


#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************************/
XMLN          * xml_node_add(XMLN * parent, char * name);
void	        xml_node_del(XMLN * p_node);
XMLN          * xml_node_get(XMLN * parent, const char * name);

int 	        soap_strcmp(const char * str1, const char * str2);
void            soap_strncpy(char * dest, const char * src, int size);

XMLN          * xml_node_soap_get(XMLN * parent, const char * name);

/***************************************************************************************/
XMLN          * xml_attr_add(XMLN * p_node, const char * name, const char * value);
void 	        xml_attr_del(XMLN * p_node, const char * name);
const char    * xml_attr_get(XMLN * p_node, const char * name);
XMLN          * xml_attr_node_get(XMLN * p_node, const char * name);

/***************************************************************************************/
void 	        xml_cdata_set(XMLN * p_node, const char * value, int len);

/***************************************************************************************/
int 	        xml_calc_buf_len(XMLN * p_node);
int 	        xml_write_buf(XMLN * p_node, char * xml_buf);
int 	        xml_nwrite_buf(XMLN * p_node, char * xml_buf, int buf_len);

/***************************************************************************************/
XMLN          * xml_parse(char * p_xml, int len);

#ifdef __cplusplus
}
#endif

#endif	// XML_NODE_H



