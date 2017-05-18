/**************************************************************************//**
 * @file
 * Source module relating to EVEL_OPTION_ types.
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
 * Initialize an ::EVEL_OPTION_INTHEADER_FIELDS to a not-set state.
 *
 * @param option        Pointer to the ::EVEL_OPTION_INTHEADER_FIELDS.
 *****************************************************************************/
void evel_init_option_intheader(EVEL_OPTION_INTHEADER_FIELDS * const option)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);
  option->object = NULL;
  option->is_set = EVEL_FALSE;
  EVEL_EXIT();
}

/**************************************************************************//**
 * Force the value of an ::EVEL_OPTION_INTHEADER_FIELDS.
 *
 * @param option        Pointer to the ::EVEL_OPTION_INTHEADER_FIELDS.
 * @param value         The value to set.
 *****************************************************************************/
void evel_force_option_intheader(EVEL_OPTION_INTHEADER_FIELDS * const option,
                           const void* value)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);

  option->object = value;
  option->is_set = EVEL_TRUE;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the value of an ::EVEL_OPTION_INTHEADER_FIELDS.
 *
 * @param option        Pointer to the ::EVEL_OPTION_INTHEADER_FIELDS.
 * @param value         The value to set.
 * @param description   Description to be used in logging.
 *****************************************************************************/
void evel_set_option_intheader(EVEL_OPTION_INTHEADER_FIELDS * const option,
                         const void * value,
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
               description, value, description, option->object);
  }
  else
  {
    EVEL_DEBUG("Setting %s to %llu", description, value);
    option->object = value;
    option->is_set = EVEL_TRUE;
  }
  EVEL_EXIT();
}

/**************************************************************************//**
 * Free the underlying resources of an ::EVEL_OPTION_INTHEADER_FIELDS.
 *
 * @param option        Pointer to the ::EVEL_OPTION_INTHEADER_FIELDS.
 *****************************************************************************/
void evel_free_option_intheader(EVEL_OPTION_INTHEADER_FIELDS * const option)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(option != NULL);

  if (option->is_set)
  {
    free(option->object);
    option->object = NULL;
    option->is_set = EVEL_FALSE;
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
