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
 * Implementation of EVEL functions relating to Other.
 *
 ****************************************************************************/

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "evel.h"
#include "evel_internal.h"

/**************************************************************************//**
 * Create a new Other event.
 *
 * @note    The mandatory fields on the Other must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          Other has immutable properties.
 * @returns pointer to the newly manufactured ::EVENT_OTHER.  If the event is
 *          not used (i.e. posted) it must be released using ::evel_free_other.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_OTHER * evel_new_other(const char *ev_name, const char *ev_id)
{
  EVENT_OTHER * other = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/

  /***************************************************************************/
  /* Allocate the Other.                                                     */
  /***************************************************************************/
  other = malloc(sizeof(EVENT_OTHER));
  if (other == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(other, 0, sizeof(EVENT_OTHER));
  EVEL_DEBUG("New Other is at %lp", other);

  /***************************************************************************/
  /* Initialize the header & the Other fields.  Optional string values are   */
  /* uninitialized (NULL).                                                   */
  /***************************************************************************/
  evel_init_header_nameid(&other->header,ev_name,ev_id);
  other->header.event_domain = EVEL_DOMAIN_OTHER;
  other->major_version = EVEL_OTHER_EVENT_MAJOR_VERSION;
  other->minor_version = EVEL_OTHER_EVENT_MINOR_VERSION;

  other->namedarrays = NULL;
  dlist_initialize(&other->jsonobjects);
  dlist_initialize(&other->namedvalues);

exit_label:
  EVEL_EXIT();
  return other;
}

/**************************************************************************//**
 * Set the Event Type property of the Other.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param other       Pointer to the Other.
 * @param type        The Event Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_other_type_set(EVENT_OTHER * other,
                         const char * const type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_header_type_set.                      */
  /***************************************************************************/
  assert(other != NULL);
  assert(other->header.event_domain == EVEL_DOMAIN_OTHER);
  evel_header_type_set(&other->header, type);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a json object to jsonObject list.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param other         Pointer to the Other.
 * @param size          size of hashtable
 *****************************************************************************/
void evel_other_field_set_namedarraysize(EVENT_OTHER * other, const int size)
{
  OTHER_FIELD * other_field = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(other != NULL);
  assert(other->header.event_domain == EVEL_DOMAIN_OTHER);
  assert(other->namedarrays == NULL);
  assert(size > 0);

  EVEL_DEBUG("Adding Named array");

  other->namedarrays =  ht_create(size);

  EVEL_EXIT();
}


/**************************************************************************//**
 * Add a json object to jsonObject list.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param other         Pointer to the Other.
 * @param size          size of hashtable
 *****************************************************************************/
void evel_other_field_add_namedarray(EVENT_OTHER * other, const char *hashname,  char * name, char *value)
{
  OTHER_FIELD * other_field = NULL;
  DLIST *list = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(other != NULL);
  assert(other->header.event_domain == EVEL_DOMAIN_OTHER);
  assert(other->namedarrays != NULL);

  EVEL_DEBUG("Adding values to Named array");
      
  EVEL_DEBUG("Adding name=%s value=%s", name, value);
  other_field = malloc(sizeof(OTHER_FIELD));
  assert(other_field != NULL);
  memset(other_field, 0, sizeof(OTHER_FIELD));
  other_field->name = strdup(name);
  other_field->value = strdup(value);
  assert(other_field->name != NULL);
  assert(other_field->value != NULL);


  list = ht_get(other->namedarrays, hashname);
  if( list == NULL )
  {
     DLIST * nlist = malloc(sizeof(DLIST));
     dlist_initialize(nlist);
     dlist_push_last(nlist, other_field);
     ht_set(other->namedarrays, hashname, nlist);
     EVEL_DEBUG("Created to new table table");
  }
  else
  {
     dlist_push_last(list, other_field);
     EVEL_DEBUG("Adding to existing table");
  }

  EVEL_EXIT();
}


/**************************************************************************//**
 * Add a json object to jsonObject list.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param other     Pointer to the Other.
 * @param jsonobj   Pointer to json object
 *****************************************************************************/
