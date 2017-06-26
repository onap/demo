/**************************************************************************//**
 * @file
 * Wrapper for event logging built on syslog.
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

#include <string.h>
#include <assert.h>
#include <syslog.h>
#include <stdlib.h>
#include <sys/time.h>

#include <curl/curl.h>

#include "evel.h"


/*****************************************************************************/
/* Debug settings.  Logging is done through macros so these need to be       */
/* externally visible.                                                       */
/*****************************************************************************/
EVEL_LOG_LEVELS debug_level = EVEL_LOG_DEBUG;
//static char *syslog_ident = "evel";
int debug_indent = 0;

/*****************************************************************************/
/* Buffers for error strings from this library.                              */
/*****************************************************************************/
static char evel_err_string[EVEL_MAX_ERROR_STRING_LEN] = "<NULL>";


/**************************************************************************//**
 * Initialize logging
 *
 * @param[in] level  The debugging level - one of ::EVEL_LOG_LEVELS.
 * @param[in] ident  The identifier for our logs.
 *****************************************************************************/
void log_initialize(EVEL_LOG_LEVELS level, const char * ident)
{
  assert(level < EVEL_LOG_MAX);
  assert(ident != NULL);

  debug_level = level;
  openlog(ident, LOG_PID, LOG_USER);
}

/**************************************************************************//**
 * Descriptive text for library errors.
 *
 * Return a text error string that relates to the last failure.  May be
 * "<null>" but will never be NULL.
 *
 * @returns   Text error string.
 *
 * @note      Must not be freed!
 *****************************************************************************/
const char * evel_error_string(void)
{
  return(evel_err_string);
}

/***************************************************************************//*
 * Store the formatted string into the static error string and log the error.
 *
 * @param format  Error string in standard printf format.
 * @param ...     Variable parameters to be substituted into the format string.
 *****************************************************************************/
void log_error_state(char * format, ...)
{
  va_list largs;

  assert(format != NULL);
  va_start(largs, format);
  vsnprintf(evel_err_string, EVEL_MAX_ERROR_STRING_LEN, format, largs);
  va_end(largs);
  EVEL_ERROR("%s", evel_err_string);
}


/**************************************************************************//**
 *  Generate a debug log.
 *
 *  Provides an interface to syslog with formatting of the nesting level
 *  so that it's easier to see function entry/exit.
 *
 *  @param[in]  level   The debug level - see ::EVEL_LOG_LEVELS.
 *  @param[in]  format  The output formatting in printf style.
 *  @param[in]  ...     Variable arguments as specified in the format string.
 *****************************************************************************/
void log_debug(EVEL_LOG_LEVELS level, char * format, ...)
{
  va_list largs;
  int priority;
  char indent_fmt[1024];
  char *syslog_fmt = NULL;

  /***************************************************************************/
  /* Test assumptions.                                                       */
  /***************************************************************************/
  assert(format != NULL);
  assert(level <= EVEL_LOG_MAX);

  if (level >= debug_level)
  {
    if ((debug_level == EVEL_LOG_INFO) || (debug_indent == 0))
    {
      /***********************************************************************/
      /* Just use the format as is.                                          */
      /***********************************************************************/
      syslog_fmt = format;
    }
    else
    {
      /***********************************************************************/
      /* Combine the format with a preceding number of indent markers.       */
      /***********************************************************************/
      sprintf(indent_fmt, "%.*s%s",
              debug_indent,
              INDENT_SEPARATORS,
              format);
      syslog_fmt = indent_fmt;
    }

    /*************************************************************************/
    /* Work out the syslog priority value.                                   */
    /*************************************************************************/
    switch (level)
    {
    case EVEL_LOG_ERROR:
      priority = LOG_ERR;
      break;

    case EVEL_LOG_INFO:
      priority = LOG_INFO;
      break;

    case EVEL_LOG_DEBUG:
    case EVEL_LOG_SPAMMY:
    default:
      priority = LOG_DEBUG;
      break;
    }

    /*************************************************************************/
    /* Write the log to the file next, which requires the var args list.     */
    /*************************************************************************/
    va_start(largs, format);
    vsyslog(priority, syslog_fmt, largs);
    va_end(largs);
  }
}
