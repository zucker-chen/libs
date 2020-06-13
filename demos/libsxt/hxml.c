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

#include "xml_node.h"
#include "hxml.h"
#include <unistd.h>

/***************************************************************************************/
#define IS_WHITE_SPACE(c) 	((c==' ') || (c=='\t') || (c=='\r') || (c=='\n'))
#define IS_XMLH_START(ptr) 	(*ptr == '<' && *(ptr+1) == '?')
#define IS_XMLH_END(ptr) 	(*ptr == '?' && *(ptr+1) == '>')

/***************************************************************************************/
#define IS_ELEMS_END(ptr) 	(*ptr == '>' || (*ptr == '/' && *(ptr+1) == '>'))
#define IS_ELEME_START(ptr) (*ptr == '<' && *(ptr+1) == '/')

#define CHECK_XML_STACK_RET(parse) \
	do{ if(parse->e_stack_index >= XML_MAX_STACK_DEPTH || parse->e_stack_index < 0) return -1;}while(0)

#define RF_NO_END			0
#define RF_ELES_END			2
#define RF_ELEE_END			3

#define CUR_PARSE_START		1
#define CUR_PARSE_END		2

/***************************************************************************************/

int hxml_parse_header(XMLPRS * parse)
{
	char * ptr = parse->ptr;
	char * xmlend = parse->xmlend;

	while (IS_WHITE_SPACE(*ptr) && (ptr != xmlend))
	{
		ptr++;
	}
	
	if (ptr == xmlend) 
	{
		return -1;
	}
	
	if (!IS_XMLH_START(ptr))
	{
		return -1;
	}
	
	ptr+=2;
	
	while ((!IS_XMLH_END(ptr)) && (ptr != xmlend))
	{
		ptr++;
	}
	
	if (ptr == xmlend)
	{
		return -1;
	}
	
	ptr+=2;
	parse->ptr = ptr;

	return 0;
}

/***************************************************************************************/
int hxml_parse_attr(XMLPRS * parse)
{
	char * ptr = parse->ptr;
	char * xmlend = parse->xmlend;
	int ret = RF_NO_END;
	int cnt = 0;

	while (1)
	{
	    int index;
	    char * attr_name;
	    char * attr_value;
	    
		ret = RF_NO_END;

		while (IS_WHITE_SPACE(*ptr) && (ptr != xmlend))
		{
			ptr++;
		}
		
		if (ptr == xmlend)
		{
			return -1;
		}
		
		if (*ptr == '>')
		{
			*ptr = '\0';
			ptr++;
			ret = RF_ELES_END;	// node start finish
			break;
		}
		else if (*ptr == '/' && *(ptr+1) == '>')
		{
			*ptr = '\0';
			ptr += 2;
			ret = RF_ELEE_END;	// node end finish
			break;
		}

		attr_name = ptr;
		
		while (*ptr != '=' && (!IS_ELEMS_END(ptr)) && ptr != xmlend)
		{
			ptr++;
		}
		
		if (ptr == xmlend)
		{
			return -1;
		}
		
		if (IS_ELEMS_END(ptr))
		{
			if (*ptr == '>')
			{
				ret = RF_ELES_END;
				*ptr = '\0'; 
				ptr++;
			}
			else if (*ptr == '/' && *(ptr+1) == '>')
			{
				ret = RF_ELEE_END;
				*ptr = '\0'; 
				ptr+=2;
			}
			
			break;
		}

		*ptr = '\0';	// '=' --> '\0'
		ptr++;

		attr_value = ptr;
		
		if (*ptr == '"')
		{
			attr_value++;
			ptr++;
			
			while (*ptr != '"' && ptr != xmlend) 
			{
				ptr++;
			}
			
			if (ptr == xmlend)
			{
			    return -1;
			}
			
			*ptr = '\0'; // '"' --> '\0'
			ptr++;
		}
		else
		{
			while ((!IS_WHITE_SPACE(*ptr)) && (!IS_ELEMS_END(ptr)) && ptr != xmlend)
			{
				ptr++;
			}
			
			if (ptr == xmlend)
			{
				return -1;
			}
			
			if (IS_WHITE_SPACE(*ptr))
			{
				*ptr = '\0';
				ptr++;
			}
			else
			{
				if (*ptr == '>')
				{
					ret = RF_ELES_END;
					*ptr = '\0';
					ptr++;
				}
				else if (*ptr == '/' && *(ptr+1) == '>')
				{
					ret = RF_ELEE_END;
					*ptr = '\0'; 
					ptr+=2;
				}
			}
		}

		index = cnt << 1;

		if (index < XML_MAX_ATTR_NUM - 2)
		{
		    parse->attr[index] = attr_name;
		    parse->attr[index+1] = attr_value;
		}
		
		cnt++;

		if (ret > RF_NO_END)
		{
			break;
		}	
	}

	parse->ptr = ptr;
	
	return ret;
}

