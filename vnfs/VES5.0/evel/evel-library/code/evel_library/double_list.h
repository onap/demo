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

/**************************************************************************//**
 * @file
 * A simple double-linked list.
 *
 * @note  No thread protection so you will need to use appropriate
 * synchronization if use spans multiple threads.
 *
 ****************************************************************************/

#ifndef DOUBLE_LIST_INCLUDED
#define DOUBLE_LIST_INCLUDED

typedef struct dlist_item
{
  struct dlist_item * previous;
  struct dlist_item * next;
  void * item;
} DLIST_ITEM;

/**************************************************************************//**
 * Double-linked list structure
 *****************************************************************************/
typedef struct dlist
{
  DLIST_ITEM * head;
  DLIST_ITEM * tail;
} DLIST;


void dlist_initialize(DLIST * list);
void * dlist_pop_last(DLIST * list);
void dlist_push_first(DLIST * list, void * item);
void dlist_push_last(DLIST * list, void * item);
DLIST_ITEM * dlist_get_first(DLIST * list);
DLIST_ITEM * dlist_get_last(DLIST * list);
DLIST_ITEM * dlist_get_next(DLIST_ITEM * item);
int dlist_is_empty(DLIST * list);
int dlist_count(DLIST * list);

#endif
