/*
  ******************************************************************************
  * (C) 2018 Alvaro Lopes <alvieboy@alvie.com>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of Alvaro Lopes nor the names of contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include <inttypes.h>
#include <stdbool.h>

typedef struct 
{
    uint8_t *buf;
    uint16_t head, tail;
    uint16_t size;
} ringbuffer_t;

/**
 * @brief Initialize a new ringbuffer with specified size
 *
 * Buffer should be designed to hold "size" elements, while maintaining
 * ability to detect full/empty situation.
 *
 * @param ring The ringbuffer to initialise
 * @param buffer The pre-allocated buffer to use. Assumed contiguous and
 *  at least (size+1) bytes long
 * @param suze The requested size of the ringbuffer.
 *
 */
void ringbuffer__init(ringbuffer_t *ring, uint8_t *buffer, unsigned size);

/**
 * @brief Reset a ringbuffer (empty the buffer contents)
 *
 * @param ring The ringbuffer
 */
void ringbuffer__reset(ringbuffer_t *ring);

/**
 * @brief Return number of bytes available in ringbuffer
 *
 * @param ring The ringbuffer
 * @return The number of bytes available for storage
 */
unsigned ringbuffer__avail(ringbuffer_t *ring);


/**
 * @brief Return if ringbuffer is empty
 *
 * @param ring The ringbuffer
 * @return true if ringbuffer is empty, false otherwise
 */
static inline bool ringbuffer__empty(ringbuffer_t *ring);

/**
 * @brief Return number of bytes used in ringbuffer
 *
 * @param ring The ringbuffer
 * @return The number of bytes used
 */
unsigned ringbuffer__size(ringbuffer_t *ring);

/**
 * @brief Copy ringbuffer data into target buffer
 *
 * @param ring The ringbuffer
 * @param dest Pointer to the destination
 * @param maxsize Maximum number of bytes to copy
 *
 * @return Number of bytes copied. May be less than "maxsize".
 */
unsigned ringbuffer__get(ringbuffer_t *ring, uint8_t *dest, unsigned maxsize);

/**
 * @brief Append data to the ringbuffer
 *
 * In case data does not fit in buffer, older data shall be discarded.
 *
 * @param ring The ringbuffer
 * @param data Data do place in buffer
 * @param size Size of data. It's assumed less than max ringbuffer size
 */
void ringbuffer__append(ringbuffer_t *ring, const uint8_t *data, unsigned size);

void ringbuffer__dump(ringbuffer_t *);

/*
 *
 * Inline functions
 *
 */

static inline bool ringbuffer__empty(ringbuffer_t *ring)
{
    return ( ring->head == ring->tail );
}


#endif
