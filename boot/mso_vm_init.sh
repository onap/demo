#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DMAAP_TOPIC=$(cat /opt/config/dmaap_topic.txt)
OPENSTACK_USERNAME=$(cat /opt/config/openstack_username.txt)
OPENSTACK_APIKEY=$(cat /opt/config/api_key.txt)
export MSO_DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)

# Deployments in OpenStack require a keystone file
if [ -e /opt/config/keystone.txt ]
then
	KEYSTONE_URL=$(cat /opt/config/keystone.txt)
	DCP_CLLI="DEFAULT_KEYSTONE"
	AUTH_TYPE="USERNAME_PASSWORD"
else
	KEYSTONE_URL="https://identity.api.rackspacecloud.com/v2.0"
	DCP_CLLI="RAX_KEYSTONE"
	AUTH_TYPE="RACKSPACE_APIKEY"
fi

# Update the MSO configuration file.
read -d '' MSO_CONFIG_UPDATES <<-EOF
{
"default_attributes":
  {
    "asdc-connections":
      {
	    "asdc-controller1":
	    {
	        "environmentName": "$DMAAP_TOPIC"
	    }
      },
      "mso-po-adapter-config": 
	  {
	    "identity_services": 
	        [
	            {"dcp_clli": "$DCP_CLLI", 
	             "identity_url": "$KEYSTONE_URL",
	             "mso_id": "$OPENSTACK_USERNAME", 
	             "mso_pass": "$OPENSTACK_APIKEY", 
	             "admin_tenant": "service", 
	             "member_role": "admin", 
	             "tenant_metadata": "true",
	             "identity_server_type": "KEYSTONE", 
	             "identity_authentication_type": "$AUTH_TYPE"
	                
	            }
	        ]
	  }
  }
}
EOF
export MSO_CONFIG_UPDATES


# Deploy the environment
cd /opt/test_lab
git pull
chmod +x deploy.sh
#This script takes in input 2 nexus repos (the first one for the MSO image, the second one for mariadb)
./deploy.sh $NEXUS_DOCKER_REPO $NEXUS_USERNAME $NEXUS_PASSWD $NEXUS_DOCKER_REPO $NEXUS_USERNAME $NEXUS_PASSWD
