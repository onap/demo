'''
/*-
* ============LICENSE_START=======================================================
* Copyright (C) 2019 Orange
* ================================================================================
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* ============LICENSE_END=========================================================
*/
'''

import os
import json
import sys
import uuid
import time
import copy
import netifaces as ni
import warnings
import contextlib
import requests
import simplejson
import http.server
import threading
from datetime import datetime
from datetime import timedelta
from simple_rest_client.api import API
from simple_rest_client.resource import Resource
from basicauth import encode
from pprint import pprint
from random import randint
from urllib3.exceptions import InsecureRequestWarning


old_merge_environment_settings = requests.Session.merge_environment_settings

hostname_cache = []
ansible_inventory = {}
osdf_response = {"last": { "id": "id", "data": None}}


class BaseServer(http.server.BaseHTTPRequestHandler):

    def __init__(self, one, two, three):
        self.osdf_resp = osdf_response
        super().__init__(one, two, three)

    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()

    def do_GET(self):
        self._set_headers()

    def do_HEAD(self):
        self._set_headers()

    def do_POST(self):
        self._set_headers()
        self.data_string = self.rfile.read(int(self.headers['Content-Length']))
        self.send_response(200)
        self.end_headers()

        data = simplejson.loads(self.data_string)
        #print(json.dumps(data, indent=4))
        self.osdf_resp["last"]["data"] = data
        self.osdf_resp["last"]["id"] = data["requestId"]
        with open("response.json", "w") as outfile:
            simplejson.dump(data, outfile)


def _run_osdf_resp_server():
    server_address = ('', 9000)
    httpd = http.server.HTTPServer(server_address, BaseServer)
    print('Starting OSDF Response Server...')
    httpd.serve_forever()

@contextlib.contextmanager
def _no_ssl_verification():
    opened_adapters = set()

    def merge_environment_settings(self, url, proxies, stream, verify, cert):
        # Verification happens only once per connection so we need to close
        # all the opened adapters once we're done. Otherwise, the effects of
        # verify=False persist beyond the end of this context manager.
        opened_adapters.add(self.get_adapter(url))

        settings = old_merge_environment_settings(self, url, proxies, stream, verify, cert)
        settings['verify'] = False

        return settings

    requests.Session.merge_environment_settings = merge_environment_settings

    try:
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', InsecureRequestWarning)
            yield
    finally:
        requests.Session.merge_environment_settings = old_merge_environment_settings

        for adapter in opened_adapters:
            try:
                adapter.close()
            except:
                pass


def _get_aai_rel_link_data(data, related_to, search_key=None, match_dict=None):
    # some strings that we will encounter frequently
    rel_lst = "relationship-list"
    rkey = "relationship-key"
    rval = "relationship-value"
    rdata = "relationship-data"
    response = list()
    if match_dict:
        m_key = match_dict.get('key')
        m_value = match_dict.get('value')
    else:
        m_key = None
        m_value = None
    rel_dict = data.get(rel_lst)
    if rel_dict:  # check if data has relationship lists
        for key, rel_list in rel_dict.items():
            for rel in rel_list:
                if rel.get("related-to") == related_to:
                    dval = None
                    matched = False
                    link = rel.get("related-link")
                    r_data = rel.get(rdata, [])
                    if search_key:
                        for rd in r_data:
                            if rd.get(rkey) == search_key:
                                dval = rd.get(rval)
                                if not match_dict:  # return first match
                                    response.append(
                                        {"link": link, "d_value": dval}
                                    )
                                    break  # go to next relation
                            if rd.get(rkey) == m_key \
                                    and rd.get(rval) == m_value:
                                matched = True
                        if match_dict and matched:  # if matching required
                            response.append(
                                {"link": link, "d_value": dval}
                            )
                            # matched, return search value corresponding
                            # to the matched r_data group
                    else:  # no search key; just return the link
                        response.append(
                            {"link": link, "d_value": dval}
                        )
    if len(response) == 0:
        response.append(
            {"link": None, "d_value": None}
        )
    return response


