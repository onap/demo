from jinja2 import Template
import json
import os
import requests
import sys

BASE_DIR = os.path.dirname(os.path.dirname(__file__)) + "/policies/"

HEADERS = {'Content-Type': 'application/json'}
AUTH = requests.auth.HTTPBasicAuth('healthcheck', 'zb!XztG34')


def get_tosca_policy(policy):
    pol = json.loads(policy)
    tosca_policy = {
            'tosca_definitions_version': 'tosca_simple_yaml_1_1_0',
            'topology_template': {
                    'policies': [pol]
                }
            }
    return json.dumps(tosca_policy)

def gen_policy(template_dir, gen_dir, filename, jinja_args):
    with open(os.path.join(template_dir, filename), 'r') as file:
        contents = file.read()
    tm = Template(contents)
    gen = tm.render(jinja_args)
    tosca_policy = get_tosca_policy(gen)
    with open(os.path.join(gen_dir, filename), 'w') as file:
        file.write(tosca_policy)

def create_and_push_policies(policy_dir):
    for filename in os.listdir(policy_dir):
        if filename.endswith('.json'):
            with open(os.path.join(policy_dir, filename), 'r') as file:
                data = json.loads(file.read())
                metadata = create_policy(data)
                if metadata:
                    push_policy(metadata)

def delete_policies(policy_dir):
    for filename in os.listdir(policy_dir):
        if filename.endswith('.json'):
            with open(os.path.join(policy_dir, filename), 'r') as file:
                data = json.loads(file.read())
                policy_id = list(data['topology_template']['policies'][0].keys())[0]
                undeploy_policy(policy_id)
                delete_policy(data)

def create_policy(data):
    policy = data['topology_template']['policies'][0]
    content = policy[list(policy.keys())[0]]
    policy_type = content['type']
    type_version = content['type_version']
    policy_url = "https://policy-api:6969"
    path = '/policy/api/v1/policytypes/{}/versions/{}/policies'.format(policy_type, type_version)
    url = policy_url + path
    try:
        response = requests.post(url, headers=HEADERS, auth=AUTH, data=json.dumps(data), verify=False)
    except Exception as e:
        print(str(e))
        return None
    if response.status_code == 200:
        print('Policy {} created'.format(content['metadata']['policy-id']))
        return content['metadata']
    else:
        print(response.content)
        return None

def push_policy(metadata):
    data = {'policies': [metadata]}
    policy_url = "https://policy-pap:6969"
    path = '/policy/pap/v1/pdps/policies'
    url = policy_url + path
    try:
        response = requests.post(url, headers=HEADERS, auth=AUTH, data=json.dumps(data), verify=False)
    except Exception as e:
        print(str(e))
        print("Cannot push policy {}".format(metadata['policy-id']))
    if response.status_code == 200:
        print("Policy {} pushed".format(metadata['policy-id']))
    else:
        print(response.content)

def undeploy_policy(policy_id):
    policy_url = "https://policy-pap:6969"
    path = '/policy/pap/v1/pdps/policies/{}'.format(policy_id)
    url = policy_url + path
    try:
        response = requests.delete(url, headers=HEADERS, auth=AUTH, verify=False)
    except Exception as e:
        print(str(e))
        print("Cannot undeploy policy {}".format(policy_id))
    if response.status_code == 200:
        print("Policy {} undeployed".format(policy_id))
    else:
        print(response.content)

def delete_policy(data):
    policy = data['topology_template']['policies'][0]
    content = policy[list(policy.keys())[0]]
    policy_type = content['type']
    type_version = content['type_version']
    policy_id = content['metadata']['policy-id']
    version = content['version']
    policy_url = "https://policy-api:6969"
    path = '/policy/api/v1/policytypes/{}/versions/{}/policies/{}/versions/{}'.format(policy_type, type_version, policy_id, version)
    url = policy_url + path
    try:
        response = requests.delete(url, headers=HEADERS, auth=AUTH, data=json.dumps(data), verify=False)
    except Exception as e:
        print(str(e))
        return None
    if response.status_code == 200:
        print('Policy {} deleted'.format(content['metadata']['policy-id']))
        return content['metadata']
    else:
        print(response.content)
        return None

def generate_nssi_policies(jinja_args):
    template_dir = BASE_DIR + 'nssi_policies'
    gen_dir = BASE_DIR + 'gen_nssi_policies'

    if not os.path.exists(gen_dir):
        os.mkdir(gen_dir)

    for filename in os.listdir(template_dir):
        if filename.endswith('.json'):
            gen_policy(template_dir, gen_dir, filename, jinja_args)

def generate_nsi_policies(jinja_args):
    template_dir = BASE_DIR + 'nsi_policies'
    gen_dir = BASE_DIR + 'gen_nsi_policies'

    if not os.path.exists(gen_dir):
        os.mkdir(gen_dir)

    for filename in os.listdir(template_dir):
        if filename.endswith('.json'):
            gen_policy(template_dir, gen_dir, filename, jinja_args)

def create_policy_types(policy_dir):
    for filename in os.listdir(policy_dir):
        if filename.endswith('.json'):
            with open(os.path.join(policy_dir, filename), 'r') as file:
                data = json.loads(file.read())
                create_policy_type(data)

def create_policy_type(data):
    policy_url = "https://policy-api:6969"
    path = '/policy/api/v1/policytypes'
    url = policy_url + path
    try:
        response = requests.post(url, headers=HEADERS, auth=AUTH, data=json.dumps(data), verify=False)
    except Exception as e:
        print(str(e))
        return None
    if response.status_code == 200:
        print('Policy type created')
    else:
        print(response.content)
        return None


action = sys.argv[1]

if action == "generate_nssi_policies":
    jinja_args = {
      'service_name': sys.argv[2],
      'goal': sys.argv[3],
      'attribute': sys.argv[4]
    }
    generate_nssi_policies(jinja_args)

elif action == "create_and_push_policies":
    policy_dir = sys.argv[2]
    create_and_push_policies(policy_dir)

elif action == "delete_policies":
    policy_dir = sys.argv[2]
    delete_policies(policy_dir)

elif action == "generate_nsi_policies":
    jinja_args = {
      'service_name': sys.argv[2]
    }
    generate_nsi_policies(jinja_args)

elif action == "create_policy_types":
    policy_dir = sys.argv[2]
    create_policy_types(policy_dir)
