#!/usr/bin/env python
'''
Example script to inject a throttling command list to the test_collector.

Only intended for test purposes.

License
-------

 * ===================================================================
 * Copyright © 2017 AT&T Intellectual Property. All rights reserved.
 * ===================================================================
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

'''
import optparse
import requests
import json

###############################################################################
# Functions to build up commandList contents
###############################################################################
def command_state():
    "return a provideThrottlingState command"
    return {'command':
            {'commandType': 'provideThrottlingState'}}

def command_interval(interval):
    "return a measurementIntervalChange command"
    return {'command':
            {'commandType': 'measurementIntervalChange',
             'measurementInterval': interval}}

def command_throttle(domain, fields, pairs):
    "return a throttlingSpecification"
    throttle_spec = {'eventDomain' : domain}
    if len(fields):
        throttle_spec['suppressedFieldNames'] = fields
    if len(pairs):
        throttle_spec['suppressedNvPairsList'] = pairs
    return {'command':
            {'commandType': 'throttlingSpecification',
             'eventDomainThrottleSpecification': throttle_spec}}

def command_nvpairs(field_name, pair_names):
    "return a suppressedNvPairs"
    return {'nvPairFieldName' : field_name,
            'suppressedNvPairNames' : pair_names}

###############################################################################
# Example functions to build up commandLists for various domains.
###############################################################################
def command_list_empty():
    return {'commandList' : []}

def command_list_provide():
    return {'commandList' : [command_state()]}

def command_list_interval(interval):
    return {'commandList' : [command_interval(interval)]}

def command_list_fault_suppress_fields():
    "Throttling Specification - two suppressedFieldNames"
    fields = ['alarmInterfaceA', 'alarmAdditionalInformation']
    pairs = []
    command_list = [command_throttle('fault', fields, pairs)]
    return {'commandList' : command_list}

def command_list_fault_suppress_nothing():
    "Throttling Specification - no suppression"
    fields = []
    pairs = []
    command_list = [command_throttle('fault', fields, pairs)]
    return {'commandList' : command_list}

def command_list_fault_suppress_pairs():
    "Throttling Specification - two suppressedNvPairNames"
    fields = []
    pairs = [command_nvpairs('alarmAdditionalInformation',
                                   ['name1', 'name2'])]
    command_list = [command_throttle('fault', fields, pairs)]
    return {'commandList' : command_list}

def command_list_fault_suppress_fields_and_pairs():
    "Throttling Specification - a mixture of fields and pairs"
    fields = ['alarmInterfaceA']
    pairs = [command_nvpairs('alarmAdditionalInformation',
                                   ['name1', 'name2'])]
    command_list = [command_throttle('fault', fields, pairs)]
    return {'commandList' : command_list}

def command_list_measurements_suppress_example():
    "Throttling Specification - measurements"
    fields = ['numberOfMediaPortsInUse', 'aggregateCpuUsage']
    pairs = [command_nvpairs('cpuUsageArray',
                             ['cpu1', 'cpu3'])]
    command_list = [command_throttle('measurementsForVfScaling',
                                     fields, pairs)]
    return {'commandList' : command_list}

def command_list_mobile_flow_suppress_example():
    "Throttling Specification - mobile flow"
    fields = ['radioAccessTechnology', 'samplingAlgorithm']
    pairs = []
    command_list = [command_throttle('mobileFlow', fields, pairs)]
    return {'commandList' : command_list}

def command_list_state_change_suppress_example():
    "Throttling Specification - state change"
    fields = ['reportingEntityId', 'eventType', 'sourceId']
    pairs = [command_nvpairs('additionalFields', ['Name1'])]
    command_list = [command_throttle('stateChange', fields, pairs)]
    return {'commandList' : command_list}

def command_list_syslog_suppress_example():
    "Throttling Specification - syslog"
    fields = ['syslogFacility', 'syslogProc', 'syslogProcId']
    pairs = [command_nvpairs('additionalFields', ['Name1', 'Name4'])]
    command_list = [command_throttle('syslog', fields, pairs)]
    return {'commandList' : command_list}

def command_list_reset_all_domains():
    "Throttling Specification - reset all domains"
    command_list = [command_throttle('fault', [], []),
                    command_throttle('measurementsForVfScaling', [], []),
                    command_throttle('mobileFlow', [], []),
                    command_throttle('stateChange', [], []),
                    command_throttle('syslog', [], [])]
    return {'commandList' : command_list}

def mixed_example():
    fields = ['alarmInterfaceA']
    pairs = [command_nvpairs('alarmAdditionalInformation',
                             ['name1', 'name2'])]
    command_list = [command_throttle('fault', fields, pairs),
                    command_interval(10),
                    command_state()]
    return {'commandList' : command_list}

###############################################################################
# Default command line values
###############################################################################
DEFAULT_FQDN = "127.0.0.1"
DEFAULT_PORT = 30000

###############################################################################
# Command Line Parsing
###############################################################################
parser = optparse.OptionParser()
parser.add_option('--fqdn',
                  action="store",
                  dest="fqdn",
                  default=DEFAULT_FQDN)
parser.add_option('--port',
                  action="store",
                  dest="port",
                  default=DEFAULT_PORT,
                  type="int")
options, remainder = parser.parse_args()

###############################################################################
# Derive the Test Control URL
###############################################################################
url = 'http://%s:%d/testControl/v1.1/commandList'%(options.fqdn, options.port)

###############################################################################
# Create JSON and POST it to the Test Control URL.
###############################################################################
command_list = command_list_fault_suppress_fields_and_pairs()
requests.post(url, json = command_list)
