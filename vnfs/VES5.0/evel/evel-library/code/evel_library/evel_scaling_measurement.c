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
 * Implementation of EVEL functions relating to the Measurement.
 *
 ****************************************************************************/

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "evel.h"
#include "evel_internal.h"
#include "evel_throttle.h"

/**************************************************************************//**
 * Create a new Measurement event.
 *
 * @note    The mandatory fields on the Measurement must be supplied to this
 *          factory function and are immutable once set.  Optional fields have
 *          explicit setter functions, but again values may only be set once so
 *          that the Measurement has immutable properties.
 *
 * @param   measurement_interval
 * @param event_name  Unique Event Name confirming Domain AsdcModel Description
 * @param event_id    A universal identifier of the event for: troubleshooting correlation, analysis, etc
 *
 * @returns pointer to the newly manufactured ::EVENT_MEASUREMENT.  If the
 *          event is not used (i.e. posted) it must be released using
 *          ::evel_free_event.
 * @retval  NULL  Failed to create the event.
 *****************************************************************************/
EVENT_MEASUREMENT * evel_new_measurement(double measurement_interval, const char* ev_name, const char *ev_id)
{
  EVENT_MEASUREMENT * measurement = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement_interval >= 0.0);

  /***************************************************************************/
  /* Allocate the measurement.                                               */
  /***************************************************************************/
  measurement = malloc(sizeof(EVENT_MEASUREMENT));
  if (measurement == NULL)
  {
    log_error_state("Out of memory for Measurement");
    goto exit_label;
  }
  memset(measurement, 0, sizeof(EVENT_MEASUREMENT));
  EVEL_DEBUG("New measurement is at %lp", measurement);

  /***************************************************************************/
  /* Initialize the header & the measurement fields.                         */
  /***************************************************************************/
  evel_init_header_nameid(&measurement->header,ev_name,ev_id);
  measurement->header.event_domain = EVEL_DOMAIN_MEASUREMENT;
  measurement->measurement_interval = measurement_interval;
  dlist_initialize(&measurement->additional_info);
  dlist_initialize(&measurement->additional_measurements);
  dlist_initialize(&measurement->additional_objects);
  dlist_initialize(&measurement->cpu_usage);
  dlist_initialize(&measurement->disk_usage);
  dlist_initialize(&measurement->mem_usage);
  dlist_initialize(&measurement->filesystem_usage);
  dlist_initialize(&measurement->latency_distribution);
  dlist_initialize(&measurement->vnic_usage);
  dlist_initialize(&measurement->codec_usage);
  dlist_initialize(&measurement->feature_usage);
  evel_init_option_double(&measurement->mean_request_latency);
  evel_init_option_int(&measurement->vnfc_scaling_metric);
  evel_init_option_int(&measurement->concurrent_sessions);
  evel_init_option_int(&measurement->configured_entities);
  evel_init_option_int(&measurement->media_ports_in_use);
  evel_init_option_int(&measurement->request_rate);
  measurement->major_version = EVEL_MEASUREMENT_MAJOR_VERSION;
  measurement->minor_version = EVEL_MEASUREMENT_MINOR_VERSION;

exit_label:
  EVEL_EXIT();
  return measurement;
}

/**************************************************************************//**
 * Set the Event Type property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement Pointer to the Measurement.
 * @param type        The Event Type to be set. ASCIIZ string. The caller
 *                    does not need to preserve the value once the function
 *                    returns.
 *****************************************************************************/
void evel_measurement_type_set(EVENT_MEASUREMENT * measurement,
                               const char * const type)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions and call evel_header_type_set.                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  evel_header_type_set(&measurement->header, type);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional value name/value pair to the Measurement.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement     Pointer to the measurement.
 * @param name      ASCIIZ string with the attribute's name.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 * @param value     ASCIIZ string with the attribute's value.  The caller
 *                  does not need to preserve the value once the function
 *                  returns.
 *****************************************************************************/
