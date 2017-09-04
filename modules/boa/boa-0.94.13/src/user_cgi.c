/*
 *  user_cgi.c
 *  used for processing request by cgi method (Support 'GET' and 'POST')
 *  Add by zucker.chen
 */

#include "boa.h"
#include "user_cgi.h"


#define UCGI_DEBUG
#ifdef UCGI_DEBUG
#define pri_dbg(M, ...) fprintf(stderr,"%s %d %s(): " M "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define pri_dbg(M, ...) do{}while(0)
#endif


static ucgi_uri_hash_tab_t *uri_hash;
static ucgi_cmd_hash_tab_t *cmd_hash;
static ucgi_http_uri_cmd_t http_uri_cmd;
static char ucgi_post_buf[SOCKETBUF_SIZE];


int ucgi_hello(request *req, ucgi_cmd_arg_t *arg)
{
    pri_dbg("arg.name = %s, arg.value = %s", arg->name, arg->value);
    char hello_name[] = "Hello World";


    if (req->method == M_GET) {
	    req->buffer_end += sprintf(req->buffer + req->buffer_end, "OK " "%s = %s\n", arg->name, hello_name);
    } else if (req->method == M_POST) {

    } else {
        return -1;
    }




	//req->buffer_end += sprintf(req->buffer + req->buffer_end, "NG " "%s\n", arg->name);

    return 0;
}



#define UCGI_CMD_HASH_TABLE_SIZE	(sizeof(http_cmd_tab)/sizeof(ucgi_http_cmd_t))
ucgi_http_cmd_t http_cmd_tab[] = {	

	//hello
	{"hello",                   ucgi_hello,              AUTHORITY_OPERATOR, 0,  1, NULL },

    // user add ...
};







int uri_decoding(request *req, char *data)
{
    pri_dbg();
	static char *zero = "\0";
	char *buf, *buf_end;
    int count = 0;
	register char chbuf;

	if (data == NULL)
		return -1;
    pri_dbg("req->query = %s\n", data);	

    memset((char *)&http_uri_cmd, 0, sizeof(http_uri_cmd));
    buf = http_uri_cmd.uri_buf;
	buf_end = buf + UCGI_MAX_CMD_LENGTH-1;
	http_uri_cmd.cmd_args[0].name = buf;
	http_uri_cmd.cmd_args[0].value = zero;
	
	do  
	{
		chbuf = *data++;
		
        /* already do it by unescape_uri in util.c
		switch ( chbuf )
		{
			case '+':
				chbuf = ' ';
				break;
			case '%':
				int hex = ascii_to_hex(data);
				if ( hex > 0 ) 
				{
					chbuf = (char)hex;
					data += 2;
				}
				break;
			case '\0':
			case '\n':
			case '\r':
			case ' ':
				*buf = '\0';
				count++;
				return 0;
		}
		*/
		
		switch ( chbuf )
		{
			case '&':
				count++;
				*buf++ = '\0';
				http_uri_cmd.cmd_args[count].name = buf;
				http_uri_cmd.cmd_args[count].value = zero;
				break;
			case '=':
				*buf++ = '\0';
				http_uri_cmd.cmd_args[count].value = buf;
				break;
			default:
				*buf++ = chbuf;
				break;
		}
	} while (buf < buf_end);
	
	*buf = '\0';
	++count;
    http_uri_cmd.cmd_count = count;
    
	return 0;
}


unsigned int hash_cal_value(char *name)
{
	unsigned int value = 0;

	while (*name)
		value = value * 37 + (unsigned int)(*name++);
	return value;
}

void hash_insert_entry(ucgi_cmd_hash_tab_t *table, ucgi_http_cmd_t *op)
{
	if (table->entry) {
		op->next = table->entry;
	}
	table->entry = op;
}

