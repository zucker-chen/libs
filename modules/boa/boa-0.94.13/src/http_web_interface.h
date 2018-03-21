/*
 *  user_cgi.h
 *  used for processing request by cgi method (Support 'GET' and 'POST')
 *  Add by zucker.chen
 */


#ifndef __HTTP_WEB_INTERFACE_H__
#define __HTTP_WEB_INTERFACE_H__


/*
 * http uri
 *
 */
#define UCGI_MAX_URI_HASH_SIZE		32

typedef enum ucgi_auth_e {
	AUTHORITY_ADMIN = 0,
	AUTHORITY_OPERATOR,
	AUTHORITY_VIEWER,
	AUTHORITY_NONE = 9
} ucgi_auth_t;

typedef struct hwi_uri_s {
	char                    *name;
	int                     (*handler)(struct request *);
	ucgi_auth_t             authority;
	int                     uri_flag;
	struct hwi_uri_s  		*next;
} hwi_uri_t;



/*
 * http optioin
 *
 */
#define HWI_MAX_CMD_HASH_SIZE		128     // max for http cmd number, like 'index.html, vb.htm...'
#define HWI_MAX_CMD_LENGTH	        1024    // max for uri query_string(behind at '?') length, eg:"http://localhost/index.html?arg1=111&arg2=222"
#define HWI_MAX_URI_CMD_NUM		16      // max for uri cmd number(number of '&' in query_string)
//#define HWI_MAX_POST_LENGTH	    1024    // max for uri post data length

typedef struct hwi_arg_s {
    char    *name;
    char    *value;
    int     flags;
} hwi_arg_t;

typedef struct hwi_uri_args_s {
	char            uri_buf[HWI_MAX_CMD_LENGTH];
	hwi_arg_t  	cmd_args[HWI_MAX_URI_CMD_NUM];
	int             cmd_count;
} hwi_uri_args_t;




int hwi_init(void);
int hwi_handle(request * req);


#endif
