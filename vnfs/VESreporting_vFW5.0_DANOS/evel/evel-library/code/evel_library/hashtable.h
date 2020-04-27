#ifndef HASHTABLE_INCLUDED
#define HASHTABLE_INCLUDED

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
 * A simple hashtable.
 *
 * @note  No thread protection so you will need to use appropriate
 * synchronization if use spans multiple threads.
*****************************************************************************/

typedef struct entry_s {
	char *key;
	void *value;
	struct entry_s *next;
} ENTRY_T;

/**************************************************************************//**
 * Hashtable structure
 *****************************************************************************/

typedef struct hashtable_s {
	size_t size;
	struct entry_s **table;	
} HASHTABLE_T;

/**************************************************************************//**
 * Hashtable initialization.
 *
 * Initialize the list supplied to be empty.
 *
 * @param   size  Size of hashtable

 * @returns Hashtable pointer
******************************************************************************/
/* Create a new hashtable. */
HASHTABLE_T *ht_create( size_t size );

/**************************************************************************//**
 * Hash a string for a particular hash table.
 *
 * Initialize the list supplied to be empty.
 *
 * @param   hashtable    Pointer to the hashtable
 * @param   key          String

 * @returns hashvalue
******************************************************************************/
size_t ht_hash( HASHTABLE_T *hashtable, char *key );

/**************************************************************************//**
 * Create a key-value pair.
 *
 * @param   key     key string
 * @param   value   value string
 *
 * @returns hashtable entry
******************************************************************************/
ENTRY_T *ht_newpair( char *key, void *value );

/**************************************************************************//**
 * Insert a key-value pair into a hash table.
 *
 * @param   key     key string
 * @param   value   value string
 *
 * @returns Nothing
******************************************************************************/
void ht_set( HASHTABLE_T *hashtable, char *key, void *value );

/**************************************************************************//**
 *  Retrieve a key-value pair from a hash table.
 *
 * @param   key     key string
 *
 * @returns  value string
******************************************************************************/
void *ht_get( HASHTABLE_T *hashtable, char *key );

#endif