void evel_measurement_addl_info_add(EVENT_MEASUREMENT * measurement, char * name, char * value)
{
  OTHER_FIELD * addl_info = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(name != NULL);
  assert(value != NULL);
  
  EVEL_DEBUG("Adding name=%s value=%s", name, value);
  addl_info = malloc(sizeof(OTHER_FIELD));
  assert(addl_info != NULL);
  memset(addl_info, 0, sizeof(OTHER_FIELD));
  addl_info->name = strdup(name);
  addl_info->value = strdup(value);
  assert(addl_info->name != NULL);
  assert(addl_info->value != NULL);

  dlist_push_last(&measurement->additional_info, addl_info);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Concurrent Sessions property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement         Pointer to the Measurement.
 * @param concurrent_sessions The Concurrent Sessions to be set.
 *****************************************************************************/
void evel_measurement_conc_sess_set(EVENT_MEASUREMENT * measurement,
                                    int concurrent_sessions)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(concurrent_sessions >= 0);

  evel_set_option_int(&measurement->concurrent_sessions,
                      concurrent_sessions,
                      "Concurrent Sessions");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Configured Entities property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement         Pointer to the Measurement.
 * @param configured_entities The Configured Entities to be set.
 *****************************************************************************/
void evel_measurement_cfg_ents_set(EVENT_MEASUREMENT * measurement,
                                   int configured_entities)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(configured_entities >= 0);

  evel_set_option_int(&measurement->configured_entities,
                      configured_entities,
                      "Configured Entities");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional set of Errors to the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement       Pointer to the measurement.
 * @param receive_discards  The number of receive discards.
 * @param receive_errors    The number of receive errors.
 * @param transmit_discards The number of transmit discards.
 * @param transmit_errors   The number of transmit errors.
 *****************************************************************************/
void evel_measurement_errors_set(EVENT_MEASUREMENT * measurement,
                                 int receive_discards,
                                 int receive_errors,
                                 int transmit_discards,
                                 int transmit_errors)
{
  MEASUREMENT_ERRORS * errors = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(receive_discards >= 0);
  assert(receive_errors >= 0);
  assert(transmit_discards >= 0);
  assert(transmit_errors >= 0);

  if (measurement->errors == NULL)
  {
    EVEL_DEBUG("Adding Errors: %d, %d; %d, %d",
               receive_discards,
               receive_errors,
               transmit_discards,
               transmit_errors);
    errors = malloc(sizeof(MEASUREMENT_ERRORS));
    assert(errors != NULL);
    memset(errors, 0, sizeof(MEASUREMENT_ERRORS));
    errors->receive_discards = receive_discards;
    errors->receive_errors = receive_errors;
    errors->transmit_discards = transmit_discards;
    errors->transmit_errors = transmit_errors;
    measurement->errors = errors;
  }
  else
  {
    errors = measurement->errors;
    EVEL_DEBUG("Ignoring attempt to add Errors: %d, %d; %d, %d\n"
               "Errors already set: %d, %d; %d, %d",
               receive_discards,
               receive_errors,
               transmit_discards,
               transmit_errors,
               errors->receive_discards,
               errors->receive_errors,
               errors->transmit_discards,
               errors->transmit_errors);
  }

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Mean Request Latency property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement          Pointer to the Measurement.
 * @param mean_request_latency The Mean Request Latency to be set.
 *****************************************************************************/
void evel_measurement_mean_req_lat_set(EVENT_MEASUREMENT * measurement,
                                       double mean_request_latency)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(mean_request_latency >= 0.0);

  evel_set_option_double(&measurement->mean_request_latency,
                         mean_request_latency,
                         "Mean Request Latency");
  EVEL_EXIT();
}


/**************************************************************************//**
 * Set the Request Rate property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement  Pointer to the Measurement.
 * @param request_rate The Request Rate to be set.
 *****************************************************************************/
void evel_measurement_request_rate_set(EVENT_MEASUREMENT * measurement,
                                       int request_rate)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(request_rate >= 0);

  evel_set_option_int(&measurement->request_rate,
                      request_rate,
                      "Request Rate");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional CPU usage value name/value pair to the Measurement.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement   Pointer to the measurement.
 * @param id            ASCIIZ string with the CPU's identifier.
 * @param usage         CPU utilization.
 *****************************************************************************/
MEASUREMENT_CPU_USE *evel_measurement_new_cpu_use_add(EVENT_MEASUREMENT * measurement,
                                 char * id, double usage)
{
  MEASUREMENT_CPU_USE * cpu_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(id != NULL);
  assert(usage >= 0.0);

  /***************************************************************************/
  /* Allocate a container for the value and push onto the list.              */
  /***************************************************************************/
  EVEL_DEBUG("Adding id=%s usage=%lf", id, usage);
  cpu_use = malloc(sizeof(MEASUREMENT_CPU_USE));
  assert(cpu_use != NULL);
  memset(cpu_use, 0, sizeof(MEASUREMENT_CPU_USE));
  cpu_use->id    = strdup(id);
  cpu_use->usage = usage;
  evel_init_option_double(&cpu_use->idle);
  evel_init_option_double(&cpu_use->intrpt);
  evel_init_option_double(&cpu_use->nice);
  evel_init_option_double(&cpu_use->softirq);
  evel_init_option_double(&cpu_use->steal);
  evel_init_option_double(&cpu_use->sys);
  evel_init_option_double(&cpu_use->user);
  evel_init_option_double(&cpu_use->wait);

  dlist_push_last(&measurement->cpu_usage, cpu_use);

  EVEL_EXIT();
  return cpu_use;
}

/**************************************************************************//**
 * Set the CPU Idle value in measurement interval
 *   percentage of CPU time spent in the idle task
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param cpu_use      Pointer to the CPU Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_cpu_use_idle_set(MEASUREMENT_CPU_USE *const cpu_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&cpu_use->idle, val, "CPU idle time");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the percentage of time spent servicing interrupts
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param cpu_use      Pointer to the CPU Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_cpu_use_interrupt_set(MEASUREMENT_CPU_USE * const cpu_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&cpu_use->intrpt, val, "CPU interrupt value");
  EVEL_EXIT();
}


/**************************************************************************//**
 * Set the percentage of time spent running user space processes that have been niced
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param cpu_use      Pointer to the CPU Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_cpu_use_nice_set(MEASUREMENT_CPU_USE * const cpu_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&cpu_use->nice, val, "CPU nice value");
  EVEL_EXIT();
}


/**************************************************************************//**
 * Set the percentage of time spent handling soft irq interrupts
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param cpu_use      Pointer to the CPU Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_cpu_use_softirq_set(MEASUREMENT_CPU_USE * const cpu_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&cpu_use->softirq, val, "CPU Soft IRQ value");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the percentage of time spent in involuntary wait
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param cpu_use      Pointer to the CPU Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_cpu_use_steal_set(MEASUREMENT_CPU_USE * const cpu_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&cpu_use->steal, val, "CPU involuntary wait");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the percentage of time spent on system tasks running the kernel
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param cpu_use      Pointer to the CPU Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_cpu_use_system_set(MEASUREMENT_CPU_USE * const cpu_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&cpu_use->sys, val, "CPU System load");
  EVEL_EXIT();
}


/**************************************************************************//**
 * Set the percentage of time spent running un-niced user space processes
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param cpu_use      Pointer to the CPU Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_cpu_use_usageuser_set(MEASUREMENT_CPU_USE * const cpu_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&cpu_use->user, val, "CPU User load value");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the percentage of CPU time spent waiting for I/O operations to complete
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param cpu_use      Pointer to the CPU Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_cpu_use_wait_set(MEASUREMENT_CPU_USE * const cpu_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&cpu_use->wait, val, "CPU Wait IO value");
  EVEL_EXIT();
}


/**************************************************************************//**
 * Add an additional Memory usage value name/value pair to the Measurement.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement   Pointer to the measurement.
 * @param id            ASCIIZ string with the Memory identifier.
 * @param vmidentifier  ASCIIZ string with the VM's identifier.
 * @param membuffsz     Memory Size.
 *
 * @return  Returns pointer to memory use structure in measurements
 *****************************************************************************/
MEASUREMENT_MEM_USE * evel_measurement_new_mem_use_add(EVENT_MEASUREMENT * measurement,
                                 char * id,  char *vmidentifier,  double membuffsz)
{
  MEASUREMENT_MEM_USE * mem_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(id != NULL);
  assert(membuffsz >= 0.0);

  /***************************************************************************/
  /* Allocate a container for the value and push onto the list.              */
  /***************************************************************************/
  EVEL_DEBUG("Adding id=%s buffer size=%lf", id, membuffsz);
  mem_use = malloc(sizeof(MEASUREMENT_MEM_USE));
  assert(mem_use != NULL);
  memset(mem_use, 0, sizeof(MEASUREMENT_MEM_USE));
  mem_use->id    = strdup(id);
  mem_use->vmid  = strdup(vmidentifier);
  mem_use->membuffsz = membuffsz;
  evel_init_option_double(&mem_use->memcache);
  evel_init_option_double(&mem_use->memconfig);
  evel_init_option_double(&mem_use->memfree);
  evel_init_option_double(&mem_use->slabrecl);
  evel_init_option_double(&mem_use->slabunrecl);
  evel_init_option_double(&mem_use->memused);

  assert(mem_use->id != NULL);

  dlist_push_last(&measurement->mem_usage, mem_use);

  EVEL_EXIT();
  return mem_use;
}

/**************************************************************************//**
 * Set kilobytes of memory used for cache
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mem_use      Pointer to the Memory Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_mem_use_memcache_set(MEASUREMENT_MEM_USE * const mem_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&mem_use->memcache, val, "Memory cache value");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set kilobytes of memory configured in the virtual machine on which the VNFC reporting
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mem_use      Pointer to the Memory Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_mem_use_memconfig_set(MEASUREMENT_MEM_USE * const mem_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&mem_use->memconfig, val, "Memory configured value");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set kilobytes of physical RAM left unused by the system
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mem_use      Pointer to the Memory Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_mem_use_memfree_set(MEASUREMENT_MEM_USE * const mem_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&mem_use->memfree, val, "Memory freely available value");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the part of the slab that can be reclaimed such as caches measured in kilobytes
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mem_use      Pointer to the Memory Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_mem_use_slab_reclaimed_set(MEASUREMENT_MEM_USE * const mem_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&mem_use->slabrecl, val, "Memory reclaimable slab set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the part of the slab that cannot be reclaimed such as caches measured in kilobytes
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mem_use      Pointer to the Memory Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_mem_use_slab_unreclaimable_set(MEASUREMENT_MEM_USE * const mem_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&mem_use->slabunrecl, val, "Memory unreclaimable slab set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the total memory minus the sum of free, buffered, cached and slab memory in kilobytes
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param mem_use      Pointer to the Memory Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_mem_use_usedup_set(MEASUREMENT_MEM_USE * const mem_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&mem_use->memused, val, "Memory usedup total set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional Disk usage value name/value pair to the Measurement.
 *
 * The name and value are null delimited ASCII strings.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement   Pointer to the measurement.
 * @param id            ASCIIZ string with the CPU's identifier.
 * @param usage         Disk utilization.
 *****************************************************************************/
MEASUREMENT_DISK_USE * evel_measurement_new_disk_use_add(EVENT_MEASUREMENT * measurement, char * id)
{
  MEASUREMENT_DISK_USE * disk_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(id != NULL);

  /***************************************************************************/
  /* Allocate a container for the value and push onto the list.              */
  /***************************************************************************/
  EVEL_DEBUG("Adding id=%s disk usage", id);
  disk_use = malloc(sizeof(MEASUREMENT_DISK_USE));
  assert(disk_use != NULL);
  memset(disk_use, 0, sizeof(MEASUREMENT_DISK_USE));
  disk_use->id    = strdup(id);
  assert(disk_use->id != NULL);
  dlist_push_last(&measurement->disk_usage, disk_use);

  evel_init_option_double(&disk_use->iotimeavg );
  evel_init_option_double(&disk_use->iotimelast );
  evel_init_option_double(&disk_use->iotimemax );
  evel_init_option_double(&disk_use->iotimemin );
  evel_init_option_double(&disk_use->mergereadavg );
  evel_init_option_double(&disk_use->mergereadlast );
  evel_init_option_double(&disk_use->mergereadmax );
  evel_init_option_double(&disk_use->mergereadmin );
  evel_init_option_double(&disk_use->mergewriteavg );
  evel_init_option_double(&disk_use->mergewritelast );
  evel_init_option_double(&disk_use->mergewritemax );
  evel_init_option_double(&disk_use->mergewritemin );
  evel_init_option_double(&disk_use->octetsreadavg );
  evel_init_option_double(&disk_use->octetsreadlast );
  evel_init_option_double(&disk_use->octetsreadmax );
  evel_init_option_double(&disk_use->octetsreadmin );
  evel_init_option_double(&disk_use->octetswriteavg );
  evel_init_option_double(&disk_use->octetswritelast );
  evel_init_option_double(&disk_use->octetswritemax );
  evel_init_option_double(&disk_use->octetswritemin );
  evel_init_option_double(&disk_use->opsreadavg );
  evel_init_option_double(&disk_use->opsreadlast );
  evel_init_option_double(&disk_use->opsreadmax );
  evel_init_option_double(&disk_use->opsreadmin );
  evel_init_option_double(&disk_use->opswriteavg );
  evel_init_option_double(&disk_use->opswritelast );
  evel_init_option_double(&disk_use->opswritemax );
  evel_init_option_double(&disk_use->opswritemin );
  evel_init_option_double(&disk_use->pendingopsavg );
  evel_init_option_double(&disk_use->pendingopslast );
  evel_init_option_double(&disk_use->pendingopsmax );
  evel_init_option_double(&disk_use->pendingopsmin );
  evel_init_option_double(&disk_use->timereadavg );
  evel_init_option_double(&disk_use->timereadlast );
  evel_init_option_double(&disk_use->timereadmax );
  evel_init_option_double(&disk_use->timereadmin );
  evel_init_option_double(&disk_use->timewriteavg );
  evel_init_option_double(&disk_use->timewritelast );
  evel_init_option_double(&disk_use->timewritemax );
  evel_init_option_double(&disk_use->timewritemin );

  EVEL_EXIT();
  return disk_use;
}

/**************************************************************************//**
 * Set milliseconds spent doing input/output operations over 1 sec; treat
 * this metric as a device load percentage where 1000ms  matches 100% load;
 * provide the average over the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_iotimeavg_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val) 
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->iotimeavg, val, "Disk ioload set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set milliseconds spent doing input/output operations over 1 sec; treat
 * this metric as a device load percentage where 1000ms  matches 100% load;
 * provide the last value within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_iotimelast_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->iotimelast, val, "Disk ioloadlast set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set milliseconds spent doing input/output operations over 1 sec; treat
 * this metric as a device load percentage where 1000ms  matches 100% load;
 * provide the maximum value within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_iotimemax_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->iotimemax, val, "Disk ioloadmax set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set milliseconds spent doing input/output operations over 1 sec; treat
 * this metric as a device load percentage where 1000ms  matches 100% load;
 * provide the minimum value within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_iotimemin_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->iotimemin, val, "Disk ioloadmin set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set number of logical read operations that were merged into physical read
 * operations, e.g., two logical reads were served by one physical disk access;
 * provide the average measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_mergereadavg_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->mergereadavg, val, "Disk Merged read average set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set number of logical read operations that were merged into physical read
 * operations, e.g., two logical reads were served by one physical disk access;
 * provide the last measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_mergereadlast_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->mergereadlast, val, "Disk mergedload last set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set number of logical read operations that were merged into physical read
 * operations, e.g., two logical reads were served by one physical disk access;
 * provide the maximum measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_mergereadmax_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->mergereadmax, val, "Disk merged loadmax set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set number of logical read operations that were merged into physical read
 * operations, e.g., two logical reads were served by one physical disk access;
 * provide the minimum measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_mergereadmin_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->mergereadmin, val, "Disk merged loadmin set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set number of logical write operations that were merged into physical read
 * operations, e.g., two logical writes were served by one physical disk access;
 * provide the last measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_mergewritelast_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->mergewritelast, val, "Disk merged writelast set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set number of logical write operations that were merged into physical read
 * operations, e.g., two logical writes were served by one physical disk access;
 * provide the maximum measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_mergewritemax_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->mergewritemax, val, "Disk writemax set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set number of logical write operations that were merged into physical read
 * operations, e.g., two logical writes were served by one physical disk access;
 * provide the maximum measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_mergewritemin_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->mergewritemin, val, "Disk writemin set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set number of octets per second read from a disk or partition;
 * provide the average measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_octetsreadavg_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->octetsreadavg, val, "Octets readavg set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set number of octets per second read from a disk or partition;
 * provide the last measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_octetsreadlast_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->octetsreadlast, val, "Octets readlast set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set number of octets per second read from a disk or partition;
 * provide the maximum measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_octetsreadmax_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->octetsreadmax, val, "Octets readmax set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set number of octets per second read from a disk or partition;
 * provide the minimum measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_octetsreadmin_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->octetsreadmin, val, "Octets readmin set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set number of octets per second written to a disk or partition;
 * provide the average measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_octetswriteavg_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->octetswriteavg, val, "Octets writeavg set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set number of octets per second written to a disk or partition;
 * provide the last measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_octetswritelast_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->octetswritelast, val, "Octets writelast set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set number of octets per second written to a disk or partition;
 * provide the maximum measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_octetswritemax_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->octetswritemax, val, "Octets writemax set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set number of octets per second written to a disk or partition;
 * provide the minimum measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_octetswritemin_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->octetswritemin, val, "Octets writemin set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set number of read operations per second issued to the disk;
 * provide the average measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_opsreadavg_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->opsreadavg, val, "Disk read operation average set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set number of read operations per second issued to the disk;
 * provide the last measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_opsreadlast_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->opsreadlast, val, "Disk read operation last set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set number of read operations per second issued to the disk;
 * provide the maximum measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_opsreadmax_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->opsreadmax, val, "Disk read operation maximum set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set number of read operations per second issued to the disk;
 * provide the minimum measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_opsreadmin_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->opsreadmin, val, "Disk read operation minimum set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set number of write operations per second issued to the disk;
 * provide the average measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_opswriteavg_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->opswriteavg, val, "Disk write operation average set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set number of write operations per second issued to the disk;
 * provide the last measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_opswritelast_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->opswritelast, val, "Disk write operation last set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set number of write operations per second issued to the disk;
 * provide the maximum measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_opswritemax_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->opswritemax, val, "Disk write operation maximum set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set number of write operations per second issued to the disk;
 * provide the average measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_opswritemin_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->opswritemin, val, "Disk write operation minimum set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set queue size of pending I/O operations per second;
 * provide the average measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_pendingopsavg_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->pendingopsavg, val, "Disk pending operation average set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set queue size of pending I/O operations per second;
 * provide the last measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_pendingopslast_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->pendingopslast, val, "Disk pending operation last set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set queue size of pending I/O operations per second;
 * provide the maximum measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_pendingopsmax_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->pendingopsmax, val, "Disk pending operation maximum set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set queue size of pending I/O operations per second;
 * provide the minimum measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_pendingopsmin_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->pendingopsmin, val, "Disk pending operation min set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set milliseconds a read operation took to complete;
 * provide the average measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_timereadavg_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->timereadavg, val, "Disk read time average set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set milliseconds a read operation took to complete;
 * provide the last measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_timereadlast_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->timereadlast, val, "Disk read time last set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set milliseconds a read operation took to complete;
 * provide the maximum measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_timereadmax_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->timereadmax, val, "Disk read time maximum set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set milliseconds a read operation took to complete;
 * provide the minimum measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_timereadmin_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->timereadmin, val, "Disk read time minimum set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set milliseconds a write operation took to complete;
 * provide the average measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_timewriteavg_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->timewriteavg, val, "Disk write time average set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set milliseconds a write operation took to complete;
 * provide the last measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_timewritelast_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->timewritelast, val, "Disk write time last set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set milliseconds a write operation took to complete;
 * provide the maximum measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_timewritemax_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->timewritemax, val, "Disk write time max set");
  EVEL_EXIT();
}
/**************************************************************************//**
 * Set milliseconds a write operation took to complete;
 * provide the average measurement within the measurement interval
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param disk_use     Pointer to the Disk Use.
 * @param val          double
 *****************************************************************************/
void evel_measurement_disk_use_timewritemin_set(MEASUREMENT_DISK_USE * const disk_use,
                                    const double val)
{
  EVEL_ENTER();
  evel_set_option_double(&disk_use->timewritemin, val, "Disk write time min set");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional File System usage value name/value pair to the
 * Measurement.
 *
 * The filesystem_name is null delimited ASCII string.  The library takes a
 * copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement     Pointer to the measurement.
 * @param filesystem_name   ASCIIZ string with the file-system's UUID.
 * @param block_configured  Block storage configured.
 * @param block_used        Block storage in use.
 * @param block_iops        Block storage IOPS.
 * @param ephemeral_configured  Ephemeral storage configured.
 * @param ephemeral_used        Ephemeral storage in use.
 * @param ephemeral_iops        Ephemeral storage IOPS.
 *****************************************************************************/
void evel_measurement_fsys_use_add(EVENT_MEASUREMENT * measurement,
                                   char * filesystem_name,
                                   double block_configured,
                                   double block_used,
                                   double block_iops,
                                   double ephemeral_configured,
                                   double ephemeral_used,
                                   double ephemeral_iops)
{
  MEASUREMENT_FSYS_USE * fsys_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(filesystem_name != NULL);
  assert(block_configured >= 0.0);
  assert(block_used >= 0.0);
  assert(block_iops >= 0.0);
  assert(ephemeral_configured >= 0.0);
  assert(ephemeral_used >= 0.0);
  assert(ephemeral_iops >= 0.0);

  /***************************************************************************/
  /* Allocate a container for the value and push onto the list.              */
  /***************************************************************************/
  EVEL_DEBUG("Adding filesystem_name=%s", filesystem_name);
  fsys_use = malloc(sizeof(MEASUREMENT_FSYS_USE));
  assert(fsys_use != NULL);
  memset(fsys_use, 0, sizeof(MEASUREMENT_FSYS_USE));
  fsys_use->filesystem_name = strdup(filesystem_name);
  fsys_use->block_configured = block_configured;
  fsys_use->block_used = block_used;
  fsys_use->block_iops = block_iops;
  fsys_use->ephemeral_configured = ephemeral_configured;
  fsys_use->ephemeral_used = ephemeral_used;
  fsys_use->ephemeral_iops = ephemeral_iops;

  dlist_push_last(&measurement->filesystem_usage, fsys_use);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a Feature usage value name/value pair to the Measurement.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement     Pointer to the measurement.
 * @param feature         ASCIIZ string with the feature's name.
 * @param utilization     Utilization of the feature.
 *****************************************************************************/
void evel_measurement_feature_use_add(EVENT_MEASUREMENT * measurement,
                                      char * feature,
                                      int utilization)
{
  MEASUREMENT_FEATURE_USE * feature_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(feature != NULL);
  assert(utilization >= 0);

  /***************************************************************************/
  /* Allocate a container for the value and push onto the list.              */
  /***************************************************************************/
  EVEL_DEBUG("Adding Feature=%s Use=%d", feature, utilization);
  feature_use = malloc(sizeof(MEASUREMENT_FEATURE_USE));
  assert(feature_use != NULL);
  memset(feature_use, 0, sizeof(MEASUREMENT_FEATURE_USE));
  feature_use->feature_id = strdup(feature);
  assert(feature_use->feature_id != NULL);
  feature_use->feature_utilization = utilization;

  dlist_push_last(&measurement->feature_usage, feature_use);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a Additional Measurement value name/value pair to the Report.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement   Pointer to the Measaurement.
 * @param group    ASCIIZ string with the measurement group's name.
 * @param name     ASCIIZ string containing the measurement's name.
 * @param value    ASCIIZ string containing the measurement's value.
 *****************************************************************************/
void evel_measurement_custom_measurement_add(EVENT_MEASUREMENT * measurement,
                                             const char * const group,
                                             const char * const name,
                                             const char * const value)
{
  MEASUREMENT_GROUP * measurement_group = NULL;
  CUSTOM_MEASUREMENT * custom_measurement = NULL;
  DLIST_ITEM * item = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(group != NULL);
  assert(name != NULL);
  assert(value != NULL);

  /***************************************************************************/
  /* Allocate a container for the name/value pair.                           */
  /***************************************************************************/
  EVEL_DEBUG("Adding Measurement Group=%s Name=%s Value=%s",
              group, name, value);
  custom_measurement = malloc(sizeof(CUSTOM_MEASUREMENT));
  assert(custom_measurement != NULL);
  memset(custom_measurement, 0, sizeof(CUSTOM_MEASUREMENT));
  custom_measurement->name = strdup(name);
  assert(custom_measurement->name != NULL);
  custom_measurement->value = strdup(value);
  assert(custom_measurement->value != NULL);

  /***************************************************************************/
  /* See if we have that group already.                                      */
  /***************************************************************************/
  item = dlist_get_first(&measurement->additional_measurements);
  while (item != NULL)
  {
    measurement_group = (MEASUREMENT_GROUP *) item->item;
    assert(measurement_group != NULL);

    EVEL_DEBUG("Got measurement group %s", measurement_group->name);
    if (strcmp(group, measurement_group->name) == 0)
    {
      EVEL_DEBUG("Found existing Measurement Group");
      break;
    }
    item = dlist_get_next(item);
  }

  /***************************************************************************/
  /* If we didn't have the group already, create it.                         */
  /***************************************************************************/
  if (item == NULL)
  {
    EVEL_DEBUG("Creating new Measurement Group");
    measurement_group = malloc(sizeof(MEASUREMENT_GROUP));
    assert(measurement_group != NULL);
    memset(measurement_group, 0, sizeof(MEASUREMENT_GROUP));
    measurement_group->name = strdup(group);
    assert(measurement_group->name != NULL);
    dlist_initialize(&measurement_group->measurements);
    dlist_push_last(&measurement->additional_measurements, measurement_group);
  }

  /***************************************************************************/
  /* If we didn't have the group already, create it.                         */
  /***************************************************************************/
  dlist_push_last(&measurement_group->measurements, custom_measurement);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add a Codec usage value name/value pair to the Measurement.
 *
 * The name is null delimited ASCII string.  The library takes
 * a copy so the caller does not have to preserve values after the function
 * returns.
 *
 * @param measurement     Pointer to the measurement.
 * @param codec           ASCIIZ string with the codec's name.
 * @param utilization     Number of codecs in use.
 *****************************************************************************/
void evel_measurement_codec_use_add(EVENT_MEASUREMENT * measurement,
                                    char * codec,
                                    int utilization)
{
  MEASUREMENT_CODEC_USE * codec_use = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Check assumptions.                                                      */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(codec != NULL);
  assert(utilization >= 0.0);

  /***************************************************************************/
  /* Allocate a container for the value and push onto the list.              */
  /***************************************************************************/
  EVEL_DEBUG("Adding Codec=%s Use=%d", codec, utilization);
  codec_use = malloc(sizeof(MEASUREMENT_CODEC_USE));
  assert(codec_use != NULL);
  memset(codec_use, 0, sizeof(MEASUREMENT_CODEC_USE));
  codec_use->codec_id = strdup(codec);
  assert(codec_use->codec_id != NULL);
  codec_use->number_in_use = utilization;

  dlist_push_last(&measurement->codec_usage, codec_use);

  EVEL_EXIT();
}


/**************************************************************************//**
 * Set the Media Ports in Use property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement         Pointer to the measurement.
 * @param media_ports_in_use  The media port usage to set.
 *****************************************************************************/
void evel_measurement_media_port_use_set(EVENT_MEASUREMENT * measurement,
                                         int media_ports_in_use)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(media_ports_in_use >= 0);

  evel_set_option_int(&measurement->media_ports_in_use,
                      media_ports_in_use,
                      "Media Ports In Use");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the VNFC Scaling Metric property of the Measurement.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param measurement     Pointer to the measurement.
 * @param scaling_metric  The scaling metric to set.
 *****************************************************************************/
void evel_measurement_vnfc_scaling_metric_set(EVENT_MEASUREMENT * measurement,
                                              int scaling_metric)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(scaling_metric >= 0.0);

  evel_set_option_int(&measurement->vnfc_scaling_metric,
                         scaling_metric,
                         "VNFC Scaling Metric");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Create a new Latency Bucket to be added to a Measurement event.
 *
 * @note    The mandatory fields on the ::MEASUREMENT_LATENCY_BUCKET must be
 *          supplied to this factory function and are immutable once set.
 *          Optional fields have explicit setter functions, but again values
 *          may only be set once so that the ::MEASUREMENT_LATENCY_BUCKET has
 *          immutable properties.
 *
 * @param count         Count of events in this bucket.
 *
 * @returns pointer to the newly manufactured ::MEASUREMENT_LATENCY_BUCKET.
 *          If the structure is not used it must be released using free.
 * @retval  NULL  Failed to create the Latency Bucket.
 *****************************************************************************/
MEASUREMENT_LATENCY_BUCKET * evel_new_meas_latency_bucket(const int count)
{
  MEASUREMENT_LATENCY_BUCKET * bucket;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(count >= 0);

  /***************************************************************************/
  /* Allocate, then set Mandatory Parameters.                                */
  /***************************************************************************/
  EVEL_DEBUG("Creating bucket, count = %d", count);
  bucket = malloc(sizeof(MEASUREMENT_LATENCY_BUCKET));
  assert(bucket != NULL);

  /***************************************************************************/
  /* Set Mandatory Parameters.                                               */
  /***************************************************************************/
  bucket->count = count;

  /***************************************************************************/
  /* Initialize Optional Parameters.                                         */
  /***************************************************************************/
  evel_init_option_double(&bucket->high_end);
  evel_init_option_double(&bucket->low_end);

  EVEL_EXIT();

  return bucket;
}

/**************************************************************************//**
 * Set the High End property of the Measurement Latency Bucket.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param bucket        Pointer to the Measurement Latency Bucket.
 * @param high_end      High end of the bucket's range.
 *****************************************************************************/
void evel_meas_latency_bucket_high_end_set(
                                     MEASUREMENT_LATENCY_BUCKET * const bucket,
                                     const double high_end)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(high_end >= 0.0);
  evel_set_option_double(&bucket->high_end, high_end, "High End");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Low End property of the Measurement Latency Bucket.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param bucket        Pointer to the Measurement Latency Bucket.
 * @param low_end       Low end of the bucket's range.
 *****************************************************************************/
void evel_meas_latency_bucket_low_end_set(
                                     MEASUREMENT_LATENCY_BUCKET * const bucket,
                                     const double low_end)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(low_end >= 0.0);
  evel_set_option_double(&bucket->low_end, low_end, "Low End");
  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional Measurement Latency Bucket to the specified event.
 *
 * @param measurement   Pointer to the Measurement event.
 * @param bucket        Pointer to the Measurement Latency Bucket to add.
 *****************************************************************************/
void evel_meas_latency_bucket_add(EVENT_MEASUREMENT * const measurement,
                                  MEASUREMENT_LATENCY_BUCKET * const bucket)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(bucket != NULL);
  dlist_push_last(&measurement->latency_distribution, bucket);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional Latency Distribution bucket to the Measurement.
 *
 * This function implements the previous API, purely for convenience.
 *
 * @param measurement   Pointer to the measurement.
 * @param low_end       Low end of the bucket's range.
 * @param high_end      High end of the bucket's range.
 * @param count         Count of events in this bucket.
 *****************************************************************************/
void evel_measurement_latency_add(EVENT_MEASUREMENT * const measurement,
                                  const double low_end,
                                  const double high_end,
                                  const int count)
{
  MEASUREMENT_LATENCY_BUCKET * bucket = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Trust the assertions in the underlying methods.                         */
  /***************************************************************************/
  bucket = evel_new_meas_latency_bucket(count);
  evel_meas_latency_bucket_low_end_set(bucket, low_end);
  evel_meas_latency_bucket_high_end_set(bucket, high_end);
  evel_meas_latency_bucket_add(measurement, bucket);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Create a new vNIC Use to be added to a Measurement event.
 *
 * @note    The mandatory fields on the ::MEASUREMENT_VNIC_PERFORMANCE must be supplied
 *          to this factory function and are immutable once set. Optional
 *          fields have explicit setter functions, but again values may only be
 *          set once so that the ::MEASUREMENT_VNIC_PERFORMANCE has immutable
 *          properties.
 *
 * @param vnic_id               ASCIIZ string with the vNIC's ID.
 * @param val_suspect           True or false confidence in data.
 *
 * @returns pointer to the newly manufactured ::MEASUREMENT_VNIC_PERFORMANCE.
 *          If the structure is not used it must be released using
 *          ::evel_measurement_free_vnic_performance.
 * @retval  NULL  Failed to create the vNIC Use.
 *****************************************************************************/
MEASUREMENT_VNIC_PERFORMANCE * evel_measurement_new_vnic_performance(char * const vnic_id,
                                                     char * const val_suspect)
{
  MEASUREMENT_VNIC_PERFORMANCE * vnic_performance;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(vnic_id != NULL);
  assert(!strcmp(val_suspect,"true") || !strcmp(val_suspect,"false"));

  /***************************************************************************/
  /* Allocate, then set Mandatory Parameters.                                */
  /***************************************************************************/
  EVEL_DEBUG("Adding VNIC ID=%s", vnic_id);
  vnic_performance = malloc(sizeof(MEASUREMENT_VNIC_PERFORMANCE));
  assert(vnic_performance != NULL);
  vnic_performance->vnic_id = strdup(vnic_id);
  vnic_performance->valuesaresuspect = strdup(val_suspect);

  /***************************************************************************/
  /* Initialize Optional Parameters.                                         */
  /***************************************************************************/
  evel_init_option_double(&vnic_performance-> recvd_bcast_packets_acc);
  evel_init_option_double(&vnic_performance-> recvd_bcast_packets_delta);
  evel_init_option_double(&vnic_performance-> recvd_discarded_packets_acc);
  evel_init_option_double(&vnic_performance-> recvd_discarded_packets_delta);
  evel_init_option_double(&vnic_performance-> recvd_error_packets_acc);
  evel_init_option_double(&vnic_performance-> recvd_error_packets_delta);
  evel_init_option_double(&vnic_performance-> recvd_mcast_packets_acc);
  evel_init_option_double(&vnic_performance-> recvd_mcast_packets_delta);
  evel_init_option_double(&vnic_performance-> recvd_octets_acc);
  evel_init_option_double(&vnic_performance-> recvd_octets_delta);
  evel_init_option_double(&vnic_performance-> recvd_total_packets_acc);
  evel_init_option_double(&vnic_performance-> recvd_total_packets_delta);
  evel_init_option_double(&vnic_performance-> recvd_ucast_packets_acc);
  evel_init_option_double(&vnic_performance-> recvd_ucast_packets_delta);
  evel_init_option_double(&vnic_performance-> tx_bcast_packets_acc);
  evel_init_option_double(&vnic_performance-> tx_bcast_packets_delta);
  evel_init_option_double(&vnic_performance-> tx_discarded_packets_acc);
  evel_init_option_double(&vnic_performance-> tx_discarded_packets_delta);
  evel_init_option_double(&vnic_performance-> tx_error_packets_acc);
  evel_init_option_double(&vnic_performance-> tx_error_packets_delta);
  evel_init_option_double(&vnic_performance-> tx_mcast_packets_acc);
  evel_init_option_double(&vnic_performance-> tx_mcast_packets_delta);
  evel_init_option_double(&vnic_performance-> tx_octets_acc);
  evel_init_option_double(&vnic_performance-> tx_octets_delta);
  evel_init_option_double(&vnic_performance-> tx_total_packets_acc);
  evel_init_option_double(&vnic_performance-> tx_total_packets_delta);
  evel_init_option_double(&vnic_performance-> tx_ucast_packets_acc);
  evel_init_option_double(&vnic_performance-> tx_ucast_packets_delta);

  EVEL_EXIT();

  return vnic_performance;
}

/**************************************************************************//**
 * Free a vNIC Use.
 *
 * Free off the ::MEASUREMENT_VNIC_PERFORMANCE supplied.  Will free all the contained
 * allocated memory.
 *
 * @note It does not free the vNIC Use itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_measurement_free_vnic_performance(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(vnic_performance != NULL);
  assert(vnic_performance->vnic_id != NULL);
  assert(vnic_performance->valuesaresuspect != NULL);

  /***************************************************************************/
  /* Free the duplicated string.                                             */
  /***************************************************************************/
  free(vnic_performance->vnic_id);
  free(vnic_performance->valuesaresuspect);
  vnic_performance->vnic_id = NULL;

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Accumulated Broadcast Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_bcast_packets_acc
 *****************************************************************************/
void evel_vnic_performance_rx_bcast_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_bcast_packets_acc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(recvd_bcast_packets_acc >= 0.0);

  evel_set_option_double(&vnic_performance->recvd_bcast_packets_acc,
                      recvd_bcast_packets_acc,
                      "Broadcast Packets accumulated");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Delta Broadcast Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_bcast_packets_delta
 *****************************************************************************/
void evel_vnic_performance_rx_bcast_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_bcast_packets_delta)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(recvd_bcast_packets_delta >= 0.0);

  evel_set_option_double(&vnic_performance->recvd_bcast_packets_delta,
                      recvd_bcast_packets_delta,
                      "Delta Broadcast Packets recieved");

  EVEL_EXIT();
}


/**************************************************************************//**
 * Set the Discarded Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_discard_packets_acc
 *****************************************************************************/
void evel_vnic_performance_rx_discard_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_discard_packets_acc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(recvd_discard_packets_acc >= 0.0);

  evel_set_option_double(&vnic_performance->recvd_discarded_packets_acc,
                      recvd_discard_packets_acc,
                      "Discarded Packets accumulated");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Delta Discarded Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_discard_packets_delta
 *****************************************************************************/
void evel_vnic_performance_rx_discard_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_discard_packets_delta)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(recvd_discard_packets_delta >= 0.0);

  evel_set_option_double(&vnic_performance->recvd_discarded_packets_delta,
                      recvd_discard_packets_delta,
                      "Delta Discarded Packets recieved");

  EVEL_EXIT();
}


/**************************************************************************//**
 * Set the Error Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_error_packets_acc
 *****************************************************************************/
void evel_vnic_performance_rx_error_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_error_packets_acc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(recvd_error_packets_acc >= 0.0);

  evel_set_option_double(&vnic_performance->recvd_error_packets_acc,
                      recvd_error_packets_acc,
                      "Error Packets received accumulated");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Delta Error Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_error_packets_delta
 *****************************************************************************/
void evel_vnic_performance_rx_error_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_error_packets_delta)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(recvd_error_packets_delta >= 0.0);

  evel_set_option_double(&vnic_performance->recvd_error_packets_delta,
                      recvd_error_packets_delta,
                      "Delta Error Packets recieved");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Accumulated Multicast Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_mcast_packets_acc
 *****************************************************************************/
void evel_vnic_performance_rx_mcast_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_mcast_packets_acc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(recvd_mcast_packets_acc >= 0.0);

  evel_set_option_double(&vnic_performance->recvd_mcast_packets_acc,
                      recvd_mcast_packets_acc,
                      "Multicast Packets accumulated");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Delta Multicast Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_mcast_packets_delta
 *****************************************************************************/
void evel_vnic_performance_rx_mcast_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_mcast_packets_delta)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(recvd_mcast_packets_delta >= 0.0);

  evel_set_option_double(&vnic_performance->recvd_mcast_packets_delta,
                      recvd_mcast_packets_delta,
                      "Delta Multicast Packets recieved");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Accumulated Octets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_octets_acc
 *****************************************************************************/
