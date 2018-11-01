#include "ringbuf.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>


#define pri_dbg(format, args...) fprintf(stderr,"%s %d %s() " format, __FILE__, __LINE__, __func__, ## args)


int ringbuf_create(ringbuf_t **rb, const void* mem, const size_t capacity)
{
    ringbuf_unit_t *rbu;
    ringbuf_t *prb;
    
 	if (NULL == mem || capacity < sizeof(ringbuf_t)) return ENOMEM;
    //init rb
    prb = *rb;
    prb = (ringbuf_t *)(mem);
    memset(prb, 0, sizeof(ringbuf_t));
    prb->ptr = (uint8_t*)(mem + sizeof(ringbuf_t));    // ringbuf start point
    prb->capacity = capacity - sizeof(ringbuf_t);
    prb->w = prb->ptr;
    *rb = prb;

    // first unit init
    rbu = prb->w;
    rbu->size = 0;
    rbu->prev = rbu;
    rbu->next = (ringbuf_unit_t *)(rbu->data + rbu->size);
    rbu->next->size = 0;
    
    return 0;
}


int ringbuf_destroy(const ringbuf_t *rb)
{
    memset(rb, 0, sizeof(ringbuf_t));
    return 0;
}


int ringbuf_read_add(const ringbuf_t *rb, ringbuf_rlink_t *rbrl)
{
    size_t index = 0;
    
    for (index = 0; index < RB_MAX_READ_NUM; index++) {
        if (rb->r[index] == NULL) {
            break;            
        }
    }
    if (index == RB_MAX_READ_NUM) {
        pri_dbg("index = %d\n",index);
        return -1;
    }
   
    rbrl->rb = rb;
    rbrl->rb->r[index] = rb->w->next;
    rbrl->index = index;
    
    return 0;
}

int ringbuf_read_del(ringbuf_rlink_t *rbrl)
{
    if (rbrl == NULL || rbrl->rb == NULL || rbrl->index < 0 || rbrl->index >= RB_MAX_READ_NUM) {
        return EACCES;
    }
    rbrl->rb->r[rbrl->index] = NULL;
    rbrl->index = -1;
    
    return 0;
}


int ringbuf_capacity(const ringbuf_t *rb)
{
    return rb->capacity;
}

int ringbuf_rlin_remained(const ringbuf_rlink_t *rbrl)
{
	return 0;
}


int ringbuf_write_get_unit(ringbuf_t *rb, unsigned char **p, int size)
{
    ringbuf_unit_t *rbu = NULL;
    
    if (rb == NULL || size == 0) return EACCES;
    if (size + sizeof(ringbuf_unit_t) > (rb->capacity - (rb->w->data - rb->ptr + rb->w->size))) {    // edge
        rbu = (ringbuf_unit_t *)rb->ptr;
        rb->w->next = rbu;
    } else {
        rbu = rb->w;
        rbu->next = (ringbuf_unit_t *)(rbu->data + rbu->size);
    }
    rbu->next->size = 0;   // Make sure the next next unit size is 0
    *p = rb->w->next->data;
   
    return 0;
}


int ringbuf_write_put_unit(ringbuf_t *rb, int size)
{
    ringbuf_unit_t *rbu = NULL;
    
    if (rb == NULL || size == 0) return EACCES;
    // compelete current unit
    rbu = rb->w->next;
    rbu->prev = rb->w;
    rbu->size = size;
    rbu->next = NULL;//rbu->data + rbu->size;
    //rbu->next->size = 0;   // Make sure the next next unit size is 0.
    
    // move to next unit.
    rb->w = rbu;
    
    return 0;
}