class AAIApiResource(Resource):
    actions = {
        'generic_vnf': {'method': 'GET', 'url': 'network/generic-vnfs/generic-vnf/{}'},
        'link': {'method': 'GET', 'url': '{}'},
        'service_instance': {'method': 'GET',
                             'url': 'business/customers/customer/{}/service-subscriptions/service-subscription/{}/service-instances/service-instance/{}'}
    }


class HASApiResource(Resource):
    actions = {
        'plans': {'method': 'POST', 'url': 'plans/'},
        'plan': {'method': 'GET', 'url': 'plans/{}'}
    }


class OSDFApiResource(Resource):
    actions = {
        'placement': {'method': 'POST', 'url': 'placement'}
    }


class APPCLcmApiResource(Resource):
    actions = {
        'distribute_traffic': {'method': 'POST', 'url': 'appc-provider-lcm:distribute-traffic/'},
        'distribute_traffic_check': {'method': 'POST', 'url': 'appc-provider-lcm:distribute-traffic-check/'},
        'action_status': {'method': 'POST', 'url': 'appc-provider-lcm:action-status/'},
    }


def _init_python_aai_api(onap_ip):
    api = API(
        api_root_url="https://{}:30233/aai/v14/".format(onap_ip),
        params={},
        headers={
            'Authorization': encode("AAI", "AAI"),
            'X-FromAppId': 'SCRIPT',
            'Accept': 'application/json',
            'Content-Type': 'application/json',
            'X-TransactionId': str(uuid.uuid4()),
        },
        timeout=30,
        append_slash=False,
        json_encode_body=True # encode body as json
    )
    api.add_resource(resource_name='aai', resource_class=AAIApiResource)
    return api


def _init_python_has_api(onap_ip):
    api = API(
        api_root_url="https://{}:30275/v1/".format(onap_ip),
        params={},
        headers={
            'Authorization': encode("admin1", "plan.15"),
            'X-FromAppId': 'SCRIPT',
            'Accept': 'application/json',
            'Content-Type': 'application/json',
            'X-TransactionId': str(uuid.uuid4()),
        },
        timeout=30,
        append_slash=False,
        json_encode_body=True # encode body as json
    )
    api.add_resource(resource_name='has', resource_class=HASApiResource)
    return api


def _init_python_osdf_api(onap_ip):
    api = API(
        api_root_url="https://{}:30248/api/oof/v1/".format(onap_ip),
        params={},
        headers={
            'Authorization': encode("test", "testpwd"),
            'X-FromAppId': 'SCRIPT',
            'Accept': 'application/json',
            'Content-Type': 'application/json',
            'X-TransactionId': str(uuid.uuid4()),
        },
        timeout=30,
        append_slash=False,
        json_encode_body=True # encode body as json
    )
    api.add_resource(resource_name='osdf', resource_class=OSDFApiResource)
    return api


def _init_python_appc_lcm_api(onap_ip):
    api = API(
        api_root_url="http://{}:30230/restconf/operations/".format(onap_ip),
        params={},
        headers={
            'Authorization': encode("admin", "Kp8bJ4SXszM0WXlhak3eHlcse2gAw84vaoGGmJvUy2U"),
            'X-FromAppId': 'SCRIPT',
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        },
        timeout=300,
        append_slash=False,
        json_encode_body=True # encode body as json
    )
    api.add_resource(resource_name='lcm', resource_class=APPCLcmApiResource)
    return api