void evel_vnic_performance_rx_octets_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_octets_acc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(recvd_octets_acc >= 0.0);

  evel_set_option_double(&vnic_performance->recvd_octets_acc,
                      recvd_octets_acc,
                      "Octets received accumulated");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Delta Octets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_octets_delta
 *****************************************************************************/
void evel_vnic_performance_rx_octets_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_octets_delta)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(recvd_octets_delta >= 0.0);

  evel_set_option_double(&vnic_performance->recvd_octets_delta,
                      recvd_octets_delta,
                      "Delta Octets recieved");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Accumulated Total Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_total_packets_acc
 *****************************************************************************/
void evel_vnic_performance_rx_total_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_total_packets_acc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(recvd_total_packets_acc >= 0.0);

  evel_set_option_double(&vnic_performance->recvd_total_packets_acc,
                      recvd_total_packets_acc,
                      "Total Packets accumulated");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Delta Total Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_total_packets_delta
 *****************************************************************************/
void evel_vnic_performance_rx_total_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_total_packets_delta)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(recvd_total_packets_delta >= 0.0);

  evel_set_option_double(&vnic_performance->recvd_total_packets_delta,
                      recvd_total_packets_delta,
                      "Delta Total Packets recieved");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Accumulated Unicast Packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_ucast_packets_acc
 *****************************************************************************/
