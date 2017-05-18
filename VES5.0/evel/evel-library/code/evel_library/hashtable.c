/**************************************************************************//**
 * @file
 * A simple Hashtable.
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

#include <limits.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>

#include "hashtable.h"

/**************************************************************************//**
 * Hashtable initialization.
 *
 * Initialize the list supplied to be empty.
 *
 * @param   size  Size of hashtable

 * @returns Hashtable pointer
******************************************************************************/
/* Create a new hashtable. */
HASHTABLE_T *ht_create( size_t size ) {

	HASHTABLE_T *hashtable = NULL;
	size_t i;

	if( size < 1 ) return NULL;

	/* Allocate the table itself. */
	if( ( hashtable = malloc( sizeof( HASHTABLE_T ) ) ) == NULL ) {
		return NULL;
	}

	/* Allocate pointers to the head nodes. */
	if( ( hashtable->table = malloc( sizeof( ENTRY_T * ) * size ) ) == NULL ) {
		return NULL;
	}
	for( i = 0; i < size; i++ ) {
		hashtable->table[i] = NULL;
	}

	hashtable->size = size;

	return hashtable;	
}

/**************************************************************************//**
 * Hash a string for a particular hash table. 
 *
 * Initialize the list supplied to be empty.
 *
 * @param   hashtable    Pointer to the hashtable
 * @param   key          String

 * @returns hashvalue
******************************************************************************/
size_t ht_hash( HASHTABLE_T *hashtable, char *key )
{

    size_t hash, i;

#ifdef HASHTABLE_USE_SIMPLE_HASH
    for ( hash = i = 0; i < strlen(key); hash = hash << 8, hash += key[ i++ ] );
#else /* Use: Jenkins' "One At a Time Hash" === Perl "Like" Hashing */
    // http://en.wikipedia.org/wiki/Jenkins_hash_function
    for ( hash = i = 0; i < strlen(key); ++i ) {
        hash += key[i], hash += ( hash << 10 ), hash ^= ( hash >> 6 );
    }
    hash += ( hash << 3 ), hash ^= ( hash >> 11 ), hash += ( hash << 15 );
#endif

    return hash % hashtable->size;

}

/**************************************************************************//**
 * Create a key-value pair.
 *
 * @param   key     key string
 * @param   value   value string
 *
 * @returns hashtable entry
******************************************************************************/
ENTRY_T *ht_newpair( char *key, void *value )
{
	ENTRY_T *newpair;

	if( ( newpair = malloc( sizeof( ENTRY_T ) ) ) == NULL ) {
		return NULL;
	}

	if( ( newpair->key = strdup( key ) ) == NULL ) {
		return NULL;
	}

	if( ( newpair->value =  value ) == NULL ) {
		return NULL;
	}

	newpair->next = NULL;

	return newpair;
}

/**************************************************************************//**
 * Insert a key-value pair into a hash table.
 *
 * @param   key     key string
 * @param   value   value string
 *
 * @returns Nothing
******************************************************************************/
void ht_set( HASHTABLE_T *hashtable, char *key, void *value ) {
	size_t bin = 0;
	ENTRY_T *newpair = NULL;
	ENTRY_T *next = NULL;
	ENTRY_T *last = NULL;

	bin = ht_hash( hashtable, key );

	next = hashtable->table[ bin ];

	while( next != NULL && next->key != NULL && strcmp( key, next->key ) > 0 ) {
		last = next;
		next = next->next;
	}

	/* There's already a pair.  Let's replace that string. */
	if( next != NULL && next->key != NULL && strcmp( key, next->key ) == 0 ) {

		free( next->value );
		next->value = value ;

	/* Nope, could't find it.  Time to grow a pair. */
	} else {
		newpair = ht_newpair( key, value );

		/* We're at the start of the linked list in this bin. */
		if( next == hashtable->table[ bin ] ) {
			newpair->next = next;
			hashtable->table[ bin ] = newpair;
	
		/* We're at the end of the linked list in this bin. */
		} else if ( next == NULL ) {
			last->next = newpair;
	
		/* We're in the middle of the list. */
		} else  {
			newpair->next = next;
			last->next = newpair;
		}
	}
}

/**************************************************************************//**
 *  Retrieve a key-value pair from a hash table.
 *
 * @param   key     key string
 *
 * @returns  value string
******************************************************************************/
void *ht_get( HASHTABLE_T *hashtable, char *key ) {
	size_t bin = 0;
	ENTRY_T *pair;

	bin = ht_hash( hashtable, key );

	/* Step through the bin, looking for our value. */
	pair = hashtable->table[ bin ];
	while( pair != NULL && pair->key != NULL && strcmp( key, pair->key ) > 0 ) {
		pair = pair->next;
	}

	/* Did we actually find anything? */
	if( pair == NULL || pair->key == NULL || strcmp( key, pair->key ) != 0 ) {
		return NULL;

	} else {
		return pair->value;
	}
	
}

/*
int main( int argc, char **argv ) {

	HASHTABLE_T *hashtable = ht_create( 65536 );

	ht_set( hashtable, "key1", "inky" );
	ht_set( hashtable, "key2", "pinky" );
	ht_set( hashtable, "key3", "blinky" );
	ht_set( hashtable, "key4", "floyd" );

	printf( "%s\n", ht_get( hashtable, "key1" ) );
	printf( "%s\n", ht_get( hashtable, "key2" ) );
	printf( "%s\n", ht_get( hashtable, "key3" ) );
	printf( "%s\n", ht_get( hashtable, "key4" ) );

	return 0;
}
*/