int ringbuf_read_get_unit(ringbuf_rlink_t *rbrl, unsigned char **p, int *size)
{
    ringbuf_unit_t *rbu = NULL;
    
    if (rbrl == NULL || p == NULL) return EACCES;
    rbu = rbrl->rb->r[rbrl->index];
    if (rbu == NULL) return EACCES;
    if (rbu->size <= 0) {
        if (NULL != rbu->next) {
            rbrl->rb->r[rbrl->index] = rbu->next;
            rbu = rbrl->rb->r[rbrl->index];
        } else {
            *p = NULL;
            *size = 0;
            return EAGAIN;
        }
    }
    *p = rbu->data;
    *size = rbu->size;

    return 0;
}

int ringbuf_read_put_unit(ringbuf_rlink_t *rbrl)
{
    if (rbrl == NULL) return EACCES;
    // move to next read unit.
    if (NULL == rbrl->rb->r[rbrl->index]->next) {
        rbrl->rb->r[rbrl->index]->size = 0;
    } else {
        rbrl->rb->r[rbrl->index] = rbrl->rb->r[rbrl->index]->next;
    }
    
    return 0;
}



#define N 1000*1024
void ringbuf_test(void)
{
    
    uint8_t testbuf[1000+280];
    ringbuf_t *rb;
    ringbuf_rlink_t    rb_read;
    uint8_t src[N] = "\0", dst[N] = "\0";
    
    ringbuf_create(&rb, testbuf, 1000+280);
    ringbuf_read_add(rb, &rb_read);
    pri_dbg("capacity = %d, sizeof(ringbuf_t) = %d, sizeof(ringbuf_unit_t) = %d\n",rb_read.rb->capacity, sizeof(ringbuf_t), sizeof(ringbuf_unit_t));
    //pri_dbg("left = %d\n",ringbuf_left(&rb_read));
    
    // 
    uint8_t *p, *q;
    int size, i;
    #if 0
    // write 100 bytes
    src[0] = 'A';
    src[99] = 'B';
    ringbuf_write_get_unit(rb, &p, 100);
    memcpy(p, src, 100);
    ringbuf_write_put_unit(rb, 100);
    // write 1 byte
    ringbuf_write_get_unit(rb, &p, 1);
    *p = 'Q';
    ringbuf_write_put_unit(rb, 1);
    
    // read 1
    ringbuf_read_get_unit(&rb_read, &q, &size);
    memcpy(dst, q, size);
    ringbuf_read_put_unit(&rb_read);
    pri_dbg("dst = %s, size = %d\n",dst, size);
    // read 2
    ringbuf_read_get_unit(&rb_read, &q, &size);
    memcpy(dst, q, size);
    ringbuf_read_put_unit(&rb_read);
    pri_dbg("dst = %s, size = %d\n",dst, size);
    #endif
  
    // test 2
	srand((unsigned int)time(NULL));
	for (i = 0; i < N; i++)
		src[i] = (uint8_t)(rand() % 256);

    ringbuf_write_get_unit(rb, &p, 888);
    memcpy(p, src, 888);
    ringbuf_write_put_unit(rb, 888);
    pri_dbg("\n");
    
    ringbuf_write_get_unit(rb, &p, 1000 - 888 + 1); // overrun(overflow)
    memcpy(p, src, 1000 - 888 + 1);
    ringbuf_write_put_unit(rb, 1000 - 888 + 1);
    pri_dbg("\n");

    ringbuf_read_get_unit(&rb_read, &q, &size);
    memcpy(dst, q, size);
    pri_dbg("size = %d\n", size);
    ringbuf_read_put_unit(&rb_read);
    //assert(0 == memcmp(src+sizeof(ringbuf_t)+sizeof(ringbuf_unit_t)*2, dst, size));
    pri_dbg("\n");
    
    ringbuf_read_get_unit(&rb_read, &q, &size);
    memcpy(dst, q, size);
    ringbuf_read_put_unit(&rb_read);
    assert(0 == memcmp(src, dst, size));
    pri_dbg("\n");
 
  
    return;
}




int main (int argc, char *argv[])
{
	ringbuf_test();
	//ringbuf_test1();

	return 0;
}



