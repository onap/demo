/**************************************************************************//**
 * @file
 * Source module relating to EVEL_OPTION_ types.
 *
 * License
 * -------
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
 *****************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "evel_internal.h"

/**************************************************************************//**
 * Free the underlying resources of an ::EVEL_OPTION_STRING.
 *
 * @param option        Pointer to the ::EVEL_OPTION_STRING.
 *****************************************************************************/
void evel_free_option_string(EVEL_OPTION_STRING * const option)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);

  if (option->is_set)
  {
    free(option->value);
    option->value = NULL;
    option->is_set = EVEL_FALSE;
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Initialize an ::EVEL_OPTION_STRING to a not-set state.
 *
 * @param option        Pointer to the ::EVEL_OPTION_STRING.
 *****************************************************************************/
void evel_init_option_string(EVEL_OPTION_STRING * const option)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);

  option->value = NULL;
  option->is_set = EVEL_FALSE;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the value of an ::EVEL_OPTION_STRING.
 *
 * @param option        Pointer to the ::EVEL_OPTION_STRING.
 * @param value         The value to set.
 * @param description   Description to be used in logging.
 *****************************************************************************/
void evel_set_option_string(EVEL_OPTION_STRING * const option,
                            const char * const value,
                            const char * const description)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);
  assert(value != NULL);
  assert(description != NULL);

  if (option->is_set)
  {
    EVEL_ERROR("Ignoring attempt to update %s to %s. %s already set to %s",
               description, value, description, option->value);
  }
  else
  {
    EVEL_DEBUG("Setting %s to %s", description, value);
    option->value = strdup(value);
    option->is_set = EVEL_TRUE;
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Force the value of an ::EVEL_OPTION_STRING.
 *
 * @param option        Pointer to the ::EVEL_OPTION_STRING.
 * @param value         The value to set.
 *****************************************************************************/
void evel_force_option_string(EVEL_OPTION_STRING * const option,
                              const char * const value)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);
  assert(option->is_set == EVEL_FALSE);
  assert(option->value == NULL);

  option->value = strdup(value);
  option->is_set = EVEL_TRUE;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Initialize an ::EVEL_OPTION_INT to a not-set state.
 *
 * @param option        Pointer to the ::EVEL_OPTION_INT.
 *****************************************************************************/
void evel_init_option_int(EVEL_OPTION_INT * const option)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);

  option->value = 0;
  option->is_set = EVEL_FALSE;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Force the value of an ::EVEL_OPTION_INT.
 *
 * @param option        Pointer to the ::EVEL_OPTION_INT.
 * @param value         The value to set.
 *****************************************************************************/
void evel_force_option_int(EVEL_OPTION_INT * const option,
                           const int value)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);

  option->value = value;
  option->is_set = EVEL_TRUE;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the value of an ::EVEL_OPTION_INT.
 *
 * @param option        Pointer to the ::EVEL_OPTION_INT.
 * @param value         The value to set.
 * @param description   Description to be used in logging.
 *****************************************************************************/
void evel_set_option_int(EVEL_OPTION_INT * const option,
                         const int value,
                         const char * const description)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);
  assert(description != NULL);

  if (option->is_set)
  {
    EVEL_ERROR("Ignoring attempt to update %s to %d. %s already set to %d",
               description, value, description, option->value);
  }
  else
  {
    EVEL_DEBUG("Setting %s to %d", description, value);
    option->value = value;
    option->is_set = EVEL_TRUE;
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Initialize an ::EVEL_OPTION_DOUBLE to a not-set state.
 *
 * @param option        Pointer to the ::EVEL_OPTION_DOUBLE.
 *****************************************************************************/
void evel_init_option_double(EVEL_OPTION_DOUBLE * const option)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);

  option->value = 0.0;
  option->is_set = EVEL_FALSE;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Force the value of an ::EVEL_OPTION_DOUBLE.
 *
 * @param option        Pointer to the ::EVEL_OPTION_DOUBLE.
 * @param value         The value to set.
 *****************************************************************************/
