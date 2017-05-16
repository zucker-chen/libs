
/*
 *	zucker.chen 2017.5.7
 *	kernel print cmd: cat /proc/kmsg
 *	This demo can redirect the /dev/console to current tty(like telnet.)
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>

#define __54321_TEST_TTYS0_DEBUG_12345__
#ifdef __54321_TEST_TTYS0_DEBUG_12345__
#define pri_dbg(M, ...) fprintf(stderr,"%s %d %s(): " M "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define pri_dbg(M, ...) do{}while(0)
#endif




/*
 * console2stdout:	redirect console to stdout
 * swich: 0, off; 1, on
 */
static int console2stdout(int swich)
{
    int tty = -1, ret = -1;
    char *tty_name = NULL;

    /* get current tty */
    tty_name = ttyname(STDOUT_FILENO);

    if(1 == swich) {
        /* redirect the console to the tty */
        tty = open(tty_name, O_RDONLY | O_WRONLY);
        ret = ioctl(tty, TIOCCONS);
        if (ret < 0) {
			pri_dbg("[ERROR] STDOUT_FILENO: ioctl(tty, TIOCCONS), %s", strerror(errno));
		}
    }
    else if(0 == swich) {
        /* reset the console */
        tty = open("/dev/console", O_RDONLY | O_WRONLY);
        ret = ioctl(tty, TIOCCONS);
        if (ret < 0) {
			pri_dbg("[ERROR] /dev/console: ioctl(tty, TIOCCONS), %s", strerror(errno));
		}
    }
    else
    {
        pri_dbg("Invalid argument.");
        return 0;
    }

    close(tty);	
	
	return 0;
}


static int socket_server_create(int port)
{
	int listenfd , connfd;	
	
	
	/*(1) 初始化监听套接字listenfd*/
	if((listenfd = socket(AF_INET , SOCK_STREAM , 0)) < 0)
	{
		pri_dbg("[ERROR] socket, %s", strerror(errno));
		return -1;
	}//if	


	/*(2) 设置服务器sockaddr_in结构*/
	struct sockaddr_in servaddr;
	bzero(&servaddr , sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //表明可接受任意IP地址
	servaddr.sin_port = htons(port);

	/*(3-) 设置端口可重复使用*/
	int reuse = 1;  
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	
	/*(3) 绑定套接字和端口*/
	if(bind(listenfd, (struct sockaddr*)&servaddr , sizeof(servaddr)) < 0)
	{
		pri_dbg("[ERROR] bind, %s", strerror(errno));
		return -1;
	}//if
	
	/*(4) 监听客户请求*/
	if(listen(listenfd, 5) < 0)
	{
		pri_dbg("[ERROR] listen, %s", strerror(errno));
		return -1;
	}//if

	/*(5) 接受客户请求*/
	struct sockaddr_in client;  
    socklen_t client_addrlength = sizeof(client);  
	pid_t childpid;
	for( ; ; )
	{
		if((connfd = accept(listenfd, (struct sockaddr *)&client , &client_addrlength)) < 0 )
		{
			pri_dbg("[ERROR] accept, %s", strerror(errno));
			return -1;
		}//if

		//新建子进程单独处理链接
		if((childpid = fork()) == 0) 
		{
			close(listenfd);
			//str_echo
			ssize_t n;
			#if 1
			char buff[128];
			while((n = read(connfd, buff , 128)) > 0)
			{
				write(connfd, buff , n);
			}
			return -1;
			#else
			//int fd = open("/proc/17503/fd/1", O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
			//dup2(fd, listenfd);
			
			
			#endif
		}//if
		close(connfd);
	}//for
	
	/*(6) 关闭监听套接字*/
	close(listenfd);
	
}


int main(int argc, char *argv[])
{
 
	//socket_server_create(6000);
	//return 0;
 
   if(argc < 2)
    {
        pri_dbg("miss argument");
        return 0;
    }

	if (!strcmp(argv[1], "on")) {
		console2stdout(1);
	} else if (!strcmp(argv[1], "off")) {
		console2stdout(0);
	}
	
	
    return 0;
}