void evel_other_field_add_jsonobj(EVENT_OTHER * other, EVEL_JSON_OBJECT *jsonobj)
{
  OTHER_FIELD * other_field = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(other != NULL);
  assert(other->header.event_domain == EVEL_DOMAIN_OTHER);
  assert(jsonobj != NULL);

  EVEL_DEBUG("Adding jsonObject");

  dlist_push_last(&other->jsonobjects, jsonobj);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a field name/value pair to the Other.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param other     Pointer to the Other.
 * @param name      ASCIIZ string with the field's name.  The caller does not
 *                  need to preserve the value once the function returns.
 * @param value     ASCIIZ string with the field's value.  The caller does not
 *                  need to preserve the value once the function returns.
 *****************************************************************************/
void evel_other_field_add(EVENT_OTHER * other, char * name, char * value)
{
  OTHER_FIELD * other_field = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(other != NULL);
  assert(other->header.event_domain == EVEL_DOMAIN_OTHER);
  assert(name != NULL);
  assert(value != NULL);

  EVEL_DEBUG("Adding name=%s value=%s", name, value);
  other_field = malloc(sizeof(OTHER_FIELD));
  assert(other_field != NULL);
  memset(other_field, 0, sizeof(OTHER_FIELD));
  other_field->name = strdup(name);
  other_field->value = strdup(value);
  assert(other_field->name != NULL);
  assert(other_field->value != NULL);

  dlist_push_last(&other->namedvalues, other_field);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Encode the Other in JSON according to AT&T's schema for the event type.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_other(EVEL_JSON_BUFFER * jbuf,
                            EVENT_OTHER * event)
{
  OTHER_FIELD * other_field = NULL;
  EVEL_JSON_OBJECT * jsonobjp = NULL;
  DLIST_ITEM * other_field_item = NULL;
  EVEL_JSON_OBJECT_INSTANCE * jsonobjinst = NULL;
  DLIST_ITEM * jsobj_field_item = NULL;
  EVEL_INTERNAL_KEY * keyinst = NULL;
  DLIST_ITEM * keyinst_field_item = NULL;
  HASHTABLE_T *ht = NULL;
  int i;
  bool itm_added = false;
  DLIST *itm_list = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_OTHER);

  evel_json_encode_header(jbuf, &event->header);
  evel_json_open_named_object(jbuf, "otherFields");

// iterate through hashtable and print DLIST for each entry

   ht = event->namedarrays;
   if( ht != NULL )
   {
     if( ht->size > 0)
     {
        for( i = 0; i < ht->size; i++ ) {
             if( ht->table[i] != NULL)
	     {
		itm_added = true;
	     }
        }
        if( itm_added == true)
        {

  if (evel_json_open_opt_named_list(jbuf, "hashOfNameValuePairArrays"))
  {
       for( i = 0; i < ht->size; i++ ) {
             if( ht->table[i] != NULL)
	     {
		itm_list = ht->table[i];

  if(evel_json_open_opt_named_list(jbuf, ht->table[i]->key))
  {
    other_field_item = dlist_get_first(&itm_list);
    while (other_field_item != NULL)
    {
     other_field = (OTHER_FIELD *) other_field_item->item;
     if(other_field != NULL){
       evel_json_open_object(jbuf);
       evel_enc_kv_string(jbuf, "name", other_field->name);
       evel_enc_kv_string(jbuf, "value", other_field->value);
       evel_json_close_object(jbuf);
       other_field_item = dlist_get_next(other_field_item);
     }
    }
    evel_json_close_list(jbuf);
  }

	     }
       }

       evel_json_close_list(jbuf);
  }


        }
     }
   }

  evel_json_checkpoint(jbuf);
  if(evel_json_open_opt_named_list(jbuf, "jsonObjects"))
  {
  bool item_added = false;
  other_field_item = dlist_get_first(&event->jsonobjects);
  while (other_field_item != NULL)
  {
    jsonobjp = (EVEL_JSON_OBJECT *) other_field_item->item;
    if(jsonobjp != NULL);
    {
     evel_json_open_object(jbuf);

       if( evel_json_open_opt_named_list(jbuf, "objectInstances"))
       {
	bool item_added2 = false;
        jsobj_field_item = dlist_get_first(&jsonobjp->jsonobjectinstances);
	while (jsobj_field_item != NULL)
	{
	   jsonobjinst = (EVEL_JSON_OBJECT_INSTANCE *) jsobj_field_item->item;
	   if( jsonobjinst != NULL )
	   {
              evel_json_open_object(jbuf);
              evel_enc_kv_object(jbuf, "objectInstance", jsonobjinst->jsonstring);
              evel_enc_kv_ull(jbuf, "objectInstanceEpochMicrosec", jsonobjinst->objinst_epoch_microsec);
  //evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "objectKeys"))
  {
    bool item_added3 = false;

    keyinst_field_item = dlist_get_first(&jsonobjinst->object_keys);
    while (keyinst_field_item != NULL)
    {
      keyinst = (EVEL_INTERNAL_KEY *)keyinst_field_item->item;
      if(keyinst != NULL)
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_string(jbuf, "keyName", keyinst->keyname);
        evel_enc_kv_opt_int(jbuf, "keyOrder", &keyinst->keyorder);
        evel_enc_kv_opt_string(jbuf, "keyValue", &keyinst->keyvalue);
        evel_json_close_object(jbuf);
	item_added3 = false;
      }
      keyinst_field_item = dlist_get_next(keyinst_field_item);
    }
    evel_json_close_list(jbuf);

    /*************************************************************************/
    /* If we've not written anything, rewind to before we opened the list.   */
    /*************************************************************************/
    //if (!item_added3)
    //{
    //  evel_json_rewind(jbuf);
    //}
  }
	       evel_json_close_object(jbuf);
	    }
	    item_added2 = true;
            jsobj_field_item = dlist_get_next(jsobj_field_item);
	}
        evel_json_close_list(jbuf);
	if( !item_added2 )
	{
	  evel_json_rewind(jbuf);
	}
       }

    evel_enc_kv_string(jbuf, "objectName", jsonobjp->object_name);
    evel_enc_kv_opt_string(jbuf, "objectSchema", &jsonobjp->objectschema);
    evel_enc_kv_opt_string(jbuf, "objectSchemaUrl", &jsonobjp->objectschemaurl);
    evel_enc_kv_opt_string(jbuf, "nfSubscribedObjectName", &jsonobjp->nfsubscribedobjname);
    evel_enc_kv_opt_string(jbuf, "nfSubscriptionId", &jsonobjp->nfsubscriptionid);
    evel_json_close_object(jbuf);
    item_added = true;
  }
  other_field_item = dlist_get_next(other_field_item);
  }
  evel_json_close_list(jbuf);

  if (!item_added)
  {
     evel_json_rewind(jbuf);
  }

  }

  if( evel_json_open_opt_named_list(jbuf, "nameValuePairs"))
  {
  other_field_item = dlist_get_first(&event->namedvalues);
  while (other_field_item != NULL)
  {
    other_field = (OTHER_FIELD *) other_field_item->item;
    assert(other_field != NULL);

    evel_json_open_object(jbuf);
    evel_enc_kv_string(jbuf, "name", other_field->name);
    evel_enc_kv_string(jbuf, "value", other_field->value);
    evel_json_close_object(jbuf);
    other_field_item = dlist_get_next(other_field_item);
  }
  }
  evel_json_close_list(jbuf);

  evel_enc_version(jbuf, "otherFieldsVersion", event->major_version,event->minor_version);

  evel_json_close_object(jbuf);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free an Other.
 *
 * Free off the Other supplied.  Will free all the contained allocated memory.
 *
 * @note It does not free the Other itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_other(EVENT_OTHER * event)
{
  OTHER_FIELD * other_field = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_OTHER);

  /***************************************************************************/
  /* Free all internal strings then the header itself.                       */
  /***************************************************************************/
  other_field = dlist_pop_last(&event->namedvalues);
  while (other_field != NULL)
  {
    EVEL_DEBUG("Freeing Other Field (%s, %s)",
               other_field->name,
               other_field->value);
    free(other_field->name);
    free(other_field->value);
    free(other_field);
    other_field = dlist_pop_last(&event->namedvalues);
  }
  evel_free_header(&event->header);

  EVEL_EXIT();
}
