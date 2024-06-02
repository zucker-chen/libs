/* J. David's webserver */
/* This is a simple webserver.
 * Created November 1999 by J. David Blackstone.
 * CSE 4344 (Network concepts), Prof. Zeigler
 * University of Texas at Arlington
 */
/* This program compiles for Sparc Solaris 2.6.
 * To compile for Linux:
 *  1) Comment out the #include <pthread.h> line.
 *  2) Comment out the line that defines the variable newthread.
 *  3) Comment out the two lines that run pthread_create().
 *  4) Uncomment the line that runs accept_request().
 *  5) Remove -lsocket from the Makefile.
 */

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
//#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>

//C库函数int isspace(int c),检查传递的字符是否是空白。也就是判断是否为空格(' ')、
//定位字符(' \t ')、CR(' \r ')、换行(' \n ')、垂直定位字符(' \v ')或翻页(' \f ')的情况。
//若参数c 为空白字符，则返回非 0，否则返回 0。
#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: lzx-tiny-httpd/0.1.0\r\n"

static void accept_request(int);
static void bad_request(int);
static void cat(int, FILE *);
static void cannot_execute(int);
static void error_die(const char *);
static void execute_cgi(int, const char *, const char *, const char *);
static int get_line(int, char *, int);
static void headers(int, const char *);
static void not_found(int);
static void serve_file(int, const char *);
static int startup(u_short *);
static void unimplemented(int);
static void readBufferBeforeSend(int);