void evel_vnic_performance_rx_ucast_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_ucast_packets_acc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(recvd_ucast_packets_acc >= 0.0);

  evel_set_option_double(&vnic_performance->recvd_ucast_packets_acc,
                      recvd_ucast_packets_acc,
                      "Unicast Packets received accumulated");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Delta Unicast packets Received in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param recvd_ucast_packets_delta
 *****************************************************************************/
void evel_vnic_performance_rx_ucast_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double recvd_ucast_packets_delta)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(recvd_ucast_packets_delta >= 0.0);

  evel_set_option_double(&vnic_performance->recvd_ucast_packets_delta,
                      recvd_ucast_packets_delta,
                      "Delta Unicast packets recieved");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Transmitted Broadcast Packets in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_bcast_packets_acc
 *****************************************************************************/
void evel_vnic_performance_tx_bcast_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_bcast_packets_acc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(tx_bcast_packets_acc >= 0.0);

  evel_set_option_double(&vnic_performance->tx_bcast_packets_acc,
                      tx_bcast_packets_acc,
                      "Transmitted Broadcast Packets accumulated");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Delta Broadcast packets Transmitted in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_bcast_packets_delta
 *****************************************************************************/
void evel_vnic_performance_tx_bcast_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_bcast_packets_delta)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(tx_bcast_packets_delta >= 0.0);

  evel_set_option_double(&vnic_performance->tx_bcast_packets_delta,
                      tx_bcast_packets_delta,
                      "Delta Transmitted Broadcast packets ");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Transmitted Discarded Packets in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_discarded_packets_acc
 *****************************************************************************/