def load_aai_data(vfw_vnf_id, onap_ip):
    api = _init_python_aai_api(onap_ip)
    aai_data = {}
    aai_data['service-info'] = {'global-customer-id': '', 'service-instance-id': '', 'service-type': ''}
    aai_data['vfw-model-info'] = {'model-invariant-id': '', 'model-version-id': '', 'vnf-name': '', 'vnf-type': ''}
    aai_data['vpgn-model-info'] = {'model-invariant-id': '', 'model-version-id': '', 'vnf-name': '', 'vnf-type': ''}
    with _no_ssl_verification():
        response = api.aai.generic_vnf(vfw_vnf_id, body=None, params={'depth': 2}, headers={})
        aai_data['vfw-model-info']['model-invariant-id'] = response.body.get('model-invariant-id')
        aai_data['vfw-model-info']['model-version-id'] = response.body.get('model-version-id')
        aai_data['vfw-model-info']['vnf-name'] = response.body.get('vnf-name')
        aai_data['vfw-model-info']['vnf-type'] = response.body.get('vnf-type')
        aai_data['vf-module-id'] = response.body['vf-modules']['vf-module'][0]['vf-module-id']

        related_to = "service-instance"
        search_key = "customer.global-customer-id"
        rl_data_list = _get_aai_rel_link_data(data=response.body, related_to=related_to, search_key=search_key)
        aai_data['service-info']['global-customer-id'] = rl_data_list[0]['d_value']

        search_key = "service-subscription.service-type"
        rl_data_list = _get_aai_rel_link_data(data=response.body, related_to=related_to, search_key=search_key)
        aai_data['service-info']['service-type'] = rl_data_list[0]['d_value']

        search_key = "service-instance.service-instance-id"
        rl_data_list = _get_aai_rel_link_data(data=response.body, related_to=related_to, search_key=search_key)
        aai_data['service-info']['service-instance-id'] = rl_data_list[0]['d_value']

        service_link = rl_data_list[0]['link']
        response = api.aai.link(service_link, body=None, params={}, headers={})

        related_to = "generic-vnf"
        search_key = "generic-vnf.vnf-id"
        rl_data_list = _get_aai_rel_link_data(data=response.body, related_to=related_to, search_key=search_key)
        for i in range(0, len(rl_data_list)):
            vnf_id = rl_data_list[i]['d_value']

            if vnf_id != vfw_vnf_id:
                vnf_link = rl_data_list[i]['link']
                response = api.aai.link(vnf_link, body=None, params={}, headers={})
                if aai_data['vfw-model-info']['model-invariant-id'] != response.body.get('model-invariant-id'):
                    aai_data['vpgn-model-info']['model-invariant-id'] = response.body.get('model-invariant-id')
                    aai_data['vpgn-model-info']['model-version-id'] = response.body.get('model-version-id')
                    aai_data['vpgn-model-info']['vnf-name'] = response.body.get('vnf-name')
                    aai_data['vpgn-model-info']['vnf-type'] = response.body.get('vnf-type')
                    break
    return aai_data