int hxml_parse_element_end(XMLPRS * parse)
{
    char * xmlend;
	char * stack_name;
	char * end_name;

	stack_name = parse->e_stack[parse->e_stack_index];
	if (stack_name == NULL)
	{
		return -1;
	}
	
	xmlend = parse->xmlend;

	while (IS_WHITE_SPACE(*(parse->ptr)) && ((parse->ptr) != xmlend))
	{
		(parse->ptr)++;
	}
	
	if ((parse->ptr) == xmlend)
	{
		return -1;
	}
	
	end_name = (parse->ptr);
	
	while ((!IS_WHITE_SPACE(*(parse->ptr))) && ((parse->ptr) != xmlend) && (*(parse->ptr) != '>'))
	{
		(parse->ptr)++;
	}
	
	if ((parse->ptr) == xmlend) 
	{
		return -1;
	}
	
	if (IS_WHITE_SPACE(*(parse->ptr)))
	{
		*(parse->ptr) = '\0';
		(parse->ptr)++;
		
		while (IS_WHITE_SPACE(*(parse->ptr)) && ((parse->ptr) != xmlend)) 
		{
			(parse->ptr)++;
		}
		
		if ((parse->ptr) == xmlend)
		{
			return -1;
		}
	}

	if (*(parse->ptr) != '>')
	{
		return -1;
	}
	
	*(parse->ptr) = '\0';
	(parse->ptr)++;

	if (strcasecmp(end_name, stack_name) != 0)
	{
		printf("%s, cur name[%s] != stack name[%s]!!!\r\n", __FUNCTION__, end_name, stack_name);
		return -1;
	}

	if (parse->endElement)
	{
		parse->endElement(parse->userdata, end_name);
	}
	
	parse->e_stack[parse->e_stack_index] = NULL;
	parse->e_stack_index--;
	
	CHECK_XML_STACK_RET(parse);

	return 0;
}

int hxml_parse_element_start(XMLPRS * parse)
{
	char * xmlend;
	char * element_name;

	xmlend = parse->xmlend;

	while (IS_WHITE_SPACE(*(parse->ptr)) && ((parse->ptr) != xmlend))
	{
		(parse->ptr)++;
	}
	
	if ((parse->ptr) == xmlend)
	{
		return -1;
	}
	
	element_name = (parse->ptr);
	
	while ((!IS_WHITE_SPACE(*(parse->ptr))) && ((parse->ptr) != xmlend) && (!IS_ELEMS_END((parse->ptr)))) 
	{
		(parse->ptr)++;
	}
	
	if ((parse->ptr) == xmlend)
	{
		return -1;
	}
	
	parse->e_stack_index++; 
	parse->e_stack[parse->e_stack_index] = element_name;
	
	CHECK_XML_STACK_RET(parse);

	if (*(parse->ptr) == '>')
	{
		*(parse->ptr) = '\0';
		(parse->ptr)++;
		
		if (parse->startElement)
		{
			parse->startElement(parse->userdata, element_name, (const char **)parse->attr);
		}
		
		return RF_ELES_END;
	}
	else if (*(parse->ptr) == '/' && *((parse->ptr)+1) == '>')
	{
		*(parse->ptr) = '\0'; 
		(parse->ptr)+=2;
		
		if (parse->startElement)
		{
			parse->startElement(parse->userdata, element_name, (const char **)parse->attr);
		}
		
		if (parse->endElement)
		{
			parse->endElement(parse->userdata, element_name);
		}
		
		parse->e_stack[parse->e_stack_index] = NULL; 
		parse->e_stack_index--;
		
		CHECK_XML_STACK_RET(parse);
		
		return RF_ELEE_END;
	}
	else
	{
	    int ret;
	    
		*(parse->ptr) = '\0'; 
		(parse->ptr)++;

		ret = hxml_parse_attr(parse);
		if (ret < 0)
		{
			return -1;
		}
		
		if (parse->startElement)
		{
			parse->startElement(parse->userdata, element_name, (const char **)parse->attr);
		}
		
		memset(parse->attr, 0, sizeof(parse->attr));

		if (ret == RF_ELEE_END)
		{
			if (parse->endElement)
			{
				parse->endElement(parse->userdata, element_name);
			}
			
			parse->e_stack[parse->e_stack_index] = NULL;
			parse->e_stack_index--;
			
			CHECK_XML_STACK_RET(parse);
		}

		return ret;
	}
}