void evel_vnic_performance_tx_discarded_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_discarded_packets_acc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(tx_discarded_packets_acc >= 0.0);

  evel_set_option_double(&vnic_performance->tx_discarded_packets_acc,
                      tx_discarded_packets_acc,
                      "Transmitted Discarded Packets accumulated");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Delta Discarded packets Transmitted in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_discarded_packets_delta
 *****************************************************************************/
void evel_vnic_performance_tx_discarded_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_discarded_packets_delta)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(tx_discarded_packets_delta >= 0.0);

  evel_set_option_double(&vnic_performance->tx_discarded_packets_delta,
                      tx_discarded_packets_delta,
                      "Delta Transmitted Discarded packets ");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Transmitted Errored Packets in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_error_packets_acc
 *****************************************************************************/
void evel_vnic_performance_tx_error_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_error_packets_acc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(tx_error_packets_acc >= 0.0);

  evel_set_option_double(&vnic_performance->tx_error_packets_acc,
                      tx_error_packets_acc,
                      "Transmitted Error Packets accumulated");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Delta Errored packets Transmitted in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_error_packets_delta
 *****************************************************************************/
void evel_vnic_performance_tx_error_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_error_packets_delta)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(tx_error_packets_delta >= 0.0);

  evel_set_option_double(&vnic_performance->tx_error_packets_delta,
                      tx_error_packets_delta,
                      "Delta Transmitted Error packets ");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Transmitted Multicast Packets in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_mcast_packets_acc
 *****************************************************************************/
void evel_vnic_performance_tx_mcast_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_mcast_packets_acc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(tx_mcast_packets_acc >= 0.0);

  evel_set_option_double(&vnic_performance->tx_mcast_packets_acc,
                      tx_mcast_packets_acc,
                      "Transmitted Multicast Packets accumulated");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Delta Multicast packets Transmitted in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_mcast_packets_delta
 *****************************************************************************/
void evel_vnic_performance_tx_mcast_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_mcast_packets_delta)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(tx_mcast_packets_delta >= 0.0);

  evel_set_option_double(&vnic_performance->tx_mcast_packets_delta,
                      tx_mcast_packets_delta,
                      "Delta Transmitted Multicast packets ");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Transmitted Octets in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_octets_acc
 *****************************************************************************/
void evel_vnic_performance_tx_octets_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_octets_acc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(tx_octets_acc >= 0.0);

  evel_set_option_double(&vnic_performance->tx_octets_acc,
                      tx_octets_acc,
                      "Transmitted Octets accumulated");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Delta Octets Transmitted in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_octets_delta
 *****************************************************************************/
void evel_vnic_performance_tx_octets_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_octets_delta)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(tx_octets_delta >= 0.0);

  evel_set_option_double(&vnic_performance->tx_octets_delta,
                      tx_octets_delta,
                      "Delta Transmitted Octets ");

  EVEL_EXIT();
}


/**************************************************************************//**
 * Set the Transmitted Total Packets in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_total_packets_acc
 *****************************************************************************/
void evel_vnic_performance_tx_total_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_total_packets_acc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(tx_total_packets_acc >= 0.0);

  evel_set_option_double(&vnic_performance->tx_total_packets_acc,
                      tx_total_packets_acc,
                      "Transmitted Total Packets accumulated");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Delta Total Packets Transmitted in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_total_packets_delta
 *****************************************************************************/
void evel_vnic_performance_tx_total_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_total_packets_delta)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(tx_total_packets_delta >= 0.0);

  evel_set_option_double(&vnic_performance->tx_total_packets_delta,
                      tx_total_packets_delta,
                      "Delta Transmitted Total Packets ");

  EVEL_EXIT();
}


/**************************************************************************//**
 * Set the Transmitted Unicast Packets in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_ucast_packets_acc
 *****************************************************************************/
void evel_vnic_performance_tx_ucast_pkt_acc_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_ucast_packets_acc)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(tx_ucast_packets_acc >= 0.0);

  evel_set_option_double(&vnic_performance->tx_ucast_packets_acc,
                      tx_ucast_packets_acc,
                      "Transmitted Unicast Packets accumulated");

  EVEL_EXIT();
}

/**************************************************************************//**
 * Set the Delta Octets Transmitted in measurement interval
 * property of the vNIC performance.
 *
 * @note  The property is treated as immutable: it is only valid to call
 *        the setter once.  However, we don't assert if the caller tries to
 *        overwrite, just ignoring the update instead.
 *
 * @param vnic_performance      Pointer to the vNIC Use.
 * @param tx_ucast_packets_delta
 *****************************************************************************/
void evel_vnic_performance_tx_ucast_pkt_delta_set(MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance,
                                    const double tx_ucast_packets_delta)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(tx_ucast_packets_delta >= 0.0);

  evel_set_option_double(&vnic_performance->tx_ucast_packets_delta,
                      tx_ucast_packets_delta,
                      "Delta Transmitted Unicast Packets ");

  EVEL_EXIT();
}


/**************************************************************************//**
 * Add an additional vNIC Use to the specified Measurement event.
 *
 * @param measurement   Pointer to the measurement.
 * @param vnic_performance      Pointer to the vNIC Use to add.
 *****************************************************************************/
