#ifndef METADATA_INCLUDED
#define METADATA_INCLUDED
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
 * Wrap the OpenStack metadata service.
 *
 ****************************************************************************/

#include "evel.h"

/**************************************************************************//**
 * Download metadata from the OpenStack metadata service.
 *
 * @param verbosity   Controls whether to generate debug to stdout.  Zero:
 *                    none.  Non-zero: generate debug.
 * @returns Status code
 * @retval  EVEL_SUCCESS      On success
 * @retval  ::EVEL_ERR_CODES  On failure.
 *****************************************************************************/
EVEL_ERR_CODES openstack_metadata(int verbosity);

/**************************************************************************//**
 * Initialize default values for vm_name and vm_uuid - for testing purposes.
 *****************************************************************************/
void openstack_metadata_initialize();

/**************************************************************************//**
 * Get the VM name provided by the metadata service.
 *
 * @returns VM name
 *****************************************************************************/
const char *openstack_vm_name();

/**************************************************************************//**
 * Get the VM UUID provided by the metadata service.
 *
 * @returns VM UUID
 *****************************************************************************/
const char *openstack_vm_uuid();

#endif