def _osdf_request(rancher_ip, onap_ip, aai_data, exclude, use_oof_cache):
    dirname = os.path.join('templates/oof-cache/', aai_data['vf-module-id'])
    if exclude:
        file = os.path.join(dirname, 'sample-osdf-excluded.json')
    else:
        file = os.path.join(dirname, 'sample-osdf-required.json')
    if use_oof_cache and os.path.exists(file):
        migrate_from = json.loads(open(file).read())
        return migrate_from

    print('Making OSDF request for excluded {}'.format(str(exclude)))
    api = _init_python_osdf_api(onap_ip)
    request_id = str(uuid.uuid4())
    transaction_id = str(uuid.uuid4())
    callback_url = "http://{}:9000/osdfCallback/".format(str(rancher_ip))
    template = json.loads(open('templates/osdfRequest.json').read())
    template["requestInfo"]["transactionId"] = transaction_id
    template["requestInfo"]["requestId"] = request_id
    template["requestInfo"]["callbackUrl"] = callback_url
    template["serviceInfo"]["serviceInstanceId"] = aai_data['service-info']['service-instance-id']
    template["placementInfo"]["requestParameters"]["chosenCustomerId"] = aai_data['service-info']['global-customer-id']
    template["placementInfo"]["placementDemands"][0]["resourceModelInfo"]["modelInvariantId"] =\
        aai_data['vfw-model-info']['model-invariant-id']
    template["placementInfo"]["placementDemands"][0]["resourceModelInfo"]["modelVersionId"] =\
        aai_data['vfw-model-info']['model-version-id']
    template["placementInfo"]["placementDemands"][1]["resourceModelInfo"]["modelInvariantId"] =\
        aai_data['vpgn-model-info']['model-invariant-id']
    template["placementInfo"]["placementDemands"][1]["resourceModelInfo"]["modelVersionId"] =\
        aai_data['vpgn-model-info']['model-version-id']
    if exclude:
        template["placementInfo"]["placementDemands"][0]["excludedCandidates"][0]["identifiers"].\
            append(aai_data['vf-module-id'])
        del template["placementInfo"]["placementDemands"][0]["requiredCandidates"]
    else:
        template["placementInfo"]["placementDemands"][0]["requiredCandidates"][0]["identifiers"].\
            append(aai_data['vf-module-id'])
        del template["placementInfo"]["placementDemands"][0]["excludedCandidates"]

    #print(json.dumps(template, indent=4))

    with _no_ssl_verification():
        response = api.osdf.placement(body=template, params={}, headers={})
        #if response.body.get('error_message') is not None:
        #    raise Exception(response.body['error_message']['explanation'])

    counter = 0
    while counter < 600 and osdf_response["last"]["id"] != request_id:
        time.sleep(1)
        if counter % 20 == 0:
            print("solving")
        counter += 1

    if osdf_response["last"]["id"] == request_id:
        status = osdf_response["last"]["data"]["requestStatus"]
        if status == "completed":
            result = {
                "solution": osdf_response["last"]["data"]["solutions"]["placementSolutions"]
            }
            if not os.path.exists(dirname):
                os.makedirs(dirname)
            f = open(file, 'w+')
            f.write(json.dumps(result, indent=4))
            f.close()
            return result
        else:
            message = osdf_response["last"]["data"]["statusMessage"]
            raise Exception("OOF request {}: {}".format(status, message))
    else:
        raise Exception("No response for OOF request")


def _has_request(onap_ip, aai_data, exclude, use_oof_cache):
    dirname = os.path.join('templates/oof-cache/', aai_data['vf-module-id'])
    if exclude:
        file = os.path.join(dirname, 'sample-has-excluded.json')
    else:
        file = os.path.join(dirname, 'sample-has-required.json')
    if use_oof_cache and os.path.exists(file):
        migrate_from = json.loads(open(file).read())
        return migrate_from

    print('Making HAS request for excluded {}'.format(str(exclude)))
    api = _init_python_has_api(onap_ip)
    request_id = str(uuid.uuid4())
    template = json.loads(open('templates/hasRequest.json').read())
    result = {}
    template['name'] = request_id
    node = template['template']['parameters']
    node['chosen_customer_id'] = aai_data['service-info']['global-customer-id']
    node['service_id'] = aai_data['service-info']['service-instance-id']
    node = template['template']['demands']['vFW-SINK'][0]
    node['attributes']['model-invariant-id'] = aai_data['vfw-model-info']['model-invariant-id']
    node['attributes']['model-version-id'] = aai_data['vfw-model-info']['model-version-id']
    if exclude:
        node['excluded_candidates'][0]['candidate_id'][0] = aai_data['vf-module-id']
        del node['required_candidates']
    else:
        node['required_candidates'][0]['candidate_id'][0] = aai_data['vf-module-id']
        del node['excluded_candidates']
    node = template['template']['demands']['vPGN'][0]
    node['attributes']['model-invariant-id'] = aai_data['vpgn-model-info']['model-invariant-id']
    node['attributes']['model-version-id'] = aai_data['vpgn-model-info']['model-version-id']

    #print(json.dumps(template, indent=4))

    with _no_ssl_verification():
        response = api.has.plans(body=template, params={}, headers={})
        if response.body.get('error_message') is not None:
            raise Exception(response.body['error_message']['explanation'])
        else:
            plan_id = response.body['id']
            response = api.has.plan(plan_id, body=None, params={}, headers={})
            status = response.body['plans'][0]['status']
            while status != 'done' and status != 'error':
                print(status)
                response = api.has.plan(plan_id, body=None, params={}, headers={})
                status = response.body['plans'][0]['status']
            if status == 'done':
                result = response.body['plans'][0]['recommendations'][0]
            else:
                raise Exception(response.body['plans'][0]['message'])

    if not os.path.exists(dirname):
        os.makedirs(dirname)
    f = open(file, 'w+')
    f.write(json.dumps(result, indent=4))
    f.close()
    return result