/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the connfd */
/**********************************************************************/
static void accept_request(int connfd)
{
    char buf[1024]; //缓冲从socket中读取的字节
    //int numchars; //读取字节数
    char method[255]; //请求方法
    char url[255]; //请求的url，包括参数
    char path[512]; //文件路径,不包括参数
    size_t i, j;
    struct stat st;
    int cgi = 0;      /* 如果确定是cgi请求需要把这个变量置为1 */
    char *query_string = NULL; //参数

    //从socket中读取一行数据
    //这里就是读取请求行(GET /index.html HTTP/1.1)，行数据放到buf中，长度返回给numchars
    get_line(connfd, buf, sizeof(buf));

    i = 0; j = 0; //这里使用两个变量i,j因为后边i被重置为0了。j用来保持请求行的seek
    while (!ISspace(buf[j]) && (i < sizeof(method) - 1)) //小于method-1是因为最后一位要放\0
    {
        method[i] = buf[j]; //获取请求方法放入method
        i++; j++;
    }
    method[i] = '\0';

    //如果请求的方法不是 GET 或 POST 任意一个的话就直接发送 response 告诉客户端没实现该方法
    //strcasecmp 忽略大小写
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) {
        //清除缓冲区消息头和消息体
        readBufferBeforeSend(connfd);

        unimplemented(connfd);

        close(connfd); //TODO 在调用没有实现的方法后没有关闭链接,这里要把connfd关闭

        return;
    }

    //如果是 POST 方法就将 cgi 标志变量置一(true)
    if (strcasecmp(method, "POST") == 0) {
        cgi = 1;
    }

    i = 0;
    //跳过所有的空白字符
    //此时buf里装的是请求行的内容(GET /index.html HTTP/1.1),而j的指针是读完GET之后的位置
    //所以跳过空格后获取的就是请求url了
    while (ISspace(buf[j]) && (j < sizeof(buf)))
        j++;

    //然后把 URL 读出来放到 url 数组中,url结尾要补充\0,所以长度要-1,buf已经存在了,判断长度只是避免越界
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
    {
        url[i] = buf[j];
        i++; j++;
    }
    url[i] = '\0';

    //如果这个请求是一个 GET 方法的话
    //TODO 对于不是GET方法的请求其实也是需要解析query_string的，
    //TODO 否则对于POST:http://10.33.106.82:8008/check.cgi?name=foo 带参数的情况path解析是失败的。
    //TODO 但这里只是简单的区分GET和POST请求,这个就不考虑了吧
    if (strcasecmp(method, "GET") == 0) {
        //用一个指针指向 url
        query_string = url;

        //去遍历这个 url，跳过字符 ？前面的所有字符，如果遍历完毕也没找到字符 ？则退出循环
        while ((*query_string != '?') && (*query_string != '\0'))
            query_string++;

        //退出循环后检查当前的字符是 ？还是字符串(url)的结尾
        //因为如果存在?的话经过上边的循环query_string的指针正好在？处，如果没有？的话query_string指针正好在url字符串的结尾\0处
        if (*query_string == '?')
        {
            //如果是 ？ 的话，证明这个请求需要调用 cgi，将cgi标志变量置一(true),因为请求静态文件不用参数的。
            cgi = 1;
            //从字符 ？ 处把字符串 url 给分隔为两份
            *query_string = '\0'; //把url的?改为\0，这样url只是？的前边部分了。
            //使指针指向字符 ？后面的那个字符
            query_string++;
        }
    }

    //将url字符串(只包含url,参数已经被截断了)
    //因为url的第一个字符是/，所以要加.表示当前目录
    sprintf(path, ".%s", url);

    //如果 path 数组中的这个字符串的最后一个字符是以字符 / 结尾的话，就拼接上一个"index.html"的字符串。首页的意思
    if (path[strlen(path) - 1] == '/') {
        strcat(path, "index.html");
    }

    //在系统上去查询该文件是否存在
    //int stat(const char * file_name, struct stat *buf);
    //stat()用来将参数file_name 所指的文件状态, 复制到参数buf 所指的结构中。执行成功则返回0，失败返回-1，错误代码存于errno。
    if (stat(path, &st) == -1) {
        //如果不存在，那把这次 http 的请求后续的内容(head 和 body)全部读完并忽略
        readBufferBeforeSend(connfd);

        //返回方法不存在
        not_found(connfd);
    } else {
        //文件存在，那去跟常量S_IFMT相与，相与之后的值可以用来判断该文件是什么类型的
        if ((st.st_mode & S_IFMT) == S_IFDIR)
            //如果这个文件是个目录，那就需要再在 path 后面拼接一个"/index.html"的字符串
            //注意此时访问需要http://10.33.106.82:8008/static/，后边不带/不能识别为目录
            strcat(path, "/index.html");

		#if 0
        //S_IXUSR:用户可以执行
        // S_IXGRP:组可以执行
        // S_IXOTH:其它人可以执行
        // 如果这个文件是一个可执行文件，不论是属于用户/组/其他这三者类型的，就将 cgi 标志变量置一
		// 该逻辑对下载文件不友好
        if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH)) {
            cgi = 1;
        }
		#else
		if (strstr(path, "./cgi") == path) {
			printf("This is cgi request.\n");
            cgi = 1;
		}
		#endif

        if (!cgi) {
            //静态解析
            serve_file(connfd, path);
        } else {
            //CGI解析
            execute_cgi(connfd, path, method, query_string);
        }
    }

    close(connfd);
}

/**********************************************************************/
/* Inform the connfd that a request it has made has a problem.
 * Parameters: connfd socket */
