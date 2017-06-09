/*************************************************************************//**
 *
 * Copyright © 2017 AT&T Intellectual Property. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 *
 ****************************************************************************/

#ifndef RING_BUFFER_INCLUDED
#define RING_BUFFER_INCLUDED

/**************************************************************************//**
 * @file
 * Ring  buffer to handle message requests.
 *
 ****************************************************************************/

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
