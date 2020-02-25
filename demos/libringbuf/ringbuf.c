#include "ringbuf.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>


/* input: mem, capacity
 * ouput: rb, ringbuf struct
 * note: mem need to be allocated by yourself before.
 */
int ringbuf_create(ringbuf_t **rb, const void* mem, const int capacity)
{
    ringbuf_unit_t *rbu;
    ringbuf_t *prb;
    
 	if (NULL == mem || capacity < sizeof(ringbuf_t)) {
		return -1;
	}
    //init rb
    prb = *rb = (ringbuf_t *)(mem);
    memset(prb, 0, sizeof(ringbuf_t));
    prb->ptr = (uint8_t*)(mem + sizeof(ringbuf_t));    // ringbuf start point
    prb->capacity = capacity - sizeof(ringbuf_t);
    prb->w = (ringbuf_unit_t *)prb->ptr;

    // first unit init, must make size = 0, next = NULL.
    rbu = prb->w;
    rbu->size = 0;
    rbu->prev = rbu;
    rbu->next = NULL;
    
    return 0;
}

/* input: rb
 * ouput:
 * note: rb->ptr need to be free by yourself after
 */
int ringbuf_destroy(ringbuf_t *rb)
{
    memset(rb, 0, sizeof(ringbuf_t));
    return 0;
}

/* input: rb
 * ouput: rbrl, read ringbuf handle
 * func: create a read handle
 */
int ringbuf_read_add(ringbuf_t *rb, ringbuf_rlink_t *rbrl)
{
    int index = 0;
    
    for (index = 0; index < RB_MAX_READ_NUM; index++) {
        if (rb->r[index] == NULL) {
            break;            
        }
    }
    if (index == RB_MAX_READ_NUM) {
        return -1;
    }
   
    rbrl->rb = rb;
    rbrl->rb->r[index] = rb->w;
    rbrl->index = index;
    
    return 0;
}

/* input: 	rb
 *		  	index: =0:latest, <0:before(previous), >0:after(next) 
 * ouput: 	rbrl, read ringbuf handle
 * return:	0:success, <0:error, -2:out range oldest, -3:out range latest
 * func: seek read link
 */
int ringbuf_read_seek(ringbuf_rlink_t *rbrl, int index)
{
    ringbuf_unit_t *rbu = NULL;
	int i = 0;
    
    if (rbrl == NULL) {
		return -1;
    } 
	
	rbu = rbrl->rb->r[rbrl->index];
	if (index == 0) {
		rbrl->rb->r[index] = rbrl->rb->w;
 	} else if (index < 0) {
		index = 0 - index;
		for (i = 0; i < index; i++)
		{
			// move to prev read unit.
			if (NULL == rbrl->rb->r[rbrl->index]->prev) {
				return -2;
			} else if ((unsigned long)rbu->prev->next == (unsigned long)rbu) {
				rbu = rbu->prev;
			} else {
				return -2;						// if wild pointer when this memory is overwritten.
			}
		}
		rbrl->rb->r[rbrl->index] = rbu;
	} else if (index > 0) {
		for (i = 0; i < index; i++)
		{
			// move to next read unit.
			if (NULL == rbu->next) {
				return -3;
			} else {
			if (rbu->next->size <= 0) {			// latest can't already
				return -3;
			}
				rbu = rbu->next;
			}
		}
		rbrl->rb->r[rbrl->index] = rbu;
	}

    return 0;
}

int ringbuf_read_del(ringbuf_rlink_t *rbrl)
{
    if (rbrl == NULL || rbrl->rb == NULL || rbrl->index < 0 || rbrl->index >= RB_MAX_READ_NUM) {
        return -1;
    }
    rbrl->rb->r[rbrl->index] = NULL;
    rbrl->index = -1;
    
    return 0;
}


int ringbuf_capacity_get(const ringbuf_t *rb)
{
    return rb->capacity;
}

/* input: rb, size
 * ouput: p, write mem address
 * func: get write data mem address
 */