def _extract_has_appc_identifiers(has_result, demand):
    if demand == 'vPGN':
        v_server = has_result[demand]['attributes']['vservers'][0]
    else:
        if len(has_result[demand]['attributes']['vservers'][0]['l-interfaces']) == 4:
            v_server = has_result[demand]['attributes']['vservers'][0]
        else:
            v_server = has_result[demand]['attributes']['vservers'][1]
    for itf in v_server['l-interfaces']:
        if itf['ipv4-addresses'][0].startswith("10.0."):
            ip = itf['ipv4-addresses'][0]
            break

    if v_server['vserver-name'] in hostname_cache and demand != 'vPGN':
        v_server['vserver-name'] = v_server['vserver-name'].replace("01", "02")
    hostname_cache.append(v_server['vserver-name'])

    config = {
        'vnf-id': has_result[demand]['attributes']['nf-id'],
        'vf-module-id': has_result[demand]['attributes']['vf-module-id'],
        'ip': ip,
        'vserver-id': v_server['vserver-id'],
        'vserver-name': v_server['vserver-name'],
        'vnfc-type': demand.lower(),
        'physical-location-id': has_result[demand]['attributes']['physical-location-id']
    }
    ansible_inventory_entry = "{} ansible_ssh_host={} ansible_ssh_user=ubuntu".format(config['vserver-name'], config['ip'])
    if demand.lower() not in ansible_inventory:
        ansible_inventory[demand.lower()] = {}
    ansible_inventory[demand.lower()][config['vserver-name']] = ansible_inventory_entry
    return config


def _extract_osdf_appc_identifiers(has_result, demand):
    if demand == 'vPGN':
        v_server = has_result[demand]['vservers'][0]
    else:
        if len(has_result[demand]['vservers'][0]['l-interfaces']) == 4:
            v_server = has_result[demand]['vservers'][0]
        else:
            v_server = has_result[demand]['vservers'][1]
    for itf in v_server['l-interfaces']:
        if itf['ipv4-addresses'][0].startswith("10.0."):
            ip = itf['ipv4-addresses'][0]
            break

    if v_server['vserver-name'] in hostname_cache and demand != 'vPGN':
        v_server['vserver-name'] = v_server['vserver-name'].replace("01", "02")
    hostname_cache.append(v_server['vserver-name'])

    config = {
        'vnf-id': has_result[demand]['nf-id'],
        'vf-module-id': has_result[demand]['vf-module-id'],
        'ip': ip,
        'vserver-id': v_server['vserver-id'],
        'vserver-name': v_server['vserver-name'],
        'vnfc-type': demand.lower(),
        'physical-location-id': has_result[demand]['locationId']
    }
    ansible_inventory_entry = "{} ansible_ssh_host={} ansible_ssh_user=ubuntu".format(config['vserver-name'], config['ip'])
    if demand.lower() not in ansible_inventory:
        ansible_inventory[demand.lower()] = {}
    ansible_inventory[demand.lower()][config['vserver-name']] = ansible_inventory_entry
    return config


