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

#ifndef HASHTABLE_INCLUDED
#define HASHTABLE_INCLUDED

/**************************************************************************//**
 * @file
 * A simple hashtable.
 *
 * @note  No thread protection so you will need to use appropriate
 * synchronization if use spans multiple threads.
 *
 ****************************************************************************/

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

#endif
