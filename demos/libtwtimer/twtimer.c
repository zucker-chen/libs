// Timing Wheel Timer(timeout)
// 64ms per bucket
// http://www.cs.columbia.edu/~nahum/w6998/papers/sosp87-timing-wheels.pdf

#include "twtimer.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/select.h> 
#include <sys/time.h>
#include <sys/prctl.h>
#include <errno.h>
#include <stdio.h>

#define TIME_RESOLUTION 6
#define TIME(clock) ((clock) >> TIME_RESOLUTION) // per 64ms

#define TVR_BITS 8
#define TVN_BITS 6
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_MASK (TVR_SIZE - 1)
#define TVN_MASK (TVN_SIZE - 1)

#define TVN_INDEX(clock, n) ((int)((clock >> (TIME_RESOLUTION + TVR_BITS + (n * TVN_BITS))) & TVN_MASK))

struct time_bucket_t {
	twtimer_t* first;
};

typedef struct time_wheel_s{
	pthread_spinlock_t locker;

	uint64_t 				count;
	uint64_t 				clock;
	struct  time_bucket_t 	tv1[TVR_SIZE];
	struct  time_bucket_t 	tv2[TVN_SIZE];
	struct  time_bucket_t 	tv3[TVN_SIZE];
	struct  time_bucket_t 	tv4[TVN_SIZE];
	struct  time_bucket_t 	tv5[TVN_SIZE];
    int     				running;    // 0:stoped, 1: running 
	twtimer_t				timer;
} time_wheel_t;


static int time_wheel_addlist(time_wheel_t *tm, twtimer_t *timer);
static int time_wheel_cascade(time_wheel_t *tm, struct time_bucket_t *tv, int index);

time_wheel_t * time_wheel_create(uint64_t clock)
{
	time_wheel_t * tm;
	tm = (time_wheel_t *)calloc(1, sizeof(*tm));
	if (tm) {
		tm->count = 0;
		tm->clock = clock;
        pthread_spin_init(&tm->locker, PTHREAD_PROCESS_SHARED);
	}
	return tm;
}

