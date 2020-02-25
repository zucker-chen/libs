#include "ringbuf.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>


#define pri_dbg(format, args...) fprintf(stderr,"%s %d %s() " format, __FILE__, __LINE__, __func__, ## args)
#define N 1000*1024
void ringbuf_test(void)
{
    uint8_t testbuf[1000+280];
    ringbuf_t *rb;
    ringbuf_rlink_t    rb_read;
    uint8_t src[N] = "\0", dst[N] = "\0";
    
    pri_dbg(" Start.....\n");
    ringbuf_create(&rb, testbuf, 1000+280);
    ringbuf_read_add(rb, &rb_read);
    pri_dbg("capacity = %lu, sizeof(ringbuf_t) = %lu, sizeof(ringbuf_unit_t) = %lu\n", rb_read.rb->capacity, sizeof(ringbuf_t), sizeof(ringbuf_unit_t));
    
    // ====> test 1 for write and read
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
    // read 1 = 100 bytes
    ringbuf_read_get_unit(&rb_read, &q, &size);
    memcpy(dst, q, size);
    ringbuf_read_put_unit(&rb_read);
    //pri_dbg("dst = %s, size = %d\n",dst, size);
    assert(100 == size);
    // read 2 = 1 byte
    ringbuf_read_get_unit(&rb_read, &q, &size);
    memcpy(dst, q, size);
    ringbuf_read_put_unit(&rb_read);
    //pri_dbg("dst = %s, size = %d\n",dst, size);
    assert(1 == size);
  
    // ====> test 2 for write and read circle.
	srand((unsigned int)time(NULL));
	for (i = 0; i < N; i++)
	{
		src[i] = (uint8_t)(rand() % 256);
	}
	// write 222 bytes
    assert(0 == ringbuf_write_get_unit(rb, &p, 222));
    memcpy(p, src, 222);
    assert(0 == ringbuf_write_put_unit(rb, 222));
	// write 666 bytes
    assert(0 == ringbuf_write_get_unit(rb, &p, 666));    // overrun(overflow), 100+1+222+666+24>1000
    memcpy(p, src+222, 666);
    assert(0 == ringbuf_write_put_unit(rb, 666));
	// write 113 bytes
    assert(0 == ringbuf_write_get_unit(rb, &p, 1000 - 888 + 1));
    memcpy(p, src+222+666, 1000 - 888 + 1);
    assert(0 == ringbuf_write_put_unit(rb, 1000 - 888 + 1));
	// write 123 bytes
    assert(0 == ringbuf_write_get_unit(rb, &p, 123));
    memcpy(p, src+222+666+113, 123);
    assert(0 == ringbuf_write_put_unit(rb, 123));
	// read 222 bytes
    assert(0 == ringbuf_read_get_unit(&rb_read, &q, &size));
    memcpy(dst, q, size);
    assert(0 == ringbuf_read_put_unit(&rb_read));
    pri_dbg("size = %d\n", size);
    assert(666 == size);
	// read 113 bytes
    assert(0 == ringbuf_read_get_unit(&rb_read, &q, &size));
    memcpy(dst+666, q, size);
    pri_dbg("size = %d\n", size);
    assert(0 == ringbuf_read_put_unit(&rb_read));
    assert(113 == size);

    assert(0 == memcmp(src+222, dst, 666+113));
    pri_dbg("End.....\n");
  
    return;
}