def _extract_has_appc_dt_config(has_result, demand):
    if demand == 'vPGN':
        return {}
    else:
        config = {
            "nf-type": has_result[demand]['attributes']['nf-type'],
            "nf-name": has_result[demand]['attributes']['nf-name'],
            "vf-module-name": has_result[demand]['attributes']['vf-module-name'],
            "vnf-type": has_result[demand]['attributes']['vnf-type'],
            "service_instance_id": "319e60ef-08b1-47aa-ae92-51b97f05e1bc",
            "cloudClli": has_result[demand]['attributes']['physical-location-id'],
            "nf-id": has_result[demand]['attributes']['nf-id'],
            "vf-module-id": has_result[demand]['attributes']['vf-module-id'],
            "aic_version": has_result[demand]['attributes']['aic_version'],
            "ipv4-oam-address": has_result[demand]['attributes']['ipv4-oam-address'],
            "vnfHostName": has_result[demand]['candidate']['host_id'],
            "ipv6-oam-address": has_result[demand]['attributes']['ipv6-oam-address'],
            "cloudOwner": has_result[demand]['candidate']['cloud_owner'],
            "isRehome": has_result[demand]['candidate']['is_rehome'],
            "locationId": has_result[demand]['candidate']['location_id'],
            "locationType": has_result[demand]['candidate']['location_type'],
            'vservers': has_result[demand]['attributes']['vservers']
        }
        return config


def _extract_osdf_appc_dt_config(osdf_result, demand):
    if demand == 'vPGN':
        return {}
    else:
        return osdf_result[demand]


def _build_config_from_has(has_result):
    v_pgn_result = _extract_has_appc_identifiers(has_result, 'vPGN')
    v_fw_result = _extract_has_appc_identifiers(has_result, 'vFW-SINK')
    dt_config = _extract_has_appc_dt_config(has_result, 'vFW-SINK')

    config = {
        'vPGN': v_pgn_result,
        'vFW-SINK': v_fw_result
    }
    #print(json.dumps(config, indent=4))
    config['dt-config'] = {
        'destinations': [dt_config]
    }
    return config


def _adapt_osdf_result(osdf_result):
    result = {}
    demand = _build_osdf_result_demand(osdf_result["solution"][0][0])
    result[demand["name"]] = demand["value"]
    demand = _build_osdf_result_demand(osdf_result["solution"][0][1])
    result[demand["name"]] = demand["value"]
    return result


def _build_osdf_result_demand(solution):
    result = {}
    result["name"] = solution["resourceModuleName"]
    value = {"candidateId": solution["solution"]["identifiers"][0]}
    for info in solution["assignmentInfo"]:
        value[info["key"]] = info["value"]
    result["value"] = value
    return result


def _build_config_from_osdf(osdf_result):
    osdf_result = _adapt_osdf_result(osdf_result)
    v_pgn_result = _extract_osdf_appc_identifiers(osdf_result, 'vPGN')
    v_fw_result = _extract_osdf_appc_identifiers(osdf_result, 'vFW-SINK')
    dt_config = _extract_osdf_appc_dt_config(osdf_result, 'vFW-SINK')

    config = {
        'vPGN': v_pgn_result,
        'vFW-SINK': v_fw_result
    }
    #print(json.dumps(config, indent=4))
    config['dt-config'] = {
        'destinations': [dt_config]
    }
    return config


def _build_appc_lcm_dt_payload(is_vpkg, oof_config, book_name, traffic_presence):
    is_check = traffic_presence is not None
    oof_config = copy.deepcopy(oof_config)
    #if is_vpkg:
    #    node_list = "[ {} ]".format(oof_config['vPGN']['vserver-id'])
    #else:
    #    node_list = "[ {} ]".format(oof_config['vFW-SINK']['vserver-id'])

    if is_vpkg:
        config = oof_config['vPGN']
    else:
        config = oof_config['vFW-SINK']
    #node = {
    #    'site': config['physical-location-id'],
    #    'vnfc_type': config['vnfc-type'],
    #    'vm_info': [{
    #        'ne_id': config['vserver-name'],
    #        'fixed_ip_address': config['ip']
    #   }]
    #}
    #node_list = list()
    #node_list.append(node)

    if is_check:
        oof_config['dt-config']['trafficpresence'] = traffic_presence

    file_content = oof_config['dt-config']

    config = {
        "configuration-parameters": {
            #"node_list": node_list,
            "ne_id": config['vserver-name'],
            "fixed_ip_address": config['ip'],
            "file_parameter_content":  json.dumps(file_content)
        }
    }
    if book_name != '':
        config["configuration-parameters"]["book_name"] = book_name
    payload = json.dumps(config)
    return payload


