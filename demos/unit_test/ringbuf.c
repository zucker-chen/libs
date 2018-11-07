#include "ringbuf.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>



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
    prb->w = (ringbuf_unit_t *)prb->ptr;
    *rb = prb;

    // first unit init
    rbu = prb->w;
    rbu->size = 0;
    rbu->prev = rbu;
    rbu->next = NULL;
    
    return 0;
}


int ringbuf_destroy(ringbuf_t *rb)
{
    memset(rb, 0, sizeof(ringbuf_t));
    return 0;
}


int ringbuf_read_add(ringbuf_t *rb, ringbuf_rlink_t *rbrl)
{
    size_t index = 0;
    
    for (index = 0; index < RB_MAX_READ_NUM; index++) {
        if (rb->r[index] == NULL) {
            break;            
        }
    }
    if (index == RB_MAX_READ_NUM) {
        return EACCES;
    }
   
    rbrl->rb = rb;
    rbrl->rb->r[index] = rb->w;
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

int ringbuf_write_get_unit(ringbuf_t *rb, unsigned char **p, int size)
{
    size_t index = 0;
    ringbuf_unit_t *rbu = NULL;
    
    if (rb == NULL || size == 0) return EACCES;
    if (size + sizeof(ringbuf_unit_t) > (rb->capacity - (rb->w->data - rb->ptr + rb->w->size))) {    // Edge
        rbu = (ringbuf_unit_t *)rb->ptr;
        rbu->prev = rb->w;
        rb->w->next = rbu;
        rbu->size = 0;
        rb->w = rb->w->next;
    } else {
        rbu = rb->w;
        //rbu->next = (ringbuf_unit_t *)(rbu->data + rbu->size);
    }
    
    #if 0
    if (rbu > rbu->next && rbu < (rbu->next + size + sizeof(ringbuf_unit_t))) {
        // Next write unit will overwrite current unit, error.
        pri_dbg("\n");
        return ERANGE;
    }
    #endif
    
    *p = rb->w->data;

    // Reset read links when write unit will overwrite read unit.
    for (index = 0; index < RB_MAX_READ_NUM; index++) {
        if (rb->r[index] != NULL && (rb->r[index] >= rbu && rb->r[index] <= (rbu + size + sizeof(ringbuf_unit_t)))) {
            rb->r[index] = rbu;
        }
    }
    
    return 0;
}


int ringbuf_write_put_unit(ringbuf_t *rb, int size)
{
    ringbuf_unit_t *rbu = NULL;
    
    if (rb == NULL || size == 0) return EACCES;
    // init next unit
    rbu = (ringbuf_unit_t *)(rb->w->data + size);
    rbu->size = 0;
    rbu->prev = rb->w;
    rbu->next = NULL;
    // move to next unit.
    rb->w->next = rbu;
    rb->w = rb->w->next;
    // set the size at last.
    rb->w->prev->size = size;
    
    return 0;
}

int ringbuf_read_get_unit(ringbuf_rlink_t *rbrl, unsigned char **p, int *size)
{
    ringbuf_unit_t *rbu = NULL;
    
    if (rbrl == NULL || p == NULL) return EACCES;
    rbu = rbrl->rb->r[rbrl->index];
    if (rbu == NULL) return EACCES;
        
    if (rbu->size <= 0) {
        if (NULL != rbu->next && rbu->next < rbu) {
            rbrl->rb->r[rbrl->index] = rbu->next;
            rbu = rbrl->rb->r[rbrl->index];
            if (rbu->size <=0) return EAGAIN;
        } else {    // Need try again if current read unit->next is NULL.
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
        //rbrl->rb->r[rbrl->index]->size = 0;
        return EACCES;
    } else {
        rbrl->rb->r[rbrl->index] = rbrl->rb->r[rbrl->index]->next;
    }
    
    return 0;
}


#define pri_dbg(format, args...) fprintf(stderr,"%s %d %s() " format, __FILE__, __LINE__, __func__, ## args)
#define N 1000*1024
void ringbuf_test(void)
{
    uint8_t testbuf[1000+280];
    ringbuf_t *rb;
    ringbuf_rlink_t    rb_read;
    uint8_t src[N] = "\0", dst[N] = "\0";
    
    ringbuf_create(&rb, testbuf, 1000+280);
    ringbuf_read_add(rb, &rb_read);
    pri_dbg("capacity = %lu, sizeof(ringbuf_t) = %lu, sizeof(ringbuf_unit_t) = %lu\n", rb_read.rb->capacity, sizeof(ringbuf_t), sizeof(ringbuf_unit_t));
    
    // 
    uint8_t *p, *q;
    int size, i;
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
  
    // test 2
	srand((unsigned int)time(NULL));
	for (i = 0; i < N; i++)
		src[i] = (uint8_t)(rand() % 256);

    assert(0 == ringbuf_write_get_unit(rb, &p, 222));
    memcpy(p, src, 222);
    assert(0 == ringbuf_write_put_unit(rb, 222));
    pri_dbg("\n");

    assert(0 == ringbuf_write_get_unit(rb, &p, 666));    // overrun(overflow), 100+1+222+666+24>1000
    memcpy(p, src+222, 666);
    assert(0 == ringbuf_write_put_unit(rb, 666));
    pri_dbg("\n");
    
    assert(0 == ringbuf_write_get_unit(rb, &p, 1000 - 888 + 1));
    memcpy(p, src+222+666, 1000 - 888 + 1);
    assert(0 == ringbuf_write_put_unit(rb, 1000 - 888 + 1));
    pri_dbg("\n");

    assert(0 == ringbuf_write_get_unit(rb, &p, 123));
    memcpy(p, src+222+666+113, 123);
    assert(0 == ringbuf_write_put_unit(rb, 123));
    pri_dbg("\n");

    assert(0 == ringbuf_read_get_unit(&rb_read, &q, &size));
    memcpy(dst, q, size);
    pri_dbg("size = %d\n", size);
    assert(0 == ringbuf_read_put_unit(&rb_read));
    pri_dbg("\n");

    assert(0 == ringbuf_read_get_unit(&rb_read, &q, &size));
    memcpy(dst+666, q, size);
    pri_dbg("size = %d\n", size);
    assert(0 == ringbuf_read_put_unit(&rb_read));
    pri_dbg("\n");

    assert(0 == memcmp(src+222, dst, 666+113));
    pri_dbg("\n");
  
    return;
}

void ringbuf_test1(void)
{
    uint8_t testbuf[1000+280];
    ringbuf_t *rb;
    ringbuf_rlink_t    rb_read;
    uint8_t src[N] = "\0", dst[N] = "\0";
    
    ringbuf_create(&rb, testbuf, 1000+280);
    ringbuf_read_add(rb, &rb_read);
    pri_dbg("capacity = %lu, sizeof(ringbuf_t) = %lu, sizeof(ringbuf_unit_t) = %lu\n",rb_read.rb->capacity, sizeof(ringbuf_t), sizeof(ringbuf_unit_t));
    
    // 
    uint8_t *p, *q;
    int size, i;
	srand((unsigned int)time(NULL));
	for (i = 0; i < N; i++)
		src[i] = (uint8_t)(rand() % 256);

    assert(0 == ringbuf_write_get_unit(rb, &p, 888));
    memcpy(p, src, 888);
    assert(0 == ringbuf_write_put_unit(rb, 888));
    pri_dbg("\n");
    
    assert(0 == ringbuf_write_get_unit(rb, &p, 1000 - 888 + 1));
    memcpy(p, src+888, 1000 - 888 + 1);
    assert(0 == ringbuf_write_put_unit(rb, 1000 - 888 + 1));
    pri_dbg("\n");

    assert(0 == ringbuf_write_get_unit(rb, &p, 123));
    memcpy(p, src+888+113, 123);
    assert(0 == ringbuf_write_put_unit(rb, 123));
    pri_dbg("\n");

    assert(0 == ringbuf_read_get_unit(&rb_read, &q, &size));
    memcpy(dst, q, size);
    pri_dbg("size = %d\n", size);
    assert(0 == ringbuf_read_put_unit(&rb_read));
    pri_dbg("\n");

    assert(0 == ringbuf_read_get_unit(&rb_read, &q, &size));
    memcpy(dst+113, q, size);
    pri_dbg("size = %d\n", size);
    assert(0 == ringbuf_read_put_unit(&rb_read));
    pri_dbg("\n");

    assert(0 == memcmp(src+888, dst, 123+113));
    pri_dbg("\n");
    
    return;
}


int main (int argc, char *argv[])
{
	ringbuf_test();
	ringbuf_test1();

	return 0;
}