int hxml_parse_element(XMLPRS * parse)
{
	char * xmlend = parse->xmlend;
	int parse_type = CUR_PARSE_START;
	
	while (1)
	{
		int ret = RF_NO_END;

xml_parse_type:

		while (IS_WHITE_SPACE(*(parse->ptr)) && ((parse->ptr) != xmlend))
		{
			(parse->ptr)++;
		}
		
		if ((parse->ptr) == xmlend)
		{
			if (parse->e_stack_index == 0)
			{
				return 0;
			}
			
			return -1;
		}


		if (*(parse->ptr) == '<' && *((parse->ptr)+1) == '/')
		{
			(parse->ptr)+=2;
			parse_type = CUR_PARSE_END;
		}
		else if (*(parse->ptr) == '<' && *((parse->ptr)+1) == '!' && *((parse->ptr)+2) == '-' && *((parse->ptr)+3) == '-')
		{
		    (parse->ptr)+=4;

            while (((parse->ptr) != xmlend) && ((parse->ptr)+1) != xmlend && ((parse->ptr)+2) != xmlend)
            {
                if (*(parse->ptr) == '-' && *((parse->ptr)+1) == '-' && *((parse->ptr)+2) == '>')
    		    {
    		        (parse->ptr)+=3;
    		        goto xml_parse_type;
    		    }

    		    (parse->ptr)++;
            }
		}
		else if (*(parse->ptr) == '<')
		{
			(parse->ptr)++;
			parse_type = CUR_PARSE_START;
		}
		else
		{
			return -1;
		}

//xml_parse_point:

		while (IS_WHITE_SPACE(*(parse->ptr)) && ((parse->ptr) != xmlend))
		{
			(parse->ptr)++;
		}
		
		if ((parse->ptr) == xmlend)
		{
			if (parse->e_stack_index == 0)
			{
				return 0;
			}
			
			return -1;
		}

		if (parse_type == CUR_PARSE_END)
		{
			ret = hxml_parse_element_end(parse);
			if (ret < 0)
			{
				return -1;
			}
			
			if (parse->e_stack_index == 0)
			{
				return 0;
			}
			
			parse_type = CUR_PARSE_START;
		}
		else // CUR_PARSE_START
		{
		    int len;
		    char * cdata_ptr;
		    
			ret = hxml_parse_element_start(parse);
			if (ret < 0)
			{
				return -1;
			}
			
			if (ret == RF_ELEE_END)
			{
				if (parse->e_stack_index == 0)
				{
					return 0;
				}	

				parse_type = CUR_PARSE_START;
				
				goto xml_parse_type;
			}

			while (IS_WHITE_SPACE(*(parse->ptr)) && ((parse->ptr) != xmlend))
			{
				(parse->ptr)++;
			}
			
			if ((parse->ptr) == xmlend)
			{
				return -1;
			}
			
			if (*(parse->ptr) == '<') 
			{
				goto xml_parse_type;
			}

			cdata_ptr = (parse->ptr);
			
			while (*(parse->ptr) != '<' && (parse->ptr) != xmlend)
			{
				(parse->ptr)++;
			}
			
			if ((parse->ptr) == xmlend)
			{
				return -1;
			}

			len = (int)(parse->ptr - cdata_ptr);
			if (len > 0)	
			{
				*(parse->ptr) = '\0';
				(parse->ptr)++;
				
				if (parse->charData)
				{
					parse->charData(parse->userdata, cdata_ptr, len);
				}
				
				if (*(parse->ptr) != '/')
				{
					return -1;
				}
				
				(parse->ptr)++;

				if (hxml_parse_element_end(parse) < 0)
				{
					return -1;
				}	
			}

			goto xml_parse_type;
		}
	}

	return 0;
}

int hxml_parse(XMLPRS * parse)
{
	int ret =  hxml_parse_header(parse);
	if (ret < 0)
	{
		printf("%s, hxml parse xml header failed!!!\r\n", __FUNCTION__);
	}

	return hxml_parse_element(parse);
}



