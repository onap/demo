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
 * Implementation of EVEL functions relating to json_object.
 *
 ****************************************************************************/

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "jsmn.h"
#include "evel.h"
#include "evel_internal.h"

/**************************************************************************//**
 * Create a new json object.
 *
 * @note    The mandatory fields on the Other must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          Other has immutable properties.
 * @param name       name of the object.
 * @returns pointer to the newly manufactured ::EVEL_JSON_OBJECT.
 *          not used (i.e. posted) it must be released using ::evel_free_jsonobject.
 * @retval  NULL  Failed to create the json object.
 *****************************************************************************/
EVEL_JSON_OBJECT * evel_new_jsonobject(const char *const name)
{
  EVEL_JSON_OBJECT *jobj = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(name != NULL);

  /***************************************************************************/
  /* Allocate the json object.                                                     */
  /***************************************************************************/
  jobj = malloc(sizeof(EVEL_JSON_OBJECT));
  if (jobj == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(jobj, 0, sizeof(EVEL_JSON_OBJECT));
  EVEL_DEBUG("New json object is at %lp", jobj);

  /***************************************************************************/
  /* Initialize the fields.  Optional string values are   */
  /* uninitialized (NULL).                                                   */
  /***************************************************************************/
  jobj->object_name = strdup(name);
  evel_init_option_string(&jobj->objectschema);
  evel_init_option_string(&jobj->objectschemaurl);
  evel_init_option_string(&jobj->nfsubscribedobjname);
  evel_init_option_string(&jobj->nfsubscriptionid);
  dlist_initialize(&jobj->jsonobjectinstances);

exit_label:
  EVEL_EXIT();
  return jobj;
}


/**************************************************************************//**
 * Create a new json object instance.
 *
 * @note    The mandatory fields on the Other must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          Other has immutable properties.
 * @param   yourjson       json string.
 * @returns pointer to the newly manufactured ::EVEL_JSON_OBJECT_INSTANCE.
 *          not used (i.e. posted) it must be released using ::evel_free_jsonobjectinstance.
 * @retval  NULL  Failed to create the json object instance.
 *****************************************************************************/
EVEL_JSON_OBJECT_INSTANCE * evel_new_jsonobjinstance(const char *const yourjson)
{
  EVEL_JSON_OBJECT_INSTANCE *jobjinst = NULL;
  jsmntok_t *key;
  int resultCode;
  jsmn_parser p;
  jsmntok_t tokens[MAX_JSON_TOKENS]; // a number >= total number of tokens
  int len=0;


  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(yourjson != NULL);
  len = strlen(yourjson)+1;
  assert(len > 0);

  /***************************************************************************/
  /*  Validate JSON for json object
  /***************************************************************************/
  jsmn_init(&p);
  resultCode = jsmn_parse(&p, yourjson, len, tokens, sizeof(tokens)/sizeof(tokens[0]));
  if( resultCode < 0 ){
    log_error_state("Failed to parse json for object");
    goto exit_label;
  }

  if (resultCode < 1 || tokens[0].type != JSMN_OBJECT) {
    log_error_state("Error json object expected");
    goto exit_label;
  }

  /***************************************************************************/
  /* Allocate the json object.                                                     */
  /***************************************************************************/
  jobjinst = malloc(sizeof(EVEL_JSON_OBJECT_INSTANCE));
  if (jobjinst == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(jobjinst, 0, sizeof(EVEL_JSON_OBJECT_INSTANCE));

  /***************************************************************************/
  /* Initialize the fields.  Optional key values are   */
  /* uninitialized (NULL).                                                   */
  /***************************************************************************/
  jobjinst->jsonstring = strdup(yourjson);
  dlist_initialize(&jobjinst->object_keys);

exit_label:
  EVEL_EXIT();
  return jobjinst;
}


/**************************************************************************//**
 * Create a new internal key.
 *
 * @note    The mandatory fields on the Other must be supplied to this factory
 *          function and are immutable once set.  Optional fields have explicit
 *          setter functions, but again values may only be set once so that the
 *          Other has immutable properties.
 * @param   keyname       name of the key.
 * @returns pointer to the newly manufactured ::EVEL_INTERNAL_KEY.
 *          not used (i.e. posted) it must be released using ::evel_free_internal_key.
 * @retval  NULL  Failed to create the internal key.
 *****************************************************************************/
EVEL_INTERNAL_KEY * evel_new_internal_key(char *keyname)
{
  EVEL_INTERNAL_KEY *keyinst = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(keyname != NULL);

  /***************************************************************************/
  /* Allocate the key object.                                                     */
  /***************************************************************************/
  keyinst = malloc(sizeof(EVEL_INTERNAL_KEY));
  if (keyinst == NULL)
  {
    log_error_state("Out of memory");
    goto exit_label;
  }
  memset(keyinst, 0, sizeof(EVEL_INTERNAL_KEY));
  keyinst->keyname = strdup(keyname);

  /***************************************************************************/
  /* Optional string values are  uninitialized (NULL).  */
  /***************************************************************************/
  evel_init_option_int(&keyinst->keyorder);
  evel_init_option_string(&keyinst->keyvalue);

exit_label:
  EVEL_EXIT();
  return keyinst;
}

/**************************************************************************//**
 * Set the keyorder  of the internal key instance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param int keyorder
 *****************************************************************************/
void evel_internal_key_keyorder_set(EVEL_INTERNAL_KEY * pinst, const int keyorder)
{
  assert (pinst != NULL);
  evel_set_option_int(&pinst->keyorder,keyorder,"Key order");
}

/**************************************************************************//**
 * Set the keyvalue  of the internal key instance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param string keyvalue
 *****************************************************************************/
void evel_internal_key_keyvalue_set(EVEL_INTERNAL_KEY * pinst, const char * const keyval)
{
  assert (pinst != NULL);
  evel_set_option_string(&pinst->keyvalue,keyval,"Key Value");
}

/**************************************************************************//**
 * Set the string values of json object
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param string object schema
 *****************************************************************************/
void evel_jsonobject_objectschema_set(EVEL_JSON_OBJECT * pinst, const char * const objectschema)
{
  assert (pinst != NULL);
  evel_set_option_string(&pinst->objectschema,objectschema,"Object Schema");
}

/**************************************************************************//**
 * Set the string values of json object
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param string object schema url
 *****************************************************************************/
void evel_jsonobject_objectschemaurl_set(EVEL_JSON_OBJECT * pinst, const char * const objectschemaurl)
{
  assert (pinst != NULL);
  evel_set_option_string(&pinst->objectschemaurl,objectschemaurl,"Object Schema URL");
}

/**************************************************************************//**
 * Set the string values of json object
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param string  NF Subscribed object name
 *****************************************************************************/
void evel_jsonobject_nfsubscribedobjname_set(EVEL_JSON_OBJECT * pinst, const char * const nfsubscribedobjname)
{
  assert (pinst != NULL);
  evel_set_option_string(&pinst->nfsubscribedobjname,nfsubscribedobjname,"NF Subscribed Object Name");
}

/**************************************************************************//**
 * Set the string values of json object
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param string  NF Subscription Id
 *****************************************************************************/
void evel_jsonobject_nfsubscriptionid_set(EVEL_JSON_OBJECT * pinst, const char * const nfsubscriptionid)
{
  assert (pinst != NULL);
  evel_set_option_string(&pinst->nfsubscriptionid,nfsubscriptionid,"NF Subscription Id");
}

/**************************************************************************//**
 * Set the Epoch time of the json object instance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param unsigned long long epoch time
 *****************************************************************************/
void evel_epoch_microsec_set(EVEL_JSON_OBJECT_INSTANCE * pinst, const unsigned long long epmicrosec)
{
  assert(epmicrosec != 0 );
  pinst->objinst_epoch_microsec = epmicrosec;
}

/**************************************************************************//**
 * Add a json object instance to jsonObject list.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param pobj         Pointer to the Other.
 * @param jinst        Pointer to HashTable
 *****************************************************************************/
void evel_jsonobject_add_jsoninstance(EVEL_JSON_OBJECT * pobj, EVEL_JSON_OBJECT_INSTANCE *jinst)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(pobj != NULL);
  assert(jinst != NULL);

  EVEL_DEBUG("Adding json object instance");

  dlist_push_last(&pobj->jsonobjectinstances, jinst);

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
void evel_jsonobjinst_add_objectkey(EVEL_JSON_OBJECT_INSTANCE * jsoninst, EVEL_INTERNAL_KEY *keyp)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(jsoninst != NULL);
  assert(keyp != NULL);

  EVEL_DEBUG("Adding jsonObject instance");

  dlist_push_last(&jsoninst->object_keys, keyp);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free an internal key.
 *
 * Free off the internal key supplied.  Will free all the contained allocated memory.
 *
 *****************************************************************************/
void evel_free_internal_key(EVEL_INTERNAL_KEY * keyp)
{

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(keyp != NULL);

  free(keyp->keyname);
  evel_free_option_string(&keyp->keyvalue);
  EVEL_EXIT();
}


/**************************************************************************//**
 * Free an json object instance.
 *
 * Free off the json object instance supplied.
 *  Will free all the contained allocated memory.
 *
 *****************************************************************************/
void evel_free_jsonobjinst(EVEL_JSON_OBJECT_INSTANCE * objinst)
{
  EVEL_INTERNAL_KEY *other_field = NULL;

  EVEL_ENTER();
  assert(objinst != NULL);
  assert(objinst->jsonstring != NULL);

  free(objinst->jsonstring);

  /***************************************************************************/
  /* Free all internal internal keys
  /***************************************************************************/
  other_field = dlist_pop_last(&objinst->object_keys);
  while (other_field != NULL)
  {
    EVEL_DEBUG("Freeing Object Instance Field (%s)",
               other_field->keyname);
    evel_free_internal_key(other_field);
    other_field = dlist_pop_last(&objinst->object_keys);
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free an json object.
 *
 * Free off the json object instance supplied.
 *  Will free all the contained allocated memory.
 *
 *****************************************************************************/
void evel_free_jsonobject(EVEL_JSON_OBJECT * jsobj)
{
  EVEL_JSON_OBJECT_INSTANCE *other_field = NULL;

  EVEL_ENTER();
  assert(jsobj != NULL);

  free(jsobj->object_name);
  evel_free_option_string(&jsobj->objectschema);
  evel_free_option_string(&jsobj->objectschemaurl);
  evel_free_option_string(&jsobj->nfsubscribedobjname);
  evel_free_option_string(&jsobj->nfsubscriptionid);

  /***************************************************************************/
  /* Free all internal strings then the header itself.                       */
  /***************************************************************************/
  other_field = dlist_pop_last(&jsobj->jsonobjectinstances);
  while (other_field != NULL)
  {
    EVEL_DEBUG("Freeing Object Instance Field (%s)",
               other_field->jsonstring);
    evel_free_jsonobjinst(other_field);
    other_field = dlist_pop_last(&jsobj->jsonobjectinstances);
  }

  EVEL_EXIT();
}
