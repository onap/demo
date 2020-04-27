
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
 *
 ****************************************************************************/

/**************************************************************************//**
 * @file
 * A simple double-linked list.
 *
 * @note  No thread protection so you will need to use appropriate
 * synchronization if use spans multiple threads.
 *
 ****************************************************************************/

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
