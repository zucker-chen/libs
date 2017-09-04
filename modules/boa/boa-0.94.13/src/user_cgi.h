/*
 *  user_cgi.h
 *  used for processing request by cgi method (Support 'GET' and 'POST')
 *  Add by zucker.chen
 */




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

typedef struct ucgi_http_uri_s {
	char                    *name;
	int                     (*handler)(struct request *);
	ucgi_auth_t             authority;
	int                     uri_flag;
	struct ucgi_http_uri_s  *next;
} ucgi_http_uri_t;

typedef struct ucgi_uri_hash_tab_s {
	ucgi_http_uri_t *entry;
} ucgi_uri_hash_tab_t;




/*
 * http optioin
 *
 */
#define UCGI_MAX_CMD_HASH_SIZE		128     // max for http cmd number, like 'index.html, vb.htm...'
#define UCGI_MAX_CMD_LENGTH	        1024    // max for uri query_string(behind at '?') length, eg:"http://localhost/index.html?arg1=111&arg2=222"
#define UCGI_MAX_URI_CMD_NUM		16      // max for uri cmd number(number of '&' in query_string)
//#define UCGI_MAX_POST_LENGTH	    1024    // max for uri post data length

typedef struct ucgi_cmd_arg_s {
    char    *name;
    char    *value;
    int     flags;
} ucgi_cmd_arg_t;

typedef struct ucgi_cmd_arg {
	char            uri_buf[UCGI_MAX_CMD_LENGTH];
	ucgi_cmd_arg_t  cmd_args[UCGI_MAX_URI_CMD_NUM];
	int             cmd_count;
} ucgi_http_uri_cmd_t;

typedef struct ucgi_http_cmd_s {
	char			        *name;
	int			            (*handler)(request *, ucgi_cmd_arg_t *);
	ucgi_auth_t	            authority;
	int			            now;
	int			            visiable;
	struct ucgi_http_cmd_s  *next;
} ucgi_http_cmd_t;

typedef struct ucgi_cmd_hash_tab_s {
	ucgi_http_cmd_t *entry;
} ucgi_cmd_hash_tab_t;





int ucgi_init(void);
ucgi_http_uri_t *http_uri_search(char *arg);