def _build_appc_lcm_status_body(req):
    payload = {
        'request-id': req['input']['common-header']['request-id'],
        'sub-request-id': req['input']['common-header']['sub-request-id'],
        'originator-id': req['input']['common-header']['originator-id']
    }
    payload = json.dumps(payload)
    template = json.loads(open('templates/appcRestconfLcm.json').read())
    template['input']['action'] = 'ActionStatus'
    template['input']['payload'] = payload
    template['input']['common-header']['request-id'] = req['input']['common-header']['request-id']
    template['input']['common-header']['sub-request-id'] = str(uuid.uuid4())
    template['input']['action-identifiers']['vnf-id'] = req['input']['action-identifiers']['vnf-id']
    return template


def _build_appc_lcm_request_body(is_vpkg, config, req_id, action, traffic_presence=None):
    if is_vpkg:
        demand = 'vPGN'
    else:
        demand = 'vFW-SINK'

    book_name = "{}/latest/ansible/{}/site.yml".format(demand.lower(), action.lower())
    payload = _build_appc_lcm_dt_payload(is_vpkg, config, book_name, traffic_presence)
    template = json.loads(open('templates/appcRestconfLcm.json').read())
    template['input']['action'] = action
    template['input']['payload'] = payload
    template['input']['common-header']['request-id'] = req_id
    template['input']['common-header']['sub-request-id'] = str(uuid.uuid4())
    template['input']['action-identifiers']['vnf-id'] = config[demand]['vnf-id']
    return template


def _set_appc_lcm_timestamp(body, timestamp=None):
    if timestamp is None:
        t = datetime.utcnow() + timedelta(seconds=-10)
        timestamp = t.strftime('%Y-%m-%dT%H:%M:%S.244Z')
    body['input']['common-header']['timestamp'] = timestamp


def build_appc_lcms_requests_body(rancher_ip, onap_ip, aai_data, use_oof_cache, if_close_loop_vfw):
    if_has = False

    if if_has:
        migrate_from = _has_request(onap_ip, aai_data, False, use_oof_cache)

        if if_close_loop_vfw:
            migrate_to = migrate_from
        else:
            migrate_to = _has_request(onap_ip, aai_data, True, use_oof_cache)

        migrate_from = _build_config_from_has(migrate_from)
        migrate_to = _build_config_from_has(migrate_to)
    else:
        migrate_from = _osdf_request(rancher_ip, onap_ip, aai_data, False, use_oof_cache)

        if if_close_loop_vfw:
            migrate_to = migrate_from
        else:
            migrate_to = _osdf_request(rancher_ip, onap_ip, aai_data, True, use_oof_cache)

        migrate_from = _build_config_from_osdf(migrate_from)
        migrate_to = _build_config_from_osdf(migrate_to)

    #print(json.dumps(migrate_from, indent=4))
    #print(json.dumps(migrate_to, indent=4))
    req_id = str(uuid.uuid4())
    payload_dt_check_vpkg = _build_appc_lcm_request_body(True, migrate_from, req_id, 'DistributeTrafficCheck', True)
    payload_dt_vpkg_to = _build_appc_lcm_request_body(True, migrate_to, req_id, 'DistributeTraffic')
    payload_dt_check_vfw_from = _build_appc_lcm_request_body(False, migrate_from, req_id, 'DistributeTrafficCheck',
                                                             False)
    payload_dt_check_vfw_to = _build_appc_lcm_request_body(False, migrate_to, req_id, 'DistributeTrafficCheck', True)

    result = list()
    result.append(payload_dt_check_vpkg)
    result.append(payload_dt_vpkg_to)
    result.append(payload_dt_check_vfw_from)
    result.append(payload_dt_check_vfw_to)
    return result


