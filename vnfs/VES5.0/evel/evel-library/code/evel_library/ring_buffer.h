#ifndef RING_BUFFER_INCLUDED
#define RING_BUFFER_INCLUDED

/**************************************************************************//**
 * @file
 * A ring buffer.
 *
 * License
 * -------
 *
 * Copyright(c) <2016>, AT&T Intellectual Property.  All other rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:  This product includes
 *    software developed by the AT&T.
 * 4. Neither the name of AT&T nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY AT&T INTELLECTUAL PROPERTY ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AT&T INTELLECTUAL PROPERTY BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <pthread.h>

/**************************************************************************//**
 * Ring buffer structure.
 *****************************************************************************/
typedef struct ring_buffer
{
    int size;
    int next_write;
    int next_read;
    void ** ring;
    pthread_cond_t ring_cv;
    pthread_mutex_t ring_mutex;
} ring_buffer;

/**************************************************************************//**
 * Ring buffer initialization.
 *
 * Initialize the buffer supplied to the specified size.
 *
 * @param   buffer  Pointer to the ring-buffer to be initialized.
 * @param   size    How many elements to be stored in the ring-buffer.
 *
 * @returns Nothing
******************************************************************************/
void ring_buffer_initialize(ring_buffer * buffer, int size);

/**************************************************************************//**
 * Read an element from a ring_buffer.
 *
 * Reads an element from the ring_buffer, advancing the next-read position.
 * Operation is synchronized and therefore MT-safe.  Blocks if no data is
 * available.
 *
 * @param   buffer  Pointer to the ring-buffer to be read.
 *
 * @returns Pointer to the element read from the buffer.
******************************************************************************/
void * ring_buffer_read(ring_buffer * buffer);

/**************************************************************************//**
 * Write an element into a ring_buffer.
 *
 * Writes an element into the ring_buffer, advancing the next-write position.
 * Operation is synchronized and therefore MT-safe.  Fails if the buffer is
 * full without blocking.
 *
 * @param   buffer  Pointer to the ring-buffer to be written.
 * @param   msg     Pointer to data to be stored in the ring_buffer.
 *
 * @returns Number of items written.
 * @retval  1       The data was written successfully.
 * @retval  0       The ring_buffer was full so no data written.
******************************************************************************/
int ring_buffer_write(ring_buffer * buffer, void * msg);

/**************************************************************************//**
 * Tests whether there is data in the ring_buffer.
 *
 * Tests whether there is currently data in the ring_buffer without blocking.
 *
 * @param   buffer  Pointer to the ring-buffer to be tested.
 *
 * @returns Whether there is data in the ring_buffer.
 * @retval  0       There isn't any data in the ring_buffer.
 * @retval  1       There is data in the ring_buffer.
******************************************************************************/
int ring_buffer_is_empty(ring_buffer * buffer);

#endif
