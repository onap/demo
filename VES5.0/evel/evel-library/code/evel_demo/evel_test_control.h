#ifndef EVEL_TEST_CONTROL
#define EVEL_TEST_CONTROL
/**************************************************************************//**
 * @file
 * Utility to post test control commands to the test_collector testControl API.
 *
 * This software is a test capability, allowing test cases to trigger
 * expected throttling behaviours at the test collector, for automated test
 * capabilty.
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
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:  This product includes software
 * developed by the AT&T.
 * 4. Neither the name of AT&T nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
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

/**************************************************************************//**
 * POST provide JSON to the test_collector testControl API.
 *
 * This function does not take ownership of the json_buffer passed in.
 *
 * @param json_buffer   Pointer to the JSON to POST
 * @param json_size     The number of bytes to POST
 * @param secure        Whether to use HTTPS (0=HTTP, 1=HTTPS)
 * @param fqdn          The test control API FQDN or IP address.
 * @param port          The test control API port.
 *****************************************************************************/
void evel_test_control(char * const json_buffer,
                       const int json_size,
                       const int secure,
                       const char * fqdn,
                       const int port);

typedef enum {
  TC_RESET_ALL_DOMAINS,
  TC_FAULT_SUPPRESS_FIELDS,
  TC_FAULT_SUPPRESS_FIELDS_AND_PAIRS,
  TC_FAULT_SUPPRESS_NOTHING,
  TC_FAULT_SUPPRESS_PAIRS,
  TC_MEAS_SUPPRESS_FIELDS_AND_PAIRS,
  TC_MOBILE_SUPPRESS_FIELDS_AND_PAIRS,
  TC_SIGNALING_SUPPRESS_FIELDS,
  TC_SERVICE_SUPPRESS_FIELDS_AND_PAIRS,
  TC_STATE_SUPPRESS_FIELDS_AND_PAIRS,
  TC_SYSLOG_SUPPRESS_FIELDS_AND_PAIRS,
  TC_PROVIDE_THROTTLING_SPEC
} EVEL_TEST_CONTROL_SCENARIO;

/**************************************************************************//**
 * POST a pre-set test scenario to the test_collector testControl API.
 *
 * This function provides various pre-configured scenarios, purely to avoid
 * duplicating them elsewhere.
 *
 * @param scenario      The scenario to POST.
 * @param secure        Whether to use HTTPS (0=HTTP, 1=HTTPS)
 * @param fqdn          The test control API FQDN or IP address.
 * @param port          The test control API port.
 *****************************************************************************/
void evel_test_control_scenario(const EVEL_TEST_CONTROL_SCENARIO scenario,
                                const int secure,
                                const char * fqdn,
                                const int port);

/**************************************************************************//**
 * POST a measurement interval change to the test_collector testControl API.
 *
 * @param interval      The measurement interval.
 * @param secure        Whether to use HTTPS (0=HTTP, 1=HTTPS)
 * @param fqdn          The test control API FQDN or IP address.
 * @param port          The test control API port.
 *****************************************************************************/
void evel_test_control_meas_interval(const int interval,
                                     const int secure,
                                     const char * fqdn,
                                     const int port);

#endif