int hash_table_init(void)
{
    pri_dbg();
	int i;

	if ( (cmd_hash = (ucgi_cmd_hash_tab_t *)calloc(sizeof(ucgi_cmd_hash_tab_t), UCGI_MAX_CMD_HASH_SIZE)) == NULL) {
		return -1;
	}
	for (i = 0; i < UCGI_CMD_HASH_TABLE_SIZE; i++) {
		hash_insert_entry(cmd_hash+(hash_cal_value(http_cmd_tab[i].name)%UCGI_MAX_CMD_HASH_SIZE), http_cmd_tab+i);
	}

	return 0;
}


ucgi_http_cmd_t *http_cmd_search(char *arg)
{
    pri_dbg();
	ucgi_http_cmd_t *opt;

	opt = cmd_hash[hash_cal_value(arg)%UCGI_MAX_CMD_HASH_SIZE].entry;

	while (opt) {
		if ( strcmp(opt->name, arg) == 0 )
			return opt;
		opt = opt->next;
	}
	return NULL;
}

/*
 * download post data by socket, start at req->header_line
 *
 */
int ucgi_dl_post_data(request * req)
{
    pri_dbg();
    char *pdata = NULL;
    int data_len = 0;

    pdata = req->header_line;
    while (*pdata == ' ' || *pdata == '\r' || *pdata == '\n')
        pdata++;
    data_len = boa_atoi(req->content_length);

    
    pri_dbg("req->header_line = \n%s\ndata length = %d", pdata, data_len);

    //if (data_len > (req->header_end - req->header_line))
    // read socket
    
    
return 0;
    int bytes = 0, buf_bytes_left = 0;

    req->post_data_fd = open("./dl_port.txt", O_RDWR|O_CREAT);
 
    buf_bytes_left = boa_atoi(req->content_length);
    if (buf_bytes_left > SOCKETBUF_SIZE) {
        pri_dbg("buf_bytes_left > SOCKETBUF_SIZE");
    }

    memset((char *)ucgi_post_buf, 0, sizeof(ucgi_post_buf));
    bytes = read(req->fd, ucgi_post_buf, buf_bytes_left);
    if (bytes < 0) {
        if (errno == EINTR)
            pri_dbg("errno == EINTR");
        if (errno == EAGAIN || errno == EWOULDBLOCK) /* request blocked */
            pri_dbg("errno == EAGAIN || errno == EWOULDBLOCK");
        return -1;
    } else if (bytes == 0) {
        return 0;
    }

    pri_dbg("POST DATA:\n%s", ucgi_post_buf);

    return 0;
}


void http_run_command(request *req, ucgi_cmd_arg_t *arg, int num)
{
    pri_dbg("num = %d", num);
	ucgi_auth_t authority = AUTHORITY_ADMIN; 	// disable authority
	ucgi_http_cmd_t *option;
	int i;

	send_r_request_ok(req);     /* All's well */

    ucgi_dl_post_data(req);

	for (i = 0; i < num; i++) {
		//strtolower((unsigned char *)arg[i].name);  // convert the command argument to lowcase
		option = http_cmd_search(arg[i].name);
		if (option) {
			if ((authority <= option->authority)) {
				//arg[i].flags = 0;
				(*option->handler)(req, &arg[i]);
			}
			else {
				req_write(req, "UA ");
				req_write(req, arg[i].name);
				pri_dbg("http_run_command: Permission denied!!!");
			}
		}
		else {
			req_write(req, "UW ");
			req_write(req, arg[i].name);
		}
	}
}


/*
 * uri 'GET' interface
 * return: 0 - close it done, 1 - keep on ready
 */
int uri_vbg_htm(request * req)
{
    pri_dbg();

    if (req->method != M_GET) {
        send_r_bad_request(req);
        return 0;
    }

	req->is_cgi = CGI;
    if (uri_decoding(req, req->query_string) < 0) {
        send_r_bad_request(req);
        return 0;
    }
    SQUASH_KA(req);
    pri_dbg("req->query_string = %s, http_uri_cmd.uri_buf = %s", req->query_string, http_uri_cmd.uri_buf);
    http_run_command(req, http_uri_cmd.cmd_args, http_uri_cmd.cmd_count);
    req->status = DONE;
    
    return 1;
}