void evel_meas_vnic_performance_add(EVENT_MEASUREMENT * const measurement,
                            MEASUREMENT_VNIC_PERFORMANCE * const vnic_performance)
{
  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(measurement != NULL);
  assert(measurement->header.event_domain == EVEL_DOMAIN_MEASUREMENT);
  assert(vnic_performance != NULL);

  dlist_push_last(&measurement->vnic_usage, vnic_performance);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Add an additional vNIC usage record Measurement.
 *
 * This function implements the previous API, purely for convenience.
 *
 * The ID is null delimited ASCII string.  The library takes a copy so the
 * caller does not have to preserve values after the function returns.
 *
 * @param measurement           Pointer to the measurement.
 * @param vnic_id               ASCIIZ string with the vNIC's ID.
 * @param valset                true or false confidence level
 * @param recvd_bcast_packets_acc         Recieved broadcast packets
 * @param recvd_bcast_packets_delta       Received delta broadcast packets
 * @param recvd_discarded_packets_acc     Recieved discarded packets
 * @param recvd_discarded_packets_delta   Received discarded delta packets
 * @param recvd_error_packets_acc         Received error packets
 * @param recvd_error_packets_delta,      Received delta error packets
 * @param recvd_mcast_packets_acc         Received multicast packets
 * @param recvd_mcast_packets_delta       Received delta multicast packets
 * @param recvd_octets_acc                Received octets
 * @param recvd_octets_delta              Received delta octets
 * @param recvd_total_packets_acc         Received total packets
 * @param recvd_total_packets_delta       Received delta total packets
 * @param recvd_ucast_packets_acc         Received Unicast packets
 * @param recvd_ucast_packets_delta       Received delta unicast packets
 * @param tx_bcast_packets_acc            Transmitted broadcast packets
 * @param tx_bcast_packets_delta          Transmitted delta broadcast packets
 * @param tx_discarded_packets_acc        Transmitted packets discarded
 * @param tx_discarded_packets_delta      Transmitted delta discarded packets
 * @param tx_error_packets_acc            Transmitted error packets
 * @param tx_error_packets_delta          Transmitted delta error packets
 * @param tx_mcast_packets_acc            Transmitted multicast packets accumulated
 * @param tx_mcast_packets_delta          Transmitted delta multicast packets
 * @param tx_octets_acc                   Transmitted octets
 * @param tx_octets_delta                 Transmitted delta octets
 * @param tx_total_packets_acc            Transmitted total packets
 * @param tx_total_packets_delta          Transmitted delta total packets
 * @param tx_ucast_packets_acc            Transmitted Unicast packets
 * @param tx_ucast_packets_delta          Transmitted delta Unicast packets
 *****************************************************************************/
void evel_measurement_vnic_performance_add(EVENT_MEASUREMENT * const measurement,
                               char * const vnic_id,
                               char * valset,
                               double recvd_bcast_packets_acc,
                               double recvd_bcast_packets_delta,
                               double recvd_discarded_packets_acc,
                               double recvd_discarded_packets_delta,
                               double recvd_error_packets_acc,
                               double recvd_error_packets_delta,
                               double recvd_mcast_packets_acc,
                               double recvd_mcast_packets_delta,
                               double recvd_octets_acc,
                               double recvd_octets_delta,
                               double recvd_total_packets_acc,
                               double recvd_total_packets_delta,
                               double recvd_ucast_packets_acc,
                               double recvd_ucast_packets_delta,
                               double tx_bcast_packets_acc,
                               double tx_bcast_packets_delta,
                               double tx_discarded_packets_acc,
                               double tx_discarded_packets_delta,
                               double tx_error_packets_acc,
                               double tx_error_packets_delta,
                               double tx_mcast_packets_acc,
                               double tx_mcast_packets_delta,
                               double tx_octets_acc,
                               double tx_octets_delta,
                               double tx_total_packets_acc,
                               double tx_total_packets_delta,
                               double tx_ucast_packets_acc,
                               double tx_ucast_packets_delta)
{
  MEASUREMENT_VNIC_PERFORMANCE * vnic_performance = NULL;
  EVEL_ENTER();

  /***************************************************************************/
  /* Trust the assertions in the underlying methods.                         */
  /***************************************************************************/
  vnic_performance = evel_measurement_new_vnic_performance(vnic_id, valset);
                                           
  evel_vnic_performance_rx_bcast_pkt_acc_set(vnic_performance, recvd_bcast_packets_acc);
  evel_vnic_performance_rx_bcast_pkt_delta_set(vnic_performance, recvd_bcast_packets_delta);
  evel_vnic_performance_rx_discard_pkt_acc_set(vnic_performance, recvd_discarded_packets_acc);
  evel_vnic_performance_rx_discard_pkt_delta_set(vnic_performance, recvd_discarded_packets_delta);
  evel_vnic_performance_rx_error_pkt_acc_set(vnic_performance, recvd_error_packets_acc);
  evel_vnic_performance_rx_error_pkt_delta_set(vnic_performance, recvd_error_packets_delta);
  evel_vnic_performance_rx_mcast_pkt_acc_set(vnic_performance, recvd_mcast_packets_acc);
  evel_vnic_performance_rx_mcast_pkt_delta_set(vnic_performance, recvd_mcast_packets_delta);
  evel_vnic_performance_rx_octets_acc_set(vnic_performance, recvd_octets_acc);
  evel_vnic_performance_rx_octets_delta_set(vnic_performance, recvd_octets_delta);
  evel_vnic_performance_rx_total_pkt_acc_set(vnic_performance, recvd_total_packets_acc);
  evel_vnic_performance_rx_total_pkt_delta_set(vnic_performance, recvd_total_packets_delta);
  evel_vnic_performance_rx_ucast_pkt_acc_set(vnic_performance, recvd_ucast_packets_acc);
  evel_vnic_performance_rx_ucast_pkt_delta_set(vnic_performance, recvd_ucast_packets_delta);
  evel_vnic_performance_tx_bcast_pkt_acc_set(vnic_performance, tx_bcast_packets_acc);
  evel_vnic_performance_tx_bcast_pkt_delta_set(vnic_performance, tx_bcast_packets_delta);
  evel_vnic_performance_tx_discarded_pkt_acc_set(vnic_performance, tx_discarded_packets_acc);
  evel_vnic_performance_tx_discarded_pkt_delta_set(vnic_performance, tx_discarded_packets_delta);
  evel_vnic_performance_tx_error_pkt_acc_set(vnic_performance, tx_error_packets_acc);
  evel_vnic_performance_tx_error_pkt_delta_set(vnic_performance, tx_error_packets_delta);
  evel_vnic_performance_tx_mcast_pkt_acc_set(vnic_performance, tx_mcast_packets_acc);
  evel_vnic_performance_tx_mcast_pkt_delta_set(vnic_performance, tx_mcast_packets_delta);
  evel_vnic_performance_tx_octets_acc_set(vnic_performance, tx_octets_acc);
  evel_vnic_performance_tx_octets_delta_set(vnic_performance, tx_octets_delta);
  evel_vnic_performance_tx_total_pkt_acc_set(vnic_performance, tx_total_packets_acc);
  evel_vnic_performance_tx_total_pkt_delta_set(vnic_performance, tx_total_packets_delta);
  evel_vnic_performance_tx_ucast_pkt_acc_set(vnic_performance, tx_ucast_packets_acc);
  evel_vnic_performance_tx_ucast_pkt_delta_set(vnic_performance, tx_ucast_packets_delta);
  evel_meas_vnic_performance_add(measurement, vnic_performance);
}

/**************************************************************************//**
 * Encode the measurement as a JSON measurement.
 *
 * @param jbuf          Pointer to the ::EVEL_JSON_BUFFER to encode into.
 * @param event         Pointer to the ::EVENT_HEADER to encode.
 *****************************************************************************/
void evel_json_encode_measurement(EVEL_JSON_BUFFER * jbuf,
                                  EVENT_MEASUREMENT * event)
{
  MEASUREMENT_CPU_USE * cpu_use = NULL;
  MEASUREMENT_MEM_USE * mem_use = NULL;
  MEASUREMENT_DISK_USE * disk_use = NULL;
  MEASUREMENT_FSYS_USE * fsys_use = NULL;
  MEASUREMENT_LATENCY_BUCKET * bucket = NULL;
  MEASUREMENT_VNIC_PERFORMANCE * vnic_performance = NULL;
  MEASUREMENT_ERRORS * errors = NULL;
  MEASUREMENT_FEATURE_USE * feature_use = NULL;
  MEASUREMENT_CODEC_USE * codec_use = NULL;
  MEASUREMENT_GROUP * measurement_group = NULL;
  CUSTOM_MEASUREMENT * custom_measurement = NULL;
  DLIST_ITEM * item = NULL;
  DLIST_ITEM * nested_item = NULL;
  DLIST_ITEM * addl_info_item = NULL;
  OTHER_FIELD *addl_info = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.                                                    */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_MEASUREMENT);

  evel_json_encode_header(jbuf, &event->header);
  evel_json_open_named_object(jbuf, "measurementsForVfScalingFields");

  /***************************************************************************/
  /* Mandatory fields.                                                       */
  /***************************************************************************/
  evel_enc_kv_int(jbuf, "measurementInterval", event->measurement_interval);

  /***************************************************************************/
  /* Optional fields.                                                        */
  /***************************************************************************/
  // additional fields
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "additionalFields"))
  {
    bool item_added = false;

    addl_info_item = dlist_get_first(&event->additional_info);
    while (addl_info_item != NULL)
    {
      addl_info = (OTHER_FIELD*) addl_info_item->item;
      assert(addl_info != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "additionalFields",
                                          addl_info->name))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_string(jbuf, "name", addl_info->name);
        evel_enc_kv_string(jbuf, "value", addl_info->value);
        evel_json_close_object(jbuf);
        item_added = true;
      }
      addl_info_item = dlist_get_next(addl_info_item);
    }
    evel_json_close_list(jbuf);

    /*************************************************************************/
    /* If we've not written anything, rewind to before we opened the list.   */
    /*************************************************************************/
    if (!item_added)
    {
      evel_json_rewind(jbuf);
    }
  }

  // TBD additional json objects
  evel_enc_kv_opt_int(jbuf, "concurrentSessions", &event->concurrent_sessions);
  evel_enc_kv_opt_int(jbuf, "configuredEntities", &event->configured_entities);

  /***************************************************************************/
  /* CPU Use list.                                                           */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "cpuUsageArray"))
  {
    bool item_added = false;

    item = dlist_get_first(&event->cpu_usage);
    while (item != NULL)
    {
      cpu_use = (MEASUREMENT_CPU_USE*) item->item;
      assert(cpu_use != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "cpuUsageArray",
                                          cpu_use->id))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_string(jbuf, "cpuIdentifier", cpu_use->id);
        evel_enc_kv_opt_double(jbuf, "cpuIdle", &cpu_use->idle);
        evel_enc_kv_opt_double(jbuf, "cpuUsageInterrupt", &cpu_use->intrpt);
        evel_enc_kv_opt_double(jbuf, "cpuUsageNice", &cpu_use->nice);
        evel_enc_kv_opt_double(jbuf, "cpuUsageSoftIrq", &cpu_use->softirq);
        evel_enc_kv_opt_double(jbuf, "cpuUsageSteal", &cpu_use->steal);
        evel_enc_kv_opt_double(jbuf, "cpuUsageSystem", &cpu_use->sys);
        evel_enc_kv_opt_double(jbuf, "cpuUsageUser", &cpu_use->user);
        evel_enc_kv_opt_double(jbuf, "cpuWait", &cpu_use->wait);
        evel_enc_kv_double(jbuf, "percentUsage",cpu_use->usage);
        evel_json_close_object(jbuf);
        item_added = true;
      }
      item = dlist_get_next(item);
    }
    evel_json_close_list(jbuf);

    /*************************************************************************/
    /* If we've not written anything, rewind to before we opened the list.   */
    /*************************************************************************/
    if (!item_added)
    {
      evel_json_rewind(jbuf);
    }
  }


  /***************************************************************************/
  /* Disk Use list.                                                           */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "diskUsageArray"))
  {
    bool item_added = false;

    item = dlist_get_first(&event->disk_usage);
    while (item != NULL)
    {
      disk_use = (MEASUREMENT_DISK_USE*) item->item;
      assert(disk_use != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "diskUsageArray",
                                          disk_use->id))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_string(jbuf, "diskIdentifier", disk_use->id);
        evel_enc_kv_opt_double(jbuf, "diskIoTimeAvg", &disk_use->iotimeavg);
        evel_enc_kv_opt_double(jbuf, "diskIoTimeLast", &disk_use->iotimelast);
        evel_enc_kv_opt_double(jbuf, "diskIoTimeMax", &disk_use->iotimemax);
        evel_enc_kv_opt_double(jbuf, "diskIoTimeMin", &disk_use->iotimemin);
        evel_enc_kv_opt_double(jbuf, "diskMergedReadAvg", &disk_use->mergereadavg);
        evel_enc_kv_opt_double(jbuf, "diskMergedReadLast", &disk_use->mergereadlast);
        evel_enc_kv_opt_double(jbuf, "diskMergedReadMax", &disk_use->mergereadmax);
        evel_enc_kv_opt_double(jbuf, "diskMergedReadMin", &disk_use->mergereadmin);
        evel_enc_kv_opt_double(jbuf, "diskMergedWriteAvg", &disk_use->mergewriteavg);
        evel_enc_kv_opt_double(jbuf, "diskMergedWriteLast", &disk_use->mergewritelast);
        evel_enc_kv_opt_double(jbuf, "diskMergedWriteMax", &disk_use->mergewritemax);
        evel_enc_kv_opt_double(jbuf, "diskMergedWriteMin", &disk_use->mergewritemin);
        evel_enc_kv_opt_double(jbuf, "diskOctetsReadAvg", &disk_use->octetsreadavg);
        evel_enc_kv_opt_double(jbuf, "diskOctetsReadLast", &disk_use->octetsreadlast);
        evel_enc_kv_opt_double(jbuf, "diskOctetsReadMax", &disk_use->octetsreadmax);
        evel_enc_kv_opt_double(jbuf, "diskOctetsReadMin", &disk_use->octetsreadmin);
        evel_enc_kv_opt_double(jbuf, "diskOctetsWriteAvg", &disk_use->octetswriteavg);
        evel_enc_kv_opt_double(jbuf, "diskOctetsWriteLast", &disk_use->octetswritelast);
        evel_enc_kv_opt_double(jbuf, "diskOctetsWriteMax", &disk_use->octetswritemax);
        evel_enc_kv_opt_double(jbuf, "diskOctetsWriteMin", &disk_use->octetswritemin);
        evel_enc_kv_opt_double(jbuf, "diskOpsReadAvg", &disk_use->opsreadavg);
        evel_enc_kv_opt_double(jbuf, "diskOpsReadLast", &disk_use->opsreadlast);
        evel_enc_kv_opt_double(jbuf, "diskOpsReadMax", &disk_use->opsreadmax);
        evel_enc_kv_opt_double(jbuf, "diskOpsReadMin", &disk_use->opsreadmin);
        evel_enc_kv_opt_double(jbuf, "diskOpsWriteAvg", &disk_use->opswriteavg);
        evel_enc_kv_opt_double(jbuf, "diskOpsWriteLast", &disk_use->opswritelast);
        evel_enc_kv_opt_double(jbuf, "diskOpsWriteMax", &disk_use->opswritemax);
        evel_enc_kv_opt_double(jbuf, "diskOpsWriteMin", &disk_use->opswritemin);
        evel_enc_kv_opt_double(jbuf, "diskPendingOperationsAvg", &disk_use->pendingopsavg);
        evel_enc_kv_opt_double(jbuf, "diskPendingOperationsLast", &disk_use->pendingopslast);
        evel_enc_kv_opt_double(jbuf, "diskPendingOperationsMax", &disk_use->pendingopsmax);
        evel_enc_kv_opt_double(jbuf, "diskPendingOperationsMin", &disk_use->pendingopsmin);
        evel_enc_kv_opt_double(jbuf, "diskTimeReadAvg", &disk_use->timereadavg);
        evel_enc_kv_opt_double(jbuf, "diskTimeReadLast", &disk_use->timereadlast);
        evel_enc_kv_opt_double(jbuf, "diskTimeReadMax", &disk_use->timereadmax);
        evel_enc_kv_opt_double(jbuf, "diskTimeReadMin", &disk_use->timereadmin);
        evel_enc_kv_opt_double(jbuf, "diskTimeWriteAvg", &disk_use->timewriteavg);
        evel_enc_kv_opt_double(jbuf, "diskTimeWriteLast", &disk_use->timewritelast);
        evel_enc_kv_opt_double(jbuf, "diskTimeWriteMax", &disk_use->timewritemax);
        evel_enc_kv_opt_double(jbuf, "diskTimeWriteMin", &disk_use->timewritemin);
        evel_json_close_object(jbuf);
        item_added = true;
      }
      item = dlist_get_next(item);
    }
    evel_json_close_list(jbuf);

    /*************************************************************************/
    /* If we've not written anything, rewind to before we opened the list.   */
    /*************************************************************************/
    if (!item_added)
    {
      evel_json_rewind(jbuf);
    }
  }

  /***************************************************************************/
  /* Filesystem Usage list.                                                  */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "filesystemUsageArray"))
  {
    bool item_added = false;

    item = dlist_get_first(&event->filesystem_usage);
    while (item != NULL)
    {
      fsys_use = (MEASUREMENT_FSYS_USE *) item->item;
      assert(fsys_use != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "filesystemUsageArray",
                                          fsys_use->filesystem_name))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_string(jbuf, "filesystemName", fsys_use->filesystem_name);
        evel_enc_kv_double(
          jbuf, "blockConfigured", fsys_use->block_configured);
        evel_enc_kv_double(jbuf, "blockIops", fsys_use->block_iops);
        evel_enc_kv_double(jbuf, "blockUsed", fsys_use->block_used);
        evel_enc_kv_double(
          jbuf, "ephemeralConfigured", fsys_use->ephemeral_configured);
        evel_enc_kv_double(jbuf, "ephemeralIops", fsys_use->ephemeral_iops);
        evel_enc_kv_double(jbuf, "ephemeralUsed", fsys_use->ephemeral_used);
        evel_json_close_object(jbuf);
        item_added = true;
      }
      item = dlist_get_next(item);
    }
    evel_json_close_list(jbuf);

    /*************************************************************************/
    /* If we've not written anything, rewind to before we opened the list.   */
    /*************************************************************************/
    if (!item_added)
    {
      evel_json_rewind(jbuf);
    }
  }

  /***************************************************************************/
  /* Latency distribution.                                                   */
  /***************************************************************************/
  item = dlist_get_first(&event->latency_distribution);
  if ((item != NULL) &&
      evel_json_open_opt_named_list(jbuf, "latencyDistribution"))
  {
    while (item != NULL)
    {
      bucket = (MEASUREMENT_LATENCY_BUCKET*) item->item;
      assert(bucket != NULL);

      evel_json_open_object(jbuf);
      evel_enc_kv_opt_double(
        jbuf, "lowEndOfLatencyBucket", &bucket->low_end);
      evel_enc_kv_opt_double(
        jbuf, "highEndOfLatencyBucket", &bucket->high_end);
      evel_enc_kv_int(jbuf, "countsInTheBucket", bucket->count);
      evel_json_close_object(jbuf);
      item = dlist_get_next(item);
    }
    evel_json_close_list(jbuf);
  }

  evel_enc_kv_opt_double(
    jbuf, "meanRequestLatency", &event->mean_request_latency);
  evel_enc_kv_opt_int(jbuf, "requestRate", &event->request_rate);

  /***************************************************************************/
  /* vNIC Usage TBD Performance array                          */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "vNicUsageArray"))
  {
    bool item_added = false;

    item = dlist_get_first(&event->vnic_usage);
    while (item != NULL)
    {
      vnic_performance = (MEASUREMENT_VNIC_PERFORMANCE *) item->item;
      assert(vnic_performance != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "vNicPerformanceArray",
                                          vnic_performance->vnic_id))
      {
        evel_json_open_object(jbuf);

        /*********************************************************************/
        /* Optional fields.                                                  */
        /*********************************************************************/
        evel_enc_kv_opt_double( jbuf,
		 "receivedBroadcastPacketsAccumulated", &vnic_performance->recvd_bcast_packets_acc);
        evel_enc_kv_opt_double( jbuf,
		 "receivedBroadcastPacketsDelta", &vnic_performance->recvd_bcast_packets_delta);
        evel_enc_kv_opt_double( jbuf,
		 "receivedDiscardedPacketsAccumulated", &vnic_performance->recvd_discarded_packets_acc);
        evel_enc_kv_opt_double( jbuf,
		 "receivedDiscardedPacketsDelta", &vnic_performance->recvd_discarded_packets_delta);
        evel_enc_kv_opt_double( jbuf,
		 "receivedErrorPacketsAccumulated", &vnic_performance->recvd_error_packets_acc);
        evel_enc_kv_opt_double( jbuf,
		 "receivedErrorPacketsDelta", &vnic_performance->recvd_error_packets_delta);
        evel_enc_kv_opt_double( jbuf,
		 "receivedMulticastPacketsAccumulated", &vnic_performance->recvd_mcast_packets_acc);
        evel_enc_kv_opt_double( jbuf,
		 "receivedMulticastPacketsDelta", &vnic_performance->recvd_mcast_packets_delta);
        evel_enc_kv_opt_double( jbuf,
		 "receivedOctetsAccumulated", &vnic_performance->recvd_octets_acc);
        evel_enc_kv_opt_double( jbuf,
		 "receivedOctetsDelta", &vnic_performance->recvd_octets_delta);
        evel_enc_kv_opt_double( jbuf,
		 "receivedTotalPacketsAccumulated", &vnic_performance->recvd_total_packets_acc);
        evel_enc_kv_opt_double( jbuf,
		 "receivedTotalPacketsDelta", &vnic_performance->recvd_total_packets_delta);
        evel_enc_kv_opt_double( jbuf,
		 "receivedUnicastPacketsAccumulated", &vnic_performance->recvd_ucast_packets_acc);
        evel_enc_kv_opt_double( jbuf,
		 "receivedUnicastPacketsDelta", &vnic_performance->recvd_ucast_packets_delta);
        evel_enc_kv_opt_double( jbuf,
		 "transmittedBroadcastPacketsAccumulated", &vnic_performance->tx_bcast_packets_acc);
        evel_enc_kv_opt_double( jbuf,
		 "transmittedBroadcastPacketsDelta", &vnic_performance->tx_bcast_packets_delta);
        evel_enc_kv_opt_double( jbuf,
		 "transmittedDiscardedPacketsAccumulated", &vnic_performance->tx_discarded_packets_acc);
        evel_enc_kv_opt_double( jbuf,
		 "transmittedDiscardedPacketsDelta", &vnic_performance->tx_discarded_packets_delta);
        evel_enc_kv_opt_double( jbuf,
		 "transmittedErrorPacketsAccumulated", &vnic_performance->tx_error_packets_acc);
        evel_enc_kv_opt_double( jbuf,
		 "transmittedErrorPacketsDelta", &vnic_performance->tx_error_packets_delta);
        evel_enc_kv_opt_double( jbuf,
		 "transmittedMulticastPacketsAccumulated", &vnic_performance->tx_mcast_packets_acc);
        evel_enc_kv_opt_double( jbuf,
		 "transmittedMulticastPacketsDelta", &vnic_performance->tx_mcast_packets_delta);
        evel_enc_kv_opt_double( jbuf,
		 "transmittedOctetsAccumulated", &vnic_performance->tx_octets_acc);
        evel_enc_kv_opt_double( jbuf,
		 "transmittedOctetsDelta", &vnic_performance->tx_octets_delta);
        evel_enc_kv_opt_double( jbuf,
		 "transmittedTotalPacketsAccumulated", &vnic_performance->tx_total_packets_acc);
        evel_enc_kv_opt_double( jbuf,
		 "transmittedTotalPacketsDelta", &vnic_performance->tx_total_packets_delta);
        evel_enc_kv_opt_double( jbuf,
		 "transmittedUnicastPacketsAccumulated", &vnic_performance->tx_ucast_packets_acc);
        evel_enc_kv_opt_double( jbuf,
		 "transmittedUnicastPacketsDelta", &vnic_performance->tx_ucast_packets_delta);

        /*********************************************************************/
        /* Mandatory fields.                                                 */
        /*********************************************************************/
        evel_enc_kv_string(jbuf, "valuesAreSuspect", vnic_performance->valuesaresuspect);
        evel_enc_kv_string(jbuf, "vNicIdentifier", vnic_performance->vnic_id);

        evel_json_close_object(jbuf);
        item_added = true;
      }
      item = dlist_get_next(item);
    }

    evel_json_close_list(jbuf);

    /*************************************************************************/
    /* If we've not written anything, rewind to before we opened the list.   */
    /*************************************************************************/
    if (!item_added)
    {
      evel_json_rewind(jbuf);
    }
  }


  /***************************************************************************/
  /* Memory Use list.                                                           */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "memoryUsageArray"))
  {
    bool item_added = false;

    item = dlist_get_first(&event->mem_usage);
    while (item != NULL)
    {
      mem_use = (MEASUREMENT_MEM_USE*) item->item;
      assert(mem_use != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "memoryUsageArray",
                                          mem_use->id))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_double(jbuf, "memoryBuffered", mem_use->membuffsz);
        evel_enc_kv_opt_double(jbuf, "memoryCached", &mem_use->memcache);
        evel_enc_kv_opt_double(jbuf, "memoryConfigured", &mem_use->memconfig);
        evel_enc_kv_opt_double(jbuf, "memoryFree", &mem_use->memfree);
        evel_enc_kv_opt_double(jbuf, "memorySlabRecl", &mem_use->slabrecl);
        evel_enc_kv_opt_double(jbuf, "memorySlabUnrecl", &mem_use->slabunrecl);
        evel_enc_kv_opt_double(jbuf, "memoryUsed", &mem_use->memused);
        evel_enc_kv_string(jbuf, "vmIdentifier", mem_use->id);
        evel_json_close_object(jbuf);
        item_added = true;
      }
      item = dlist_get_next(item);
    }
    evel_json_close_list(jbuf);

    /*************************************************************************/
    /* If we've not written anything, rewind to before we opened the list.   */
    /*************************************************************************/
    if (!item_added)
    {
      evel_json_rewind(jbuf);
    }
  }


  evel_enc_kv_opt_int(
    jbuf, "numberOfMediaPortsInUse", &event->media_ports_in_use);
  evel_enc_kv_opt_int(
    jbuf, "vnfcScalingMetric", &event->vnfc_scaling_metric);

  /***************************************************************************/
  /* Errors list.                                                            */
  /***************************************************************************/
  if ((event->errors != NULL) &&
      evel_json_open_opt_named_object(jbuf, "errors"))
  {
    errors = event->errors;
    evel_enc_kv_int(jbuf, "receiveDiscards", errors->receive_discards);
    evel_enc_kv_int(jbuf, "receiveErrors", errors->receive_errors);
    evel_enc_kv_int(jbuf, "transmitDiscards", errors->transmit_discards);
    evel_enc_kv_int(jbuf, "transmitErrors", errors->transmit_errors);
    evel_json_close_object(jbuf);
  }

  /***************************************************************************/
  /* Feature Utilization list.                                               */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "featureUsageArray"))
  {
    bool item_added = false;

    item = dlist_get_first(&event->feature_usage);
    while (item != NULL)
    {
      feature_use = (MEASUREMENT_FEATURE_USE*) item->item;
      assert(feature_use != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "featureUsageArray",
                                          feature_use->feature_id))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_string(jbuf, "featureIdentifier", feature_use->feature_id);
        evel_enc_kv_int(
          jbuf, "featureUtilization", feature_use->feature_utilization);
        evel_json_close_object(jbuf);
        item_added = true;
      }
      item = dlist_get_next(item);
    }
    evel_json_close_list(jbuf);

    /*************************************************************************/
    /* If we've not written anything, rewind to before we opened the list.   */
    /*************************************************************************/
    if (!item_added)
    {
      evel_json_rewind(jbuf);
    }
  }

  /***************************************************************************/
  /* Codec Utilization list.                                                 */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "codecUsageArray"))
  {
    bool item_added = false;

    item = dlist_get_first(&event->codec_usage);
    while (item != NULL)
    {
      codec_use = (MEASUREMENT_CODEC_USE*) item->item;
      assert(codec_use != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "codecUsageArray",
                                          codec_use->codec_id))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_string(jbuf, "codecIdentifier", codec_use->codec_id);
        evel_enc_kv_int(jbuf, "numberInUse", codec_use->number_in_use);
        evel_json_close_object(jbuf);
        item_added = true;
      }
      item = dlist_get_next(item);
    }
    evel_json_close_list(jbuf);

    /*************************************************************************/
    /* If we've not written anything, rewind to before we opened the list.   */
    /*************************************************************************/
    if (!item_added)
    {
      evel_json_rewind(jbuf);
    }
  }

  /***************************************************************************/
  /* Additional Measurement Groups list.                                     */
  /***************************************************************************/
  evel_json_checkpoint(jbuf);
  if (evel_json_open_opt_named_list(jbuf, "additionalMeasurements"))
  {
    bool item_added = false;

    item = dlist_get_first(&event->additional_measurements);
    while (item != NULL)
    {
      measurement_group = (MEASUREMENT_GROUP *) item->item;
      assert(measurement_group != NULL);

      if (!evel_throttle_suppress_nv_pair(jbuf->throttle_spec,
                                          "additionalMeasurements",
                                          measurement_group->name))
      {
        evel_json_open_object(jbuf);
        evel_enc_kv_string(jbuf, "name", measurement_group->name);
        evel_json_open_opt_named_list(jbuf, "arrayOfFields");

        /*********************************************************************/
        /* Measurements list.                                                */
        /*********************************************************************/
        nested_item = dlist_get_first(&measurement_group->measurements);
        while (nested_item != NULL)
        {
          custom_measurement = (CUSTOM_MEASUREMENT *) nested_item->item;
          assert(custom_measurement != NULL);

          evel_json_open_object(jbuf);
          evel_enc_kv_string(jbuf, "name", custom_measurement->name);
          evel_enc_kv_string(jbuf, "value", custom_measurement->value);
          evel_json_close_object(jbuf);
          nested_item = dlist_get_next(nested_item);
        }
        evel_json_close_list(jbuf);
        evel_json_close_object(jbuf);
        item_added = true;
      }
      item = dlist_get_next(item);
    }
    evel_json_close_list(jbuf);

    /*************************************************************************/
    /* If we've not written anything, rewind to before we opened the list.   */
    /*************************************************************************/
    if (!item_added)
    {
      evel_json_rewind(jbuf);
    }
  }

  /***************************************************************************/
  /* Although optional, we always generate the version.  Note that this      */
  /* closes the object, too.                                                 */
  /***************************************************************************/
  evel_enc_version(jbuf,
                   "measurementsForVfScalingVersion",
                   event->major_version,
                   event->minor_version);
  evel_json_close_object(jbuf);

  EVEL_EXIT();
}