def appc_lcm_request(onap_ip, req):
    api = _init_python_appc_lcm_api(onap_ip)
    #print(json.dumps(req, indent=4))
    if req['input']['action'] == "DistributeTraffic":
        result = api.lcm.distribute_traffic(body=req, params={}, headers={})
    elif req['input']['action'] == "DistributeTrafficCheck":
        result = api.lcm.distribute_traffic_check(body=req, params={}, headers={})
    else:
        raise Exception("{} action not supported".format(req['input']['action']))

    if result.body['output']['status']['code'] == 400:
        print("Request Completed")
    elif result.body['output']['status']['code'] == 100:
        print("Request Accepted. Receiving result status...")
#    elif result.body['output']['status']['code'] == 311:
#        timestamp = result.body['output']['common-header']['timestamp']
#        _set_appc_lcm_timestamp(req, timestamp)
#        appc_lcm_request(onap_ip, req)
#        return
    else:
        raise Exception("{} - {}".format(result.body['output']['status']['code'],
                                         result.body['output']['status']['message']))
    #print(result)
    return result.body['output']['status']['code']


def appc_lcm_status_request(onap_ip, req):
    api = _init_python_appc_lcm_api(onap_ip)
    status_body = _build_appc_lcm_status_body(req)
    _set_appc_lcm_timestamp(status_body)

    result = api.lcm.action_status(body=status_body, params={}, headers={})

    if result.body['output']['status']['code'] == 400:
        status = json.loads(result.body['output']['payload'])
        return status
    else:
        raise Exception("{} - {}".format(result.body['output']['status']['code'],
                                         result.body['output']['status']['message']))


def confirm_appc_lcm_action(onap_ip, req, check_appc_result):
    print("Checking LCM {} Status".format(req['input']['action']))

    while True:
        time.sleep(2)
        status = appc_lcm_status_request(onap_ip, req)
        print(status['status'])
        if status['status'] == 'SUCCESSFUL':
            return
        elif status['status'] == 'IN_PROGRESS':
            continue
        elif check_appc_result:
            raise Exception("LCM {} {} - {}".format(req['input']['action'], status['status'], status['status-reason']))
        else:
            return


def execute_workflow(vfw_vnf_id, rancher_ip, onap_ip, use_oof_cache, if_close_loop_vfw, info_only, check_result):
    print("\nExecuting workflow for VNF ID '{}' on Rancher with IP {} and ONAP with IP {}".format(
        vfw_vnf_id, rancher_ip, onap_ip))
    print("\nOOF Cache {}, is CL vFW {}, only info {}, check LCM result {}".format(use_oof_cache, if_close_loop_vfw,
                                                                                   info_only, check_result))
    x = threading.Thread(target=_run_osdf_resp_server, daemon=True)
    x.start()
    aai_data = load_aai_data(vfw_vnf_id, onap_ip)
    print("\nvFWDT Service Information:")
    print(json.dumps(aai_data, indent=4))
    lcm_requests = build_appc_lcms_requests_body(rancher_ip, onap_ip, aai_data, use_oof_cache, if_close_loop_vfw)
    print("\nAnsible Inventory:")
    inventory = "[host]\nlocalhost   ansible_connection=local\n"
    for key in ansible_inventory:
        inventory += str("[{}]\n").format(key)
        for host in ansible_inventory[key]:
            inventory += str("{}\n").format(ansible_inventory[key][host])

    print(inventory)
    f = open("Ansible_inventory", 'w+')
    f.write(inventory)
    f.close()

    if info_only:
        return
    print("\nDistribute Traffic Workflow Execution:")
    for i in range(len(lcm_requests)):
        req = lcm_requests[i]
        print("APPC REQ {} - {}".format(i, req['input']['action']))
        _set_appc_lcm_timestamp(req)
        result = appc_lcm_request(onap_ip, req)
        if result == 100:
            confirm_appc_lcm_action(onap_ip, req, check_result)
            #time.sleep(30)


#vnf_id, Rancher node IP, K8s node IP, use OOF cache, if close loop vfw, if info_only, if check APPC result
execute_workflow(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4].lower() == 'true', sys.argv[5].lower() == 'true',
                 sys.argv[6].lower() == 'true', sys.argv[7].lower() == 'true')
