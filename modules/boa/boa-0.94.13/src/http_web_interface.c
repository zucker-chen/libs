/*
 *  http_web_interface.c
 *  used for processing request by cgi method (Support 'GET' and 'POST')
 *  Add by zucker.chen
 */

#include "boa.h"
#include "hashmap.h"
#include "http_web_interface.h"


#define HWI_DEBUG
#ifdef HWI_DEBUG
#define pri_dbg(M, ...) fprintf(stderr,"%s %d %s(): " M "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define pri_dbg(M, ...) do{}while(0)
#endif


static hwi_uri_args_t 	hwi_uri_args;
static char hwi_post_buf[SOCKETBUF_SIZE];
map_t hwi_map;



/*
 * decode uri string
 * return: 0 - success, -1 - fail
 */
static int hwi_uri_decoding(request *req, char *data)
{
    pri_dbg();
	static char *zero = "\0";
	char *buf, *buf_end;
    int count = 0;
	register char chbuf;

	if (data == NULL)
		return -1;
    pri_dbg("req->query = %s\n", data);	

    memset((char *)&hwi_uri_args, 0, sizeof(hwi_uri_args));
    buf = hwi_uri_args.uri_buf;
	buf_end = buf + HWI_MAX_CMD_LENGTH-1;
	hwi_uri_args.cmd_args[0].name = buf;
	hwi_uri_args.cmd_args[0].value = zero;
	
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
				hwi_uri_args.cmd_args[count].name = buf;
				hwi_uri_args.cmd_args[count].value = zero;
				break;
			case '=':
				*buf++ = '\0';
				hwi_uri_args.cmd_args[count].value = buf;
				break;
			default:
				*buf++ = chbuf;
				break;
		}
	} while (buf < buf_end);
	
	*buf = '\0';
	++count;
    hwi_uri_args.cmd_count = count;
    
	return 0;
}


static int hwi_vb_echo(request * req)
{
    pri_dbg();
	
	if (req->method == M_GET) {
		if (hwi_uri_decoding(req, req->query_string) < 0) {
			send_r_bad_request(req);
			return 1;
		}
		SQUASH_KA(req);
	    pri_dbg("req->query_string = %s, http_uri_cmd.uri_buf = %s", req->query_string, hwi_uri_args.uri_buf);
		//http_cmd_run(req, &http_uri_cmd, NULL);
	    //http_run_command(req, http_uri_cmd.cmd_args, http_uri_cmd.cmd_count);

	} else if (req->method == M_POST) {
		int bytes_read = 0;
		memset((void *)hwi_post_buf, 0, SOCKETBUF_SIZE);
		lseek(req->post_data_fd, SEEK_SET, 0);
		bytes_read = read(req->post_data_fd, (void *)hwi_post_buf, SOCKETBUF_SIZE);
		if (bytes_read <= 0) {
			send_r_bad_request(req);
			pri_dbg();
			return -1;
		}

	    pri_dbg("bytes_read = %d, data = %s\n", bytes_read, hwi_post_buf);
		
		//req_write(req, hwi_post_buf);
		// access post data.....
		//http_cmd_run(req, &http_uri_cmd, hwi_post_buf);

	} else {
		////
	}

	return 0;
}



#define HWI_URI_HASH_SIZE	(sizeof(hwi_uri_tab)/sizeof(hwi_uri_t))
static hwi_uri_t hwi_uri_tab[] =
{
    // 'GET' or POST'
    {"/vb/echo",            hwi_vb_echo,     AUTHORITY_VIEWER,       0, NULL },
    
    // add here ...
};



int hwi_handle(request * req)
{
    pri_dbg();
	hwi_uri_t *urit;
	int ret = hashmap_get(hwi_map, req->request_uri, (void**)(&urit));
	if (ret >= 0 && urit != NULL && urit->handler != NULL) {
		if ((urit->handler)(req) < 0) {
			pri_dbg("value != NULL && value->handler != NULL");
			return -1;
		}
	} else {
		pri_dbg("else");
		return -1;
	}

	return 0;
}


// http web interface
int hwi_init()
{
    pri_dbg();
	int index = 0, ret = -1;
	hwi_uri_t *urit = hwi_uri_tab;

	hwi_map = hashmap_new();

	for (index = 0; index < HWI_URI_HASH_SIZE; index++, urit++)
    {
        ret = hashmap_put(hwi_map, urit->name, urit);
		if (ret < 0) {
			pri_dbg("name = %s, ret = %d\n", urit->name, ret);
			return ret;
		}
	}

	return 0;
}