int time_wheel_destroy(time_wheel_t *tm)
{
    int r = -1;
    
    if (NULL == tm) {
        printf("%s(%d): NULL == tm!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (0 < tm->count) {
        printf("%s(%d): Warnning: tm->count = %lu!\n", __FUNCTION__, __LINE__, tm->count);
        return -1;
    }
    r = pthread_spin_destroy(&tm->locker);
    free(tm);
    tm = NULL;
    
	return r;
}

int time_wheel_start(time_wheel_t *tm, twtimer_t *timer)
{
	int r;
    
    if (NULL == tm) {
        printf("%s(%d): NULL == tm!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (NULL == timer->ontimeout) {
        printf("%s(%d): NULL == timer->ontimeout!\n", __FUNCTION__, __LINE__);
        return -1;
    }

	pthread_spin_lock(&tm->locker);
	r = time_wheel_addlist(tm, timer);
	pthread_spin_unlock(&tm->locker);
    
	return r;
}

int time_wheel_stop(time_wheel_t *tm, twtimer_t *timer)
{
	twtimer_t** pprev;
    
    if (NULL == tm) {
        printf("%s(%d): NULL == tm!\n", __FUNCTION__, __LINE__);
        return -1;
    }
	pthread_spin_lock(&tm->locker);
	pprev = timer->pprev;
	if (timer->pprev) {
		--tm->count; // timer validation ???
		*timer->pprev = timer->next;
	}
	if (timer->next)
		timer->next->pprev = timer->pprev;
	timer->pprev = NULL;
	timer->next = NULL;
	pthread_spin_unlock(&tm->locker);
    
	return pprev ? 0 : -1;
}

static void * ontimeout_thdcb(void *param)
{
	pthread_detach(pthread_self());
    prctl(PR_SET_NAME, __FUNCTION__);
    
    twtimer_t* timer;
    timer = (twtimer_t*)param;
    
    timer->ontimeout(timer->param);
    
	return NULL;
}

int time_wheel_process(time_wheel_t *tm, uint64_t clock)
{
	int index;
	twtimer_t* timer;
	struct time_bucket_t bucket;

    if (NULL == tm) {
        printf("%s(%d): NULL == tm!\n", __FUNCTION__, __LINE__);
        return -1;
    }
	pthread_spin_lock(&tm->locker);
	while (tm->clock < clock)
	{
		index = (int)(TIME(tm->clock) & TVR_MASK);

		if (0 == index 
			&& 0 == time_wheel_cascade(tm, tm->tv2, TVN_INDEX(tm->clock, 0))
			&& 0 == time_wheel_cascade(tm, tm->tv3, TVN_INDEX(tm->clock, 1))
			&& 0 == time_wheel_cascade(tm, tm->tv4, TVN_INDEX(tm->clock, 2)))
		{
			time_wheel_cascade(tm, tm->tv5, TVN_INDEX(tm->clock, 3));
		}

		// move bucket
		bucket.first = tm->tv1[index].first;
		if (bucket.first)
			bucket.first->pprev = &bucket.first;
		tm->tv1[index].first = NULL; // clear

		// trigger timer
		while (bucket.first)
		{
			timer = bucket.first;
			bucket.first = timer->next;
			if (timer->next) {
				timer->pprev = &bucket.first;
            }
            timer->next = NULL;
            timer->pprev = NULL;
            --tm->count;
            if (timer->type == TIMER_CONTINUS) {
                time_wheel_addlist(tm, timer);
            }
			if (timer->ontimeout) {
                pthread_t pid;
				pthread_spin_unlock(&tm->locker);
                pthread_create(&pid, NULL, ontimeout_thdcb, (void *)timer);
				//timer->ontimeout(timer->param);
				pthread_spin_lock(&tm->locker);
			}
		}	
        
        // update clock
		tm->clock += (1 << TIME_RESOLUTION);
	}

	pthread_spin_unlock(&tm->locker);
	return (int)(tm->clock - clock);
}

static int time_wheel_cascade(time_wheel_t *tm, struct time_bucket_t *tv, int index)
{
	twtimer_t* timer;
	twtimer_t* next;
	struct time_bucket_t bucket;
	bucket.first = tv[index].first;
	tv[index].first = NULL; // clear

    if (NULL == tm) {
        printf("%s(%d): NULL == tm!\n", __FUNCTION__, __LINE__);
        return -1;
    }
	for (timer = bucket.first; timer; timer = next)
	{
		--tm->count; // start will add count
		next = timer->next;
		timer->next = NULL;
		timer->pprev = NULL;
		time_wheel_addlist(tm, timer);
	}

	return index;
}

static int time_wheel_addlist(time_wheel_t *tm, twtimer_t *timer)
{
	int i;
	uint64_t expire_clock, diff;
	struct time_bucket_t* tv;

    if (NULL == tm) {
        printf("%s(%d): NULL == tm!\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (NULL == timer->ontimeout) {
        printf("%s(%d): NULL == timer->ontimeout!\n", __FUNCTION__, __LINE__);
        return -1;
    }
	if (timer->pprev)
	{
        printf("%s(%d): timer have been started!\n", __FUNCTION__, __LINE__);
		return EEXIST;
	}

    expire_clock = timer->expire + tm->clock;
	diff = TIME(expire_clock - tm->clock); // per 64ms

	if (expire_clock < tm->clock)
	{
		i = TIME(tm->clock) & TVR_MASK;
		tv = tm->tv1 + i;
	}
	else if (diff < (1 << TVR_BITS))
	{
		i = TIME(expire_clock) & TVR_MASK;
		tv = tm->tv1 + i;
	}
	else if (diff < (1 << (TVR_BITS + TVN_BITS)))
	{
		i = (TIME(expire_clock) >> TVR_BITS) & TVN_MASK;
		tv = tm->tv2 + i;
	}
	else if (diff < (1 << (TVR_BITS + 2 * TVN_BITS)))
	{
		i = (TIME(expire_clock) >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
		tv = tm->tv3 + i;
	}
	else if (diff < (1 << (TVR_BITS + 3 * TVN_BITS)))
	{
		i = (TIME(expire_clock) >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
		tv = tm->tv4 + i;
	}
	else if (diff < (1ULL << (TVR_BITS + 4 * TVN_BITS)))
	{
		i = (TIME(expire_clock) >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
		tv = tm->tv5 + i;
	}
	else
	{
		//pthread_spin_unlock(&tm->locker);
        printf("%s(%d): exceed max timeout value!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	// list insert
	timer->pprev = &tv->first;
	timer->next = tv->first;
	if (timer->next)
		timer->next->pprev = &timer->next;
	tv->first = timer;
	++tm->count;
	return 0;
}


/*******************************************************************************/
uint64_t twtimer_get_systime(void)
{
	uint64_t time_ms = 0;

	#if 0
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_ms = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
	#else
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv); 
	time_ms = (uint64_t)tv.tv_sec * 1000 + tv.tv_nsec / 1000000;
	#endif

	return time_ms;
}

int twtimer_msleep(uint64_t ms)
{
    struct timeval time; 
    int ret = 0;
    
    time.tv_sec = ms/1000; 
    time.tv_usec = (ms%1000) * 1000; 
    while (1)
    {
        ret = select(0, NULL, NULL, NULL, &time); 
        if (ret < 0 && errno != EINTR) {
            return -1;
        } else if (ret < 0 && errno == EINTR) {
            // printf("select EINTR\n");
            continue;
        }
        break;
    }
    
    return 0;
}

static void * twtimer_loop_thdcb(void * param)
{
    time_wheel_t *twheel = NULL;

    twheel = (time_wheel_t *)param;
	pthread_detach(pthread_self());
    prctl(PR_SET_NAME, __FUNCTION__);
    
    while (twheel->running == 1)
    {
        time_wheel_process(twheel, twtimer_get_systime());
        twtimer_msleep(1 << (TIME_RESOLUTION - 1));
    }
	twheel->running = 0;
	printf("%s(%d): pthread exit.\n", __FUNCTION__, __LINE__);

	return NULL;
}

static tw_handle_t *twtimer_create(void)
{
    time_wheel_t *twheel = NULL;
    pthread_t pid;
    
    twheel = time_wheel_create(twtimer_get_systime());
    if (NULL == twheel) {
        printf("%s(%d): NULL == twheel!\n", __FUNCTION__, __LINE__);
        return NULL;
    }
    
	if (twheel->running == 0) {
		twheel->running = 1;
		pthread_create(&pid, NULL, twtimer_loop_thdcb, (void *)twheel);
	} else {
        printf("%s(%d): pthread_create error.\n", __FUNCTION__, __LINE__);
        twheel->running = 0;
		return NULL;
	}
    
    return (tw_handle_t *)twheel;
}

static int twtimer_destroy(tw_handle_t *tw)
{
    int ret = -1;
    time_wheel_t *twheel = (time_wheel_t *)tw;
    //twheel->running = 0;
    //twtimer_msleep(100);
    ret = time_wheel_destroy(twheel);
    twheel = NULL;
    
    return ret;
}

tw_handle_t *twtimer_start(twtimer_t *timer)
{
    time_wheel_t *twheel = NULL;

	twheel = twtimer_create();
	if (twheel == NULL) {
        printf("%s(%d): twtimer_create error.\n", __FUNCTION__, __LINE__);
		return NULL;
	}
	memcpy(&twheel->timer, timer, sizeof(twtimer_t));
    time_wheel_start(twheel, &twheel->timer);
	
    return (tw_handle_t *)twheel;
}

int twtimer_stop(tw_handle_t *twheel)
{
	if (twheel == NULL) {
        printf("%s(%d): twheel == NULL error.\n", __FUNCTION__, __LINE__);
		return -1;
	}
	
    twheel->running = 0;
    twtimer_msleep(100);
    time_wheel_stop(twheel, &twheel->timer);
	
	twtimer_destroy(twheel);
	
	return 0;
}

