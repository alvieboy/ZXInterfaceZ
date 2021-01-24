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

#include "ringbuffer.h"
#include <stdbool.h>
#include <stdio.h>

void ringbuffer__init(ringbuffer_t *ring, uint8_t *buffer, unsigned size)
{
    ring->buf   = buffer;
    ring->size  = size;
    ring->head = 0;
    ring->tail = 0;
}

void ringbuffer__reset(ringbuffer_t *ring)
{
    ring->head = 0;
    ring->tail = 0;
}

unsigned ringbuffer__avail(ringbuffer_t *ring)
{
    return ring->size - ringbuffer__size(ring);

}

unsigned ringbuffer__size(ringbuffer_t *ring)
{
    int delta = ring->head - ring->tail;
    if (delta<0) {
        return ring->size + delta +1;
    }
    return delta;
}

unsigned ringbuffer__get(ringbuffer_t *ring, uint8_t *dest, unsigned maxsize)
{
    unsigned size = ringbuffer__size(ring);
    unsigned count = 0;
    while ((size>0) && (maxsize>0)) {
        ringbuffer__dump(ring);
        count++;
        *dest++ = ring->buf[ring->tail];
        ring->tail++;
        if (ring->tail > ring->size)
            ring->tail = 0;
        size--;
        maxsize--;
    }
    return count;
}

void ringbuffer__append(ringbuffer_t *ring, const uint8_t *data, unsigned size)
{
    bool overflow = false;
    // Assumption: size <= ring->size
    if (size>ringbuffer__avail(ring)) {
        overflow = true;
    }
    // TODO: convert this into memcpy()
    while (size--) {
        ring->buf[ring->head] = *data;
        data++;
        ring->head++;
        if (ring->head > ring->size)
            ring->head = 0;
    }
    if (overflow) {
        int16_t newtail = ring->head + 1;
        if ( newtail > ring->size)
            newtail = 0;
        ring->tail = (uint16_t)newtail;
    }
}


void ringbuffer__dump(ringbuffer_t *ring)
{
#ifdef __linux__
    printf("D: head %d tail %d avail %d\n", ring->head, ring->tail, ringbuffer__avail(ring));
#endif
}
