#ifndef _ringbuf_h_
#define _ringbuf_h_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ringbuf_unit_s {
    struct ringbuf_unit_s *prev;
    struct ringbuf_unit_s *next;
    size_t size;
    uint8_t data[0];
} ringbuf_unit_t;

typedef struct ringbuf_s {
    uint8_t *ptr;
    size_t capacity;
    #define RB_MAX_READ_NUM	32
    ringbuf_unit_t *r[RB_MAX_READ_NUM];    // Pointer array, supports multi-path simultaneous reading of ringbuf
    ringbuf_unit_t *w; // write position
} ringbuf_t;

typedef struct ringbuf_rlink_s {
    ringbuf_t *rb;
    size_t index; // read index num
} ringbuf_rlink_t;





#if 0
typedef struct ringbuf_base_s {
    uint8_t *ptr;
    size_t capacity;
    #define RINGBUF_MAX_READ_NUM	32
    uint8_t *r[RINGBUF_MAX_READ_NUM];    // Pointer array, supports multi-path simultaneous reading of ringbuf
    uint8_t *w; // write position
} ringbuf_base_t;


typedef struct ringbuf_s
{
    ringbuf_base_t *rbb;
    size_t r_index; // read index num
} ringbuf_t;






int ringbuf_alloc(struct ringbuf_t* rb, size_t capacity);
int ringbuf_free(struct ringbuf_t* rb);
void ringbuf_clear(struct ringbuf_t* rb);

int ringbuf_write(struct ringbuf_t* rb, const void* data, size_t bytes);
int ringbuf_read(struct ringbuf_t* rb, void* data, size_t bytes);

/// @return readable element count
size_t ringbuf_size(struct ringbuf_t* rb);
/// @return writeable element count
size_t ringbuf_space(struct ringbuf_t* rb);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !_ringbuf_h_ */
