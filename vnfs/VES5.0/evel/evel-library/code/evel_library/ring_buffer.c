/*************************************************************************//**
 *
 * Copyright © 2017 AT&T Intellectual Property. All rights reserved.
 *
 * Unless otherwise specified, all software contained herein is
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
 * ECOMP is a trademark and service mark of AT&T Intellectual Property.
 ****************************************************************************/
/**************************************************************************//**
 * @file
 * A ring buffer with multi-threaded synchronization.
 *
 ****************************************************************************/

#include <assert.h>
#include <malloc.h>

#include "ring_buffer.h"
#include "evel.h"

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
void ring_buffer_initialize(ring_buffer * buffer, int size)
{
  int pthread_rc = 0;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(buffer != NULL);
  assert(size > 0);

  /***************************************************************************/
  /* Initialize the synchronization objects.                                 */
  /***************************************************************************/
  pthread_rc = pthread_mutex_init(&buffer->ring_mutex, NULL);
  assert(pthread_rc == 0);
  pthread_rc = pthread_cond_init(&buffer->ring_cv, NULL);
  assert(pthread_rc == 0);

  /***************************************************************************/
  /* Allocate the ring buffer itself.                                        */
  /***************************************************************************/
  buffer->ring = malloc(size * sizeof(void *));
  assert(buffer->ring != NULL);

  /***************************************************************************/
  /* Initialize the ring as empty.                                           */
  /***************************************************************************/
  buffer->next_write = 0;
  buffer->next_read = 0;
  buffer->size = size;

  EVEL_EXIT();
}

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
void * ring_buffer_read(ring_buffer * buffer)
{
  void *msg = NULL;
  EVEL_DEBUG("RBR: Ring buffer read");

  pthread_mutex_lock(&buffer->ring_mutex);
  while (1)
  {
    EVEL_DEBUG("RBR: got lock. NR=%d NW=%d",
               buffer->next_read,
               buffer->next_write);
    if(buffer->next_read != buffer->next_write)
    {
      EVEL_DEBUG("RBR: buffer has item available");
      msg = (buffer->ring)[buffer->next_read];
      buffer->ring[buffer->next_read] = NULL;
      buffer->next_read = (buffer->next_read + 1) % buffer->size;
      EVEL_DEBUG("RBR: next read location is %d", buffer->next_read);
      pthread_mutex_unlock(&buffer->ring_mutex);
      break;
    }
    else
    {
      EVEL_DEBUG("RBR: Waiting for condition variable");
      pthread_cond_wait(&buffer->ring_cv, &buffer->ring_mutex);
      EVEL_DEBUG("RBR: Condition variable wait completed");
    }
  }
  EVEL_DEBUG("RBR: Ring buffer read returning data at %lp", msg);
  return msg;
}

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
int ring_buffer_write(ring_buffer * buffer, void * msg)
{
  int item_count = 0;
  int items_written = 0;
  EVEL_DEBUG("RBW: Ring Buffer Write message at %lp", msg);

  pthread_mutex_lock(&buffer->ring_mutex);
  EVEL_DEBUG("RBW: got lock. NR=%d NW=%d SZ=%d",
             buffer->next_read,
             buffer->next_write,
             buffer->size);

  item_count = (buffer->next_write - buffer->next_read) % buffer->size;
  if (item_count < 0)
  {
    item_count += buffer->size;
  }
  if (item_count < buffer->size - 1)
  {
    EVEL_DEBUG("RBW: %d items in buffer", item_count);
    buffer->ring[buffer->next_write] = msg;
    buffer->next_write = (buffer->next_write + 1) % buffer->size;
    EVEL_DEBUG("RBW: next write location is %d", buffer->next_write);
    items_written = 1;
  }
  else
  {
    EVEL_ERROR("RBW: ring buffer full - unable to write event");
  }

  pthread_mutex_unlock(&buffer->ring_mutex);
  EVEL_DEBUG("RBW: released lock");
  pthread_cond_signal(&buffer->ring_cv);

  return items_written;
}

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
int ring_buffer_is_empty(ring_buffer * buffer)
{
  int is_empty = 0;
  EVEL_DEBUG("RBE: Ring empty check");

  pthread_mutex_lock(&buffer->ring_mutex);
  is_empty = (buffer->next_read == buffer->next_write);
  pthread_mutex_unlock(&buffer->ring_mutex);

  EVEL_DEBUG("RBE: Ring state= %d", is_empty);
  return is_empty;
}