/**********************************************************************/
static void bad_request(int connfd)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(connfd, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(connfd, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(connfd, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(connfd, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(connfd, buf, sizeof(buf), 0);
}

static int tcp_send(int fd, const void* data, size_t bytes)
{
	int ret = 0;
	int times = 500;
	size_t sent_bytes = 0;
	int align_bytes = 4096;
	int remain_bytes = bytes;
	int write_bytes = 0;

	do {
		write_bytes = remain_bytes > align_bytes ? align_bytes : remain_bytes;
		ret = send(fd, data + sent_bytes, write_bytes, MSG_NOSIGNAL);
		if (ret < 0) {
			if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
				usleep(1000);
				times--;
				continue;
			}
			return -1;
		}
		remain_bytes -= ret;
		sent_bytes += ret;
	} while (remain_bytes > 0 && times > 0);
	
	return bytes == sent_bytes ? 0 : -1;
}


/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the connfd socket descriptor
 *             FILE pointer for the file to cat */
/**********************************************************************/
static void cat(int connfd, FILE *resource)
{
    char buf[1024];
	int bytes = 0;

    //从文件描述符中读取指定内容
    while (!feof(resource))
    {
		bytes = fread(buf, 1, sizeof(buf), resource);
		if (0 != tcp_send(connfd, buf, bytes)) {
			unimplemented(connfd);
			break;
		}
    }
}

/**********************************************************************/
/* Inform the connfd that a CGI script could not be executed.
 * Parameter: the connfd socket descriptor. */
/**********************************************************************/
static void cannot_execute(int connfd)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(connfd, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
static void error_die(const char *sc)
{
    // #include <stdio.h>,void perror(char *string);
    // 在输出错误信息前加上字符串sc:
    perror(sc);
    exit(1);
}

/**********************************************************************/
/* 执行一个cgi脚本,设置一些环境变量

 * Parameters: connfd   socket descriptor
 *             path     to the CGI script */
/**********************************************************************/
static void execute_cgi(int connfd, const char *path, const char *method, const char *query_string)
{
    char buf[1024];
    int cgi_output[2]; //重定向输出的管道
    int cgi_input[2]; //重定向输入的管道
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;
    int content_length = -1;

    buf[0] = 'A'; buf[1] = '\0';

    //如果是 http 请求是 GET 方法的话读取并忽略请求剩下的内容
    if (strcasecmp(method, "GET") == 0) {
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
            numchars = get_line(connfd, buf, sizeof(buf));
    } else {
        //只有 POST 方法才继续读内容
        numchars = get_line(connfd, buf, sizeof(buf));
        //这个循环的目的是读出指示 body 长度大小的参数，并记录 body 的长度大小。其余的 header 里面的参数一律忽略
        //注意这里只读完 header 的内容，body 的内容没有读
        while ((numchars > 0) && strcmp("\n", buf))
        {
            buf[15] = '\0';
            if (strcasecmp(buf, "Content-Length:") == 0)
                content_length = atoi(&(buf[16])); //记录 body 的长度大小
            numchars = get_line(connfd, buf, sizeof(buf));
        }

        //如果 http 请求的 header 没有指示 body 长度大小的参数，则报错返回
        if (content_length == -1) {
            bad_request(connfd);
            return;
        }
    }

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(connfd, buf, strlen(buf), 0);

    //下面这里创建两个管道，用于两个进程间通信
    //管道是一种进程间通信的方法，但是管道是半双工的，就是流只能向一个方向流动，
    //所以如果想把一个进程的输入和输出都重定向的话，则需要建立两条管道，并且需要关闭不需要的管道的读端或者写端
    if (pipe(cgi_output) < 0) {
        cannot_execute(connfd);
        return;
    }
    if (pipe(cgi_input) < 0) {
        cannot_execute(connfd);
        return;
    }

    //创建一个子进程
    if ( (pid = fork()) < 0 ) {
        cannot_execute(connfd);
        return;
    }

    //子进程用来执行 cgi 脚本
    if (pid == 0)
    {
        /* child: CGI script */
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        //将子进程的输出由标准输出重定向到 cgi_ouput 的管道写端上
        dup2(cgi_output[1], 1);
        //将子进程的输入由标准输入重定向到 cgi_input 的管道读端上
        dup2(cgi_input[0], 0);
        //关闭 cgi_ouput 管道的读端与cgi_input 管道的写端
        close(cgi_output[0]);
        close(cgi_input[1]);

        //构造一个环境变量
        sprintf(meth_env, "REQUEST_METHOD=%s", method);

        //将这个环境变量加进子进程的运行环境中
        putenv(meth_env);

        //根据http 请求的不同方法，构造并存储不同的环境变量
        if (strcasecmp(method, "GET") == 0) {
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        } else {
            /* POST */
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }

        execl(path, path, NULL);
        exit(0);

    } else {    /* parent */
        //父进程则关闭了 cgi_output管道的写端和 cgi_input 管道的读端
        close(cgi_output[1]);
        close(cgi_input[0]);

        //如果是 POST 方法的话就继续读 body 的内容，并写到 cgi_input 管道里让子进程去读
        if (strcasecmp(method, "POST") == 0) {

            for (i = 0; i < content_length; i++) {
                recv(connfd, &c, 1, 0);
                write(cgi_input[1], &c, 1);
            }
        }

        //然后从 cgi_output 管道中读子进程的输出，并发送到客户端去
        while (read(cgi_output[0], &c, 1) > 0)
            send(connfd, &c, 1, 0);

        //关闭管道
        close(cgi_output[0]);
        close(cgi_input[1]);
        //等待子进程的退出
        waitpid(pid, &status, 0);
    }
}

/**********************************************************************/
/* 从socket读取一行。
 * 如果行以换行(\n)或者回车(\r)或者CRLF(\r\n)做为结束,就使用'\0'停止字符串读取
 * 如果buffer读取完都没有发现换行符,使用'\0'结束字符串
 * 如果上边三个任意一个行终止符被读出,都会被替换为\n,并且在末尾补充'\0'
 * 意思就是不管换行符是\r还是\n还是\r\n，都会被替换为\n\0
 *
 * 关于回车和换行：
 * '\r'是回车，使光标到行首，（carriage return）
 * '\n'是换行，使光标下移一格，（line feed / newline）
 * '\r\n'就是CRLF啦
 *
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
static int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0'; //补充到结尾的字符
    int n;

    while ((i < size - 1) && (c != '\n')) //因为字符串的最后一位要使用'\0'结束，所以size需要 -1
    {
        //从sock中一次读一个字符，循环读
        //int recv(int s, void *buf, int len, unsigned int flags);
        //recv()用来接收远端主机经指定的socket传来的数据, 并把数据存到由参数buf指向的内存空间, 参数len为可接收数据的最大长度.
        n = recv(sock, &c, 1, 0);
        if (n > 0) {
            if (c == '\r') { //这个if里边处理了\r和\r\n结尾的情况,把c统一改为\n,如果是\n结尾那无需处理直接就赋值给buf啦

                //参数 flags 一般设0，MSG_PEEK表示返回来的数据并不会在系统内删除, 如果再调用recv()会返回相同的数据内容.
                //这个选项用于测试下一个字符是不是\n，并不会把当前读出的字符从缓冲区中删掉
                n = recv(sock, &c, 1, MSG_PEEK);
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n')) //如果是\n，就把下个字符读取出来,如果不是\n就把\r改为\n
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }

            buf[i] = c;
            i++;
        } else {
            //读取失败直接赋值\n
            c = '\n';
        }
    }

    //读取完一行数据后在获得的字符串末尾补上\0
    buf[i] = '\0';

    return (i);
}

/**
 * 处理请求方法不是GET和POST的情况
 * @param connfd [description]
 */
static void readBufferBeforeSend(int connfd)
{
    int numchars = 1;
    char buf[1024];

    int content_length = -1;    //post要读取的长度
    int i;  //for循环准备
    char c; //for循环读取消息体准备

    buf[0] = 'A'; buf[1] = '\0';

    //对于GET和HEAD方法,只有请求头部分,不会发送请求体
    //注意HEAD方法只会返回请求头,不返回请求体,页面上看不到输出是正常的,只要返回状态码正确就行了。
    //除了GET和HEAD其他方法都会发送请求体,这里先读出请求头,根据Content-Length判断是否有请求体

    //strcmp若参数s1 和s2 字符串相同则返回0。s1 若大于s2 则返回大于0 的值。s1 若小于s2 则返回小于0 的值。
    //完全等于"\n"代表当前行是消息头和消息体之前的那个空行
    while ((numchars > 0) && strcmp("\n", buf)) 
    {
        buf[15] = '\0';
        if (strcasecmp(buf, "Content-Length:") == 0) {
            content_length = atoi(&(buf[16])); //把ascii码转为整形,http协议传输的是ascii码
        }
        numchars = get_line(connfd, buf, sizeof(buf));
    }

    //读出消息体并忽略,这里不能多读也不能少读
    if(content_length != -1){
        for (i = 0; i < content_length; i++) {
            recv(connfd, &c, 1, 0);
        }
    }
}

/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/
static void headers(int connfd, const char *filename)
{
    char buf[1024];
    (void)filename;  /* could use filename to determine file type */

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(connfd, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(connfd, buf, strlen(buf), 0);
	if (strstr(filename, ".htm")) {
		sprintf(buf, "Content-type: text/html\r\n");
	} else {
		sprintf(buf, "Content-Type: application/octet-stream\r\n");
	}
    send(connfd, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(connfd, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Give a connfd a 404 not found status message. */
/**********************************************************************/
static void not_found(int connfd)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(connfd, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Send a regular file to the connfd.  Use headers, and report
 * errors to connfd if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
static void serve_file(int connfd, const char *filename)
{
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];

    buf[0] = 'A'; buf[1] = '\0';
    
    //循环读出请求头内容并忽略,这里也可以调用我添加的函数readBufferBeforeSend完成.
    while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
        numchars = get_line(connfd, buf, sizeof(buf));

    //打开这个传进来的这个路径所指的文件
    resource = fopen(filename, "r");
    if (resource == NULL) {
        not_found(connfd);
    } else {
        //打开成功后，将这个文件的基本信息封装成 response 的头部(header)并返回
        headers(connfd, filename);
        //接着把这个文件的内容读出来作为 response 的 body 发送到客户端
        cat(connfd, resource);
    }

    fclose(resource);
}

/**********************************************************************/
/* 这个函数在指定的端口上启动一个监听进程，如果端口是0，就动态分配一个
 * 并把值赋给变量port
 *
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
static int startup(u_short *port)
{
    int httpd = 0;
    struct sockaddr_in name;

    //建立TCP SOCKET,PF_INET等同于AF_INET
    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        error_die("socket");

    //在修改源码后重新启动总是提示bind: Address already in use,使用tcpreuse解决
    int reuse = 1;
    if (setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        error_die("setsockopet");
    }

    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;

    //<arpa/inet.h>，将*port 转换成以网络字节序表示的16位整数
    //这里port已被我修改为8008,如果为0的话内核会在bind时自动分配一个端口号
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
        error_die("bind");

    //这里是原来的代码逻辑,如果port是0的话通过bind后内核会随机分配一个端口号给当前的socket连接,
    //获取这个端口号给port变量
    if (*port == 0)  /* if dynamically allocating a port */
    {
        int namelen = sizeof(name);
        if (getsockname(httpd, (struct sockaddr *)&name, (socklen_t *)&namelen) == -1)
            error_die("getsockname");
        *port = ntohs(name.sin_port);
    }


    //启动监听，backlog为5
    if (listen(httpd, 5) < 0)
        error_die("listen");
    return (httpd);
}

/**********************************************************************/
/* 通知connfd请求的web方法没有实现                                    */
/**********************************************************************/
static void unimplemented(int connfd)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(connfd, buf, strlen(buf), 0);
}

/**********************************************************************/

static void usage()
{
  fprintf(stderr, "Usage: httpd [port] [root]\n");
  fprintf(stderr, "       Like: httpd &\n");
  fprintf(stderr, "             httpd 8008 /tmp &\n");
}


int main(int argc, char *argv[])
{
    int listenfd = -1;
    u_short port = 80;
	char *root_path = "./";
    int connfd = -1;

    struct sockaddr_in client;
    int client_len = sizeof(client);

	usage();
	if (argc > 1) {
		port = (u_short)atoi(argv[1]);
	}
	if (argc > 2) {
		root_path = argv[2];
	}

    //设置httpd的根目录路径
    if (chdir(root_path) == -1) {
        printf("Could not chdir to \"%s\": aborting\n", root_path);
        return (-1);
    }
    printf("httpd root path: %s\n", root_path);

    //绑定监听端口
    listenfd = startup(&port);
    printf("httpd running on port %d\n", port);

    while (1)
    {
        //loop waiting for client connection
        connfd = accept(listenfd, (struct sockaddr *)&client, (socklen_t *)&client_len);
        if (connfd == -1) {
            error_die("accept");
        }

        //处理请求
        accept_request(connfd);

        /*if (pthread_create(&newthread , NULL, accept_request, connfd_sock) != 0)
          perror("pthread_create");*/
    }

    //关闭监听描述符,注意在这之前需要关闭close(connfd)
    close(listenfd);

    return (0);
}
