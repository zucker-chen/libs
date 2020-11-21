#include "vofaplus.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <math.h>





static int vofaplus_client_conn(char *ip, int port)
{
	// int socket(int family, int type, int protocol);
	int client_socket = socket(AF_INET, SOCK_STREAM, 0);    // 创建一个Socket，返回大于0的文件描述符。
	if(client_socket == -1) {
		perror("socket");
		return -1;
	}

    // 定义sockaddr_in结构体
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;  							// 标记为IPv4
    addr.sin_port = htons(port);    						// 绑定端口
    addr.sin_addr.s_addr = inet_addr(ip);  					// 指定Server IP地址

    int conn = connect(client_socket, (struct sockaddr *)&addr, sizeof(addr));   // 向服务器发送连接请求。
    if(conn == -1) {
        perror("connect");
        return -1;
    }

	return client_socket;
}


/* input: ip, port
 * ouput: vofaplus_t
 * note: 
 */
vofaplus_t *vofaplus_init(char *ip, int port)
{
	vofaplus_t *vofa = calloc(1, sizeof(vofaplus_t));
	if (vofa == NULL) {
        perror("malloc");
        return NULL;
    }    
	
	strncpy(vofa->ip, ip, sizeof(vofa->ip));
	vofa->port = port;
	vofa->fd = vofaplus_client_conn(ip, port);
	
    return vofa;
}

/* input: vofa
 * ouput:
 * note: 
 */
int vofaplus_deinit(vofaplus_t *vofa)
{
    if (vofa != NULL) {
		close(vofa->fd);
		free(vofa);
		vofa = NULL;
	}
	
    return 0;
}


/* input: vofa, data, count
 * ouput:
 * note: count, 表示同时绘制数据曲线的数量， send数据会自动重连socket
 */
int vofaplus_data_send(vofaplus_t *vofa, float data[], int count)
{
	int i, ret;

	if (vofa == NULL || count <= 0)
		return -1;
	
	ret = dprintf(vofa->fd, "d:"); if (ret < 0) goto vofa_send_error;
	for (i = 0; i < count; i++)
	{
		ret = dprintf(vofa->fd, "%f", data[i]); if (ret < 0) goto vofa_send_error;
		if (i != count - 1) {
			ret = dprintf(vofa->fd, ","); if (ret < 0) goto vofa_send_error;
		}
	}
	ret = dprintf(vofa->fd, "\n"); fsync(vofa->fd); if (ret < 0) goto vofa_send_error;
	return 0;

vofa_send_error:
	if (vofa->fd > 0) close(vofa->fd);
	vofa->fd = vofaplus_client_conn(vofa->ip, vofa->port);
	return 1;
}





