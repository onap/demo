#ifndef METADATA_INCLUDED
#define METADATA_INCLUDED
/**************************************************************************//**
 * @file
 * Wrap the OpenStack metadata service.
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