void evel_force_option_double(EVEL_OPTION_DOUBLE * const option,
                              const double value)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);

  option->value = value;
  option->is_set = EVEL_TRUE;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the value of an ::EVEL_OPTION_DOUBLE.
 *
 * @param option        Pointer to the ::EVEL_OPTION_DOUBLE.
 * @param value         The value to set.
 * @param description   Description to be used in logging.
 *****************************************************************************/
void evel_set_option_double(EVEL_OPTION_DOUBLE * const option,
                            const double value,
                            const char * const description)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);
  assert(description != NULL);

  if (option->is_set)
  {
    EVEL_ERROR("Ignoring attempt to update %s to %lf. %s already set to %lf",
               description, value, description, option->value);
  }
  else
  {
    EVEL_DEBUG("Setting %s to %lf", description, value);
    option->value = value;
    option->is_set = EVEL_TRUE;
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Initialize an ::EVEL_OPTION_ULL to a not-set state.
 *
 * @param option        Pointer to the ::EVEL_OPTION_ULL.
 *****************************************************************************/
void evel_init_option_ull(EVEL_OPTION_ULL * const option)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);
  option->value = 0;
  option->is_set = EVEL_FALSE;
  EVEL_EXIT();
}

/**************************************************************************//**
 * Force the value of an ::EVEL_OPTION_ULL.
 *
 * @param option        Pointer to the ::EVEL_OPTION_ULL.
 * @param value         The value to set.
 *****************************************************************************/
void evel_force_option_ull(EVEL_OPTION_ULL * const option,
                           const unsigned long long value)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);

  option->value = value;
  option->is_set = EVEL_TRUE;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the value of an ::EVEL_OPTION_ULL.
 *
 * @param option        Pointer to the ::EVEL_OPTION_ULL.
 * @param value         The value to set.
 * @param description   Description to be used in logging.
 *****************************************************************************/
void evel_set_option_ull(EVEL_OPTION_ULL * const option,
                         const unsigned long long value,
                         const char * const description)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);
  assert(description != NULL);

  if (option->is_set)
  {
    EVEL_ERROR("Ignoring attempt to update %s to %llu. %s already set to %llu",
               description, value, description, option->value);
  }
  else
  {
    EVEL_DEBUG("Setting %s to %llu", description, value);
    option->value = value;
    option->is_set = EVEL_TRUE;
  }
  EVEL_EXIT();
}

/**************************************************************************//**
 * Initialize an ::EVEL_OPTION_TIME to a not-set state.
 *
 * @param option        Pointer to the ::EVEL_OPTION_TIME.
 *****************************************************************************/
void evel_init_option_time(EVEL_OPTION_TIME * const option)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);
  option->value = 0;
  option->is_set = EVEL_FALSE;
  EVEL_EXIT();
}

/**************************************************************************//**
 * Force the value of an ::EVEL_OPTION_TIME.
 *
 * @param option        Pointer to the ::EVEL_OPTION_TIME.
 * @param value         The value to set.
 *****************************************************************************/
void evel_force_option_time(EVEL_OPTION_TIME * const option,
                            const time_t value)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);

  option->value = value;
  option->is_set = EVEL_TRUE;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the value of an ::EVEL_OPTION_TIME.
 *
 * @param option        Pointer to the ::EVEL_OPTION_TIME.
 * @param value         The value to set.
 * @param description   Description to be used in logging.
 *****************************************************************************/
void evel_set_option_time(EVEL_OPTION_TIME * const option,
                          const time_t value,
                          const char * const description)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);
  assert(description != NULL);

  if (option->is_set)
  {
    EVEL_ERROR("Ignoring attempt to update %s to %d. %s already set to %d",
               description, value, description, option->value);
  }
  else
  {
    EVEL_DEBUG("Setting %s to %d", description, value);
    option->value = value;
    option->is_set = EVEL_TRUE;
  }
  EVEL_EXIT();
}