void ringbuf_test1(void)
{
    uint8_t testbuf[1000+280];
    ringbuf_t *rb;
    ringbuf_rlink_t    rb_read;
    uint8_t src[N] = "\0", dst[N] = "\0";
    
    pri_dbg(" Start.....\n");
    ringbuf_create(&rb, testbuf, 1000+280);
    ringbuf_read_add(rb, &rb_read);
    pri_dbg("capacity = %lu, sizeof(ringbuf_t) = %lu, sizeof(ringbuf_unit_t) = %lu\n",rb_read.rb->capacity, sizeof(ringbuf_t), sizeof(ringbuf_unit_t));
    
    // 
    uint8_t *p, *q;
    int size, i;
	srand((unsigned int)time(NULL));
	for (i = 0; i < N; i++)
		src[i] = (uint8_t)(rand() % 256);

	// write 888 bytes
    assert(0 == ringbuf_write_get_unit(rb, &p, 888));
    memcpy(p, src, 888);
    assert(0 == ringbuf_write_put_unit(rb, 888));
	// write 113 bytes
    assert(0 == ringbuf_write_get_unit(rb, &p, 1000 - 888 + 1));
    memcpy(p, src+888, 1000 - 888 + 1);
    assert(0 == ringbuf_write_put_unit(rb, 1000 - 888 + 1));
	// write 123 bytes
    assert(0 == ringbuf_write_get_unit(rb, &p, 123));
    memcpy(p, src+888+113, 123);
    assert(0 == ringbuf_write_put_unit(rb, 123));
	// read 113 bytes
    assert(0 == ringbuf_read_get_unit(&rb_read, &q, &size));
    memcpy(dst, q, size);
    assert(0 == ringbuf_read_put_unit(&rb_read));
    assert(113 == size);
	// read 123 bytes
    assert(0 == ringbuf_read_get_unit(&rb_read, &q, &size));
    memcpy(dst+113, q, size);
    //pri_dbg("size = %d\n", size);
    assert(0 == ringbuf_read_put_unit(&rb_read));
    assert(123 == size);

    assert(0 == memcmp(src+888, dst, 123+113));

	// ringbuf seek test
	// write 222 bytes
    assert(0 == ringbuf_write_get_unit(rb, &p, 222));
    memcpy(p, src, 222);
    assert(0 == ringbuf_write_put_unit(rb, 222));
	// write 666 bytes
    assert(0 == ringbuf_write_get_unit(rb, &p, 666));    // overrun(overflow), 100+1+222+666+24>1000
    memcpy(p, src+222, 666);
    assert(0 == ringbuf_write_put_unit(rb, 666));
	// write 113 bytes
    assert(0 == ringbuf_write_get_unit(rb, &p, 1000 - 888 + 1));
    memcpy(p, src+222+666, 1000 - 888 + 1);
    assert(0 == ringbuf_write_put_unit(rb, 1000 - 888 + 1));
	// write 123 bytes
    assert(0 == ringbuf_write_get_unit(rb, &p, 123));
    memcpy(p, src+222+666+113, 123);
    assert(0 == ringbuf_write_put_unit(rb, 123));
	// read 666 bytes
    assert(0 == ringbuf_read_get_unit(&rb_read, &q, &size));
    memcpy(dst, q, size);
    assert(0 == ringbuf_read_put_unit(&rb_read));
    pri_dbg("size = %d\n", size);
    assert(666 == size);
    assert(0 == ringbuf_read_seek(&rb_read, -1));
    assert(0 == ringbuf_read_seek(&rb_read, 1));
	// read 113 bytes
    assert(0 == ringbuf_read_get_unit(&rb_read, &q, &size));
    memcpy(dst, q, size);
    assert(0 == ringbuf_read_put_unit(&rb_read));
    pri_dbg("size = %d\n", size);
    assert(113 == size);
    assert(-2 == ringbuf_read_seek(&rb_read, -8));
    assert(-3 == ringbuf_read_seek(&rb_read, 1));
	// read 123 bytes
    assert(0 == ringbuf_read_get_unit(&rb_read, &q, &size));
    memcpy(dst, q, size);
    assert(0 == ringbuf_read_put_unit(&rb_read));
    pri_dbg("size = %d\n", size);
    assert(123 == size);


	pri_dbg("End.....\n");
    
    return;
}


int main (int argc, char *argv[])
{
	ringbuf_test();
	ringbuf_test1();

	return 0;
}