/*
 * uri 'POST' interface
 * return: 0 - close it done, 1 - keep on ready
 */
int uri_vbp_htm(request * req)
{
    pri_dbg();
    
    if (req->method != M_POST) {
        send_r_bad_request(req);
        return 0;
    }
    
	req->is_cgi = CGI;
    if (uri_decoding(req, req->query_string) < 0) {
        send_r_bad_request(req);
        return 0;
    }
    SQUASH_KA(req);

    /* POST only support one command */
    if (http_uri_cmd.cmd_count > 1) {
        pri_dbg("Warnning: http_uri_cmd.cmd_count (%d)>1, only access cmd: %s", http_uri_cmd.cmd_count, http_uri_cmd.cmd_args[0].name);
        http_uri_cmd.cmd_count = 1;
    }
    
    pri_dbg("req->query_string = %s, http_uri_cmd.uri_buf = %s", req->query_string, http_uri_cmd.uri_buf);
    http_run_command(req, http_uri_cmd.cmd_args, http_uri_cmd.cmd_count);
    req->status = DONE;
    
    return 0;
}




#define UCGI_URI_HASH_SIZE	(sizeof(ucgi_http_uri_tab)/sizeof(ucgi_http_uri_t))
static ucgi_http_uri_t ucgi_http_uri_tab[] =
{
    // user cgi 'GET' interface
    {"/vbg.htm",            uri_vbg_htm,     AUTHORITY_VIEWER,       0, NULL },
    {"/vbg.html",           uri_vbg_htm,     AUTHORITY_VIEWER,       0, NULL },
    // user cgi 'POST' interface
    {"/vbp.htm",            uri_vbp_htm,     AUTHORITY_VIEWER,       0, NULL },
    {"/vbp.html",           uri_vbp_htm,     AUTHORITY_VIEWER,       0, NULL },

    // add here ...
};

unsigned int ucgi_uri_hash_cal_value(char *name)
{
	    unsigned int value = 0;

	    while (*name)
	    	value = value * 37 + (unsigned int)(*name++);
	    return value;
}


void ucgi_uri_hash_insert_entry(ucgi_uri_hash_tab_t *table, ucgi_http_uri_t *arg)
{
	  if (table->entry) {
	  	arg->next = table->entry;
	  }
	  table->entry = arg;
}


int ucgi_uri_hash_table_init(void)
{
	  int i;

	if ( (uri_hash = (ucgi_uri_hash_tab_t *)calloc(sizeof(ucgi_uri_hash_tab_t), UCGI_MAX_URI_HASH_SIZE)) == NULL) {
		//dbg("ucgi_uri_hash_table_init: Allocate memory fail!!!\n");
		return -1;
	}
	for (i = 0; i< UCGI_URI_HASH_SIZE; i++) {
		ucgi_uri_hash_insert_entry(uri_hash+(ucgi_uri_hash_cal_value(ucgi_http_uri_tab[i].name)%UCGI_MAX_URI_HASH_SIZE), ucgi_http_uri_tab + i);
	}
	return 0;
}

ucgi_http_uri_t *ucgi_http_uri_search(char *arg)
{
    pri_dbg();
	ucgi_http_uri_t *opt;
	opt = uri_hash[ucgi_uri_hash_cal_value(arg)%UCGI_MAX_URI_HASH_SIZE].entry;

	while (opt) {
		if ( strcasecmp(opt->name, arg) == 0 )
			return opt;
		opt = opt->next;
	}

    pri_dbg("return NULL");

	return NULL;
}



int ucgi_init()
{
    if (ucgi_uri_hash_table_init() < 0) {
        return -1;
    }
    if (hash_table_init() < 0) {
        return -1;
    }

    return 0;
}