/**************************************************************************//**
 * Free a Measurement.
 *
 * Free off the Measurement supplied.  Will free all the contained allocated
 * memory.
 *
 * @note It does not free the Measurement itself, since that may be part of a
 * larger structure.
 *****************************************************************************/
void evel_free_measurement(EVENT_MEASUREMENT * event)
{
  MEASUREMENT_CPU_USE * cpu_use = NULL;
  MEASUREMENT_DISK_USE * disk_use = NULL;
  MEASUREMENT_FSYS_USE * fsys_use = NULL;
  MEASUREMENT_LATENCY_BUCKET * bucket = NULL;
  MEASUREMENT_MEM_USE * mem_use = NULL;
  MEASUREMENT_VNIC_PERFORMANCE * vnic_performance = NULL;
  MEASUREMENT_FEATURE_USE * feature_use = NULL;
  MEASUREMENT_CODEC_USE * codec_use = NULL;
  MEASUREMENT_GROUP * measurement_group = NULL;
  CUSTOM_MEASUREMENT * measurement = NULL;
  OTHER_FIELD *addl_info = NULL;

  EVEL_ENTER();

  /***************************************************************************/
  /* Check preconditions.  As an internal API we don't allow freeing NULL    */
  /* events as we do on the public API.                                      */
  /***************************************************************************/
  assert(event != NULL);
  assert(event->header.event_domain == EVEL_DOMAIN_MEASUREMENT);

  /***************************************************************************/
  /* Free all internal strings then the header itself.                       */
  /***************************************************************************/
  addl_info = dlist_pop_last(&event->additional_info);
  while (addl_info != NULL)
  {
    EVEL_DEBUG("Freeing Additional Info (%s, %s)",
               addl_info->name,
               addl_info->value);
    free(addl_info->name);
    free(addl_info->value);
    free(addl_info);
    addl_info = dlist_pop_last(&event->additional_info);
  }



  cpu_use = dlist_pop_last(&event->cpu_usage);
  while (cpu_use != NULL)
  {
    EVEL_DEBUG("Freeing CPU use Info (%s)", cpu_use->id);
    free(cpu_use->id);
    free(cpu_use);
    cpu_use = dlist_pop_last(&event->cpu_usage);
  }
  disk_use = dlist_pop_last(&event->disk_usage);
  while (disk_use != NULL)
  {
    EVEL_DEBUG("Freeing Disk use Info (%s)", disk_use->id);
    free(disk_use->id);
    free(disk_use);
    disk_use = dlist_pop_last(&event->disk_usage);
  }
  mem_use = dlist_pop_last(&event->mem_usage);
  while (mem_use != NULL)
  {
    EVEL_DEBUG("Freeing Memory use Info (%s)", mem_use->id);
    free(mem_use->id);
    free(mem_use->vmid);
    free(mem_use);
    mem_use = dlist_pop_last(&event->mem_usage);
  }

  fsys_use = dlist_pop_last(&event->filesystem_usage);
  while (fsys_use != NULL)
  {
    EVEL_DEBUG("Freeing Filesystem Use info (%s)", fsys_use->filesystem_name);
    free(fsys_use->filesystem_name);
    free(fsys_use);
    fsys_use = dlist_pop_last(&event->filesystem_usage);
  }

  bucket = dlist_pop_last(&event->latency_distribution);
  while (bucket != NULL)
  {
    EVEL_DEBUG("Freeing Latency Bucket");
    free(bucket);
    bucket = dlist_pop_last(&event->latency_distribution);
  }

  vnic_performance = dlist_pop_last(&event->vnic_usage);
  while (vnic_performance != NULL)
  {
    EVEL_DEBUG("Freeing vNIC performance Info (%s)", vnic_performance->vnic_id);
    evel_measurement_free_vnic_performance(vnic_performance);
    free(vnic_performance);
    vnic_performance = dlist_pop_last(&event->vnic_usage);
  }

  codec_use = dlist_pop_last(&event->codec_usage);
  while (codec_use != NULL)
  {
    EVEL_DEBUG("Freeing Codec use Info (%s)", codec_use->codec_id);
    free(codec_use->codec_id);
    free(codec_use);
    codec_use = dlist_pop_last(&event->codec_usage);
  }

  if (event->errors != NULL)
  {
    EVEL_DEBUG("Freeing Errors");
    free(event->errors);
  }

  feature_use = dlist_pop_last(&event->feature_usage);
  while (feature_use != NULL)
  {
    EVEL_DEBUG("Freeing Feature use Info (%s)", feature_use->feature_id);
    free(feature_use->feature_id);
    free(feature_use);
    feature_use = dlist_pop_last(&event->feature_usage);
  }

  measurement_group = dlist_pop_last(&event->additional_measurements);
  while (measurement_group != NULL)
  {
    EVEL_DEBUG("Freeing Measurement Group (%s)", measurement_group->name);

    measurement = dlist_pop_last(&measurement_group->measurements);
    while (measurement != NULL)
    {
      EVEL_DEBUG("Freeing Measurement (%s)", measurement->name);
      free(measurement->name);
      free(measurement->value);
      free(measurement);
      measurement = dlist_pop_last(&measurement_group->measurements);
    }
    free(measurement_group->name);
    free(measurement_group);
    measurement_group = dlist_pop_last(&event->additional_measurements);
  }

  evel_free_header(&event->header);

  EVEL_EXIT();
}

