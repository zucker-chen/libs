/**
 * @file    fifoutil.h
 * @brief   
 *
 * This interface enables easy passing of fixed size messages between POSIX
 * threads in Linux using first in first out ordering. Only one reader and
 * writer per fifo is supported unless the application serializes the calls.
 *
 * @verbatim
 * ============================================================================
 * Copyright (c) Texas Instruments Inc 2007
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
 * @endverbatim
 */

#ifndef _FIFOUTIL_H
#define _FIFOUTIL_H

#include <unistd.h>

/**
 * @brief Object that stores the state of the fifo.
 */
typedef struct FifoUtil_Obj {
    size_t size;
    int pipes[2];
} FifoUtil_Obj;

/**
 * @brief Pointer to a fifo util object
 */
typedef FifoUtil_Obj *FifoUtil_Handle;

/**
 * @brief Indicates that a fifo util API call completed successfully.
 */
#define FIFOUTIL_SUCCESS 0

/**
 * @brief Indicates that a fifo util API call failed.
 */
#define FIFOUTIL_FAILURE -1

/**
 * @brief Opens the fifo. Must be called before other API:s on a fifo.
 * @param hFifo Pointer to the fifo object to open.
 * @param size Size in bytes of the messages to be passed through this fifo.
 * @return FIFOUTIL_SUCCESS for success or FIFOUTIL_FAILURE for failure.
 */
static inline int FifoUtil_open(FifoUtil_Handle hFifo, size_t size)
{
    if (pipe(hFifo->pipes)) {
        return FIFOUTIL_FAILURE;
    }

    hFifo->size = size;

    return FIFOUTIL_SUCCESS;
}

/**
 * @brief Closes the fifo. No API calls can be made on this fifo after this.
 * @param hFifo Pointer to the fifo object to close.
 * @return FIFOUTIL_SUCCESS for success or FIFOUTIL_FAILURE for failure.
 */
static inline int FifoUtil_close(FifoUtil_Handle hFifo)
{
    int ret = FIFOUTIL_SUCCESS;

    if (close(hFifo->pipes[0])) {
        ret = FIFOUTIL_FAILURE;
    }

    if (close(hFifo->pipes[1])) {
        ret = FIFOUTIL_FAILURE;
    }

    return ret;
}

/**
 * @brief Blocking call to get a message from a fifo.
 * @param hFifo Pointer to a previously opened fifo object.
 * @param buffer A pointer to the buffer which will be copied to the fifo.
 * @return FIFOUTIL_SUCCESS for success or FIFOUTIL_FAILURE for failure.
 */
static inline int FifoUtil_get(FifoUtil_Handle hFifo, void *buffer)
{
    if (read(hFifo->pipes[0], buffer, hFifo->size) != hFifo->size) {
        return FIFOUTIL_FAILURE;
    }

    return FIFOUTIL_SUCCESS;
}

/**
 * @brief Put a message on the fifo.
 * @param hFifo Pointer to a previously opened fifo object.
 * @param buffer A pointer to the buffer which will be copied from the fifo.
 * @return FIFOUTIL_SUCCESS for success or FIFOUTIL_FAILURE for failure.
 */
static inline int FifoUtil_put(FifoUtil_Handle hFifo, void *buffer)
{
    if (write(hFifo->pipes[1], buffer, hFifo->size) != hFifo->size) {
        return FIFOUTIL_FAILURE;
    }

    return FIFOUTIL_SUCCESS;
}

#endif // _FIFOUTIL_H