int ringbuf_write_get_unit(ringbuf_t *rb, unsigned char **p, int size)
{
    int index = 0;
    ringbuf_unit_t *rbu = NULL;
	char *rbu_end;
    
    if (rb == NULL || size <= 0 || size + sizeof(ringbuf_unit_t) > rb->capacity) {
		return -1;
    }
	
    if (size + sizeof(ringbuf_unit_t) > (rb->capacity - (rb->w->data - rb->ptr))) {    // Edge, Reserve one more header space
        rbu = (ringbuf_unit_t *)rb->ptr;
        rbu->prev = rb->w;
        rbu->size = 0;
		rbu->next = NULL;
        rb->w->next = rbu;
        rb->w = rbu;
    } else {
        rbu = rb->w;
    }
        
    *p = rbu->data;

    // Reset read links when write unit will overwrite read unit.
    rbu_end = (char *)(rbu->data + size + sizeof(ringbuf_unit_t));
    for (index = 0; index < RB_MAX_READ_NUM; index++) {
        if (rb->r[index] != NULL && ( \
			((unsigned long)rbu_end > (unsigned long)rbu && (rb->r[index] > rbu && (unsigned long)rb->r[index] <= (unsigned long)rbu_end)) || \
			((unsigned long)rbu_end < (unsigned long)rbu && (rb->r[index] > rbu || (unsigned long)rb->r[index] <= (unsigned long)rbu_end)) )) {
			printf("func = %s, line = %d:  Read slow, Reset read link[%d]\n", __FUNCTION__, __LINE__, index);
            rb->r[index] = rbu;     // Warnning! There is a risk of data tampering if the unit is read when transferred.
        }
    }
    
    return 0;
}

/* input: rb, size
 * ouput: 
 * func: update write ringbuf link table
 */
int ringbuf_write_put_unit(ringbuf_t *rb, int size)
{
    ringbuf_unit_t *rbu = NULL;
    
    if (rb == NULL || size <= 0) {
		return -1;
    }
    // init next unit, must make size = 0, next = NULL.
    rbu = (ringbuf_unit_t *)(rb->w->data + size);
    rbu->size = 0;
    rbu->prev = rb->w;
    rbu->next = NULL;
    // move to next unit.
    rb->w->next = rbu;
    rb->w = rb->w->next;
    // set the size at last, this is to lock function.
    rb->w->prev->size = size;
    
    return 0;
}

/* input: rbrl
 * ouput: p, read data mem address; size, data size
 * func: get read data mem address
 */
int ringbuf_read_get_unit(ringbuf_rlink_t *rbrl, unsigned char **p, int *size)
{
    ringbuf_unit_t *rbu = NULL;
    
    if (rbrl == NULL || p == NULL) {
		return -1;
    }
    rbu = rbrl->rb->r[rbrl->index];
    if (rbu == NULL) {
		return -1;
    }
        
    if (rbu->size <= 0) {
        if (NULL != rbu->next && rbu->next < rbu) { // Edge
            rbrl->rb->r[rbrl->index] = rbu->next;
            rbu = rbrl->rb->r[rbrl->index];
            if (rbu->size <=0) {					// waiting for the lastest data.
				*size = 0;
				return -2;
			}
        } else {    // Need try again if read unit->next is NULL, waiting for the lastest data.
            *size = 0;
            return -2;
        }
    }
    *p = rbu->data;
    *size = rbu->size;

    return 0;
}

/* input: rbrl
 * ouput: 
 * func: update read ringbuf link table
 */
int ringbuf_read_put_unit(ringbuf_rlink_t *rbrl)
{
    if (rbrl == NULL) {
		return -1;
    }
    // move to next read unit.
    if (NULL == rbrl->rb->r[rbrl->index]->next) {
        return -1;
    } else {
        rbrl->rb->r[rbrl->index] = rbrl->rb->r[rbrl->index]->next;
    }
    
    return 0;
}



