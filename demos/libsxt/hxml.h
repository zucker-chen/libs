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

#ifndef	H_XML_H
#define	H_XML_H

#define XML_MAX_STACK_DEPTH	    1024
#define XML_MAX_ATTR_NUM		200

typedef struct xmlparser 
{
	char *	xmlstart;
	char *	xmlend;
	char *	ptr;		// pointer to current character
	int		xmlsize;
	char *	e_stack[XML_MAX_STACK_DEPTH];
	int		e_stack_index;
	char *	attr[XML_MAX_ATTR_NUM];
	void *	userdata;
	
	void (*startElement)(void * userdata, const char * name, const char ** attr);
	void (*endElement)(void * userdata, const char * name);
	void (*charData)(void * userdata, const char * str, int len);
} XMLPRS;

#ifdef __cplusplus
extern "C" {
#endif

int  hxml_parse_header(XMLPRS * parse);
int  hxml_parse_attr(XMLPRS * parse);
int  hxml_parse_element_end(XMLPRS * parse);
int  hxml_parse_element_start(XMLPRS * parse);
int  hxml_parse_element(XMLPRS * parse);
int  hxml_parse(XMLPRS * parse);

#ifdef __cplusplus
}
#endif

#endif	// H_XML_H



