/**************************************************************************//**
 * @file
 * A simple double-linked list.
 *
 * @note  No thread protection so you will need to use appropriate
 * synchronization if use spans multiple threads.
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

#include <assert.h>
#include <malloc.h>

#include "double_list.h"
#include "evel.h"

/**************************************************************************//**
 * List initialization.
 *
 * Initialize the list supplied to be empty.
 *
 * @param   list    Pointer to the list to be initialized.

 * @returns Nothing
******************************************************************************/
void dlist_initialize(DLIST * list)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(list != NULL);

  /***************************************************************************/
  /* Initialize the list as empty.                                           */
  /***************************************************************************/
  list->head = NULL;
  list->tail = NULL;

  EVEL_EXIT();
}

void * dlist_pop_last(DLIST * list)
{
  void *item = NULL;
  DLIST_ITEM *current_tail = NULL;
  DLIST_ITEM *new_tail = NULL;

  assert(list != NULL);

  current_tail = list->tail;
  if (current_tail != NULL)
  {
    item = current_tail->item;
    new_tail = current_tail->previous;
    if (new_tail == NULL)
    {
      list->head = NULL;
      list->tail = NULL;
    }
    else
    {
      new_tail->next = NULL;
      list->tail = new_tail;
    }
    free(current_tail);
  }

  return item;
}

void dlist_push_first(DLIST * list, void * item)
{
  DLIST_ITEM * new_element = NULL;
  DLIST_ITEM * current_head = NULL;

  /***************************************************************************/
  /* Check assumptions.  Note that we do allow putting NULL pointers into    */
  /* the list - not sure you'd want to, but let it happen.                   */
  /***************************************************************************/
  assert(list != NULL);

  current_head = list->head;

  new_element = malloc(sizeof(DLIST_ITEM));
  assert(new_element != NULL);
  new_element->next = current_head;
  new_element->previous = NULL;
  new_element->item = item;
  list->head = new_element;

  if (current_head != NULL)
  {
    current_head->previous = new_element;
  }
  else
  {
    list->tail = new_element;
  }
}

void dlist_push_last(DLIST * list, void * item)
{
  DLIST_ITEM * new_element = NULL;
  DLIST_ITEM * current_tail = NULL;

  /***************************************************************************/
  /* Check assumptions.  Note that we do allow putting NULL pointers into    */
  /* the list - not sure you'd want to, but let it happen.                   */
  /***************************************************************************/
  assert(list != NULL);

  current_tail = list->tail;

  new_element = malloc(sizeof(DLIST_ITEM));
  assert(new_element != NULL);
  new_element->next = NULL;
  new_element->previous = current_tail;
  new_element->item = item;
  list->tail = new_element;

  if (current_tail != NULL)
  {
    current_tail->next = new_element;
  }
  else
  {
    list->head = new_element;
  }
}

DLIST_ITEM * dlist_get_first(DLIST * list)
{
  return list->head;
}

DLIST_ITEM * dlist_get_last(DLIST * list)
{
  return list->tail;
}

DLIST_ITEM * dlist_get_next(DLIST_ITEM * item)
{
  return item->next;
}

int dlist_is_empty(DLIST * list)
{
  return (list->head == NULL);
}

int dlist_count(DLIST * list)
{
  int count = 0;
  DLIST_ITEM * item = list->head;

  while (item != NULL)
  {
    count++;
    item = item->next;
  }

  return count;
}
