#!/usr/bin/python

import argparse
#import getopt
import json
import sys
import urllib2
import ConfigParser
import time
from fabric.context_managers import settings
from fabric.api import *
from fabfile import *


class ReadConfFile:

    config = None

    def __init__(self, file="onap_docker_upgrade.conf"):

        """
        Method to read from conf file specific options

        :param file:
        """
        self.config = ConfigParser.SafeConfigParser()
        self.config.readfp(open(file))

    def read_option(self, group, name):
        """
        :return:
        """
        value = self.config.get(group, name)
        return value


def getToken(url, osuser, ostenant, ospassword):

    """ 
    Returns a token to the user given a tenant, 
    user name, password, and OpenStack API URL. 
    """
    url = url + '/tokens'
    tokenRequest = urllib2.Request(url)
    tokenRequest.add_header("Content-type", "application/json")
    jsonPayload = json.dumps({'auth' : {'tenantName' : ostenant, 'passwordCredentials' : {'username' : osuser, 'password' : ospassword}}})
    
    request = urllib2.urlopen(tokenRequest, jsonPayload)
    json_data = json.loads(request.read())
    
    request.close()
    return json.dumps(json_data)

def getServers(url, token):
    """
    Returns a list of server in a given tenant
    """
    url = url + '/servers'
    #handler=urllib2.HTTPHandler(debuglevel=1)
    #opener = urllib2.build_opener(handler)
    #urllib2.install_opener(opener)
    nova_server_request = urllib2.Request(url)
    nova_server_request.add_header("Content-type", "application/json")
    nova_server_request.add_header("X-Auth-Token", token)
    request = urllib2.urlopen(nova_server_request)
    json_data = json.loads(request.read())
    
    request.close()
    return json.dumps(json_data)

def getServerInfo(url, token, server):
    """
    Returns the server information in a given tenant
    """
    url = url + '/servers/' + server
    #handler=urllib2.HTTPHandler(debuglevel=1)
    #opener = urllib2.build_opener(handler)
    #urllib2.install_opener(opener)
    nova_server_request = urllib2.Request(url)
    nova_server_request.add_header("Content-type", "application/json")
    nova_server_request.add_header("X-Auth-Token", token)
    request = urllib2.urlopen(nova_server_request)
    json_data = json.loads(request.read())

    request.close()
    return json.dumps(json_data)

conf_file = ReadConfFile()

#Reading keystone_auth
url=conf_file.read_option('keystone_auth', 'url')
user=conf_file.read_option('keystone_auth', 'user')
password=conf_file.read_option('keystone_auth', 'password')
tenant=conf_file.read_option('keystone_auth', 'tenant')

#Reading onap
instance_prefix=conf_file.read_option('onap', 'instance_prefix')
deployment_type=conf_file.read_option('onap', 'deployment_type')
if deployment_type in ("1-nic-float" or "2-nic"):
	onap_net_name=conf_file.read_option('onap', 'onap_net_name')
dcae_key_path=conf_file.read_option('onap', 'dcae_key_path')
onap_key_path=conf_file.read_option('onap', 'onap_key_path')

#Reading nova
nova_url=conf_file.read_option('nova', 'url')
    
# Since we return a raw JSON payload from getToken,
# we need to load it into a readable object.
adminToken = json.loads(getToken(url, user, tenant, password))

# Access the token portion of the JSON payload and grab the token and tenant ID
adminTokenID = adminToken['access']['token']['id']
adminTokenTenantID = adminToken['access']['token']['tenant']['id']

for item in adminToken['access']['serviceCatalog']:
    """ 
    The "name" of each OpenStack service catalog item from
    the item list changed between versions.  Things like 
    "glance" became "volume" and "keystone" became "identity".  
    You will need to update this based on your installation.
    """
    if nova_url:
	adminNovaURL = nova_url + "/" + adminTokenTenantID
    elif item['name'] == "nova": 
        adminNovaURL = item['endpoints'][0]['adminURL']

print "------ Admin information ------"
print "Token ID = ", adminTokenID
print "Tenant ID = ", adminTokenTenantID
print "Nova URL = ", adminNovaURL
print "------ End Admin info ---------"

get_nova_servers = json.loads(getServers(adminNovaURL, adminTokenID))
#print get_nova_servers
#print get_nova_servers['servers'][0]['id']
execute_aai1 = False
for item in get_nova_servers['servers']:
	#print item['id'], item['name']
	if instance_prefix in item['name']:
		print "Found matching server name: %s with id %s" % (item['name'], item['id'])
		get_server_info = json.loads(getServerInfo(adminNovaURL, adminTokenID, item['id']))
		for net_info in get_server_info['server']['addresses']:
			if deployment_type in ("1-nic-float" or "2-nic"):
 				if onap_net_name not in net_info:
					server_ip = get_server_info['server']['addresses'][net_info][0]['addr']
			elif deployment_type in "1-nic-nofloat":
				server_ip = get_server_info['server']['addresses'][net_info][1]['addr']
			server_name = item['name']
			if "robot" in server_name:
				robot_ip = server_ip
  			elif "aai-inst1" in server_name:
				aai1_ip = server_ip
				is_aai2_executed = False
				if not is_aai2_executed:
					execute_aai1 = False
					aai1_server_name = server_name
				else:
					execute_aai1 = True			
			elif "aai-inst2" in server_name:
				aai2_ip = server_ip
				is_aai2_executed = True
			print "IP address of vm %s is %s" % (item['name'], server_ip)

			with settings(warnings=True, skip_bad_hosts=True):
				if "dcae-controller" in server_name:
					execute(upgrade_docker, server_name, hosts=server_ip, my_env={'key_filename':dcae_key_path, 'user':'ubuntu'})
				else:
					if "aai-inst1" not in server_name:
						execute(upgrade_docker, server_name, hosts=server_ip, my_env={'key_filename':onap_key_path, 'user':'ubuntu'})
					elif execute_aai1:
						execute(upgrade_docker, server_name, hosts=server_ip, my_env={'key_filename':onap_key_path, 'user':'ubuntu'})
					else:
						print "Skipping %s upgrade until aai2 finishes upgrade" % server_name
			
with settings(warnings=True):
	if not execute_aai1:
		print "Starting %s upgrade" % aai1_ip
		execute(upgrade_docker, aai1_server_name, hosts=aai1_ip, my_env={'key_filename':onap_key_path, 'user':'ubuntu'})
  	time.sleep(300)
	execute(health_checks_robot, hosts=robot_ip, my_env={'key_filename':onap_key_path, 'user':'ubuntu'})
