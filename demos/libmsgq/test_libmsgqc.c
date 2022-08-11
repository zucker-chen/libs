#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "libmsgq.h"


#define MSGQ_KEY            12345678


int msg_recv_cb(mq_handle_t *ctx, char *buf, int size)
{
    printf("%s(%d): buf = %s, size = %d\n", __FUNCTION__, __LINE__, buf, size);
    
    return 0;
}


static void *test_1(void *arg)
{
	pthread_detach(pthread_self());
    
    mq_handle_t *ctx = NULL;
    ctx = mq_init_client(MSGQ_KEY, msg_recv_cb);
	if (ctx == NULL) {
        printf("%s(%d): mq_init_client error!\n", __FUNCTION__, __LINE__);
		return NULL;
	}
    
    mq_send(ctx, "test1-0", 8);
	sleep(1);
    mq_send(ctx, "test1-2", 8);
	sleep(1);
    mq_send(ctx, "test1-3", 8);
	sleep(1);
    mq_send(ctx, "test1-4", 8);
	sleep(3);
    mq_send(ctx, "test1-5", 8);
	sleep(1);
    mq_deinit_client(ctx);		
	
	return NULL;
}

static void *test_2(void *arg)
{
	pthread_detach(pthread_self());
	
    mq_handle_t *ctx = NULL;
	//sleep(1);
    ctx = mq_init_client(MSGQ_KEY, msg_recv_cb);
	if (ctx == NULL) {
        printf("%s(%d): mq_init_client error!\n", __FUNCTION__, __LINE__);
		return NULL;
	}
    
    mq_send(ctx, "test2-0", 8);
	sleep(2);
    mq_send(ctx, "test2-2", 8);
	sleep(2);
    mq_send(ctx, "test2-3", 8);
	sleep(2);
    mq_send(ctx, "test2-4", 8);
	sleep(2);
    mq_deinit_client(ctx);		
	
	return NULL;
}

static void *test_3(void *arg)
{
	pthread_detach(pthread_self());
	
    mq_handle_t *ctx = NULL;
    char buf[MQ_MAX_BUF_LEN] = {0};
	int size;
	//sleep(1);
    ctx = mq_init_client(MSGQ_KEY, msg_recv_cb);
	if (ctx == NULL) {
        printf("%s(%d): mq_init_client error!\n", __FUNCTION__, __LINE__);
		return NULL;
	}
    
    mq_send(ctx, "test3-0", 8);
	sleep(2);
	size = mq_recv(ctx, buf, MQ_MAX_BUF_LEN);
	if (size <= 0) {
        printf("%s(%d): mq_recv error!\n", __FUNCTION__, __LINE__);
	}
	printf("%s(%d): mq_recv buf = %s\n", __FUNCTION__, __LINE__, buf);
	sleep(2);
    mq_deinit_client(ctx);		
	
	return NULL;
}

static void *test_4(void *arg)
{
	pthread_detach(pthread_self());
	
    mq_handle_t *ctx = NULL;
    char buf[MQ_MAX_BUF_LEN] = {0};
	int size;
	//sleep(1);
    ctx = mq_init_client(MSGQ_KEY, msg_recv_cb);
	if (ctx == NULL) {
        printf("%s(%d): mq_init_client error!\n", __FUNCTION__, __LINE__);
		return NULL;
	}
    
    mq_send(ctx, "test4-0", 8);
	sleep(2);
	size = mq_recv(ctx, buf, MQ_MAX_BUF_LEN);
	if (size <= 0) {
        printf("%s(%d): mq_recv error!\n", __FUNCTION__, __LINE__);
	}
	printf("%s(%d): mq_recv buf = %s\n", __FUNCTION__, __LINE__, buf);
	sleep(2);
    mq_deinit_client(ctx);		
	
	return NULL;
}


int main(int argc, char **argv)
{
	pthread_t tid;

	#if 1
	if (0 != pthread_create(&tid, NULL, test_1, NULL)) {
		printf("pthread_create failed!\n");
	}
	#endif
	#if 1
	if (0 != pthread_create(&tid, NULL, test_2, NULL)) {
		printf("pthread_create failed!\n");
	}
	#endif
	#if 1
	if (0 != pthread_create(&tid, NULL, test_3, NULL)) {
		printf("pthread_create failed!\n");
	}
	#endif
	#if 1
	if (0 != pthread_create(&tid, NULL, test_4, NULL)) {
		printf("pthread_create failed!\n");
	}
	#endif

	sleep(20);

    return 0;
}
