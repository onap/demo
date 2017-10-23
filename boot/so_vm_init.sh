#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DMAAP_TOPIC=$(cat /opt/config/dmaap_topic.txt)
OPENSTACK_USERNAME=$(cat /opt/config/openstack_username.txt)
OPENSTACK_APIKEY=$(cat /opt/config/api_key.txt)
export MSO_DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)
export MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)
export OPENO_IP=$(cat /opt/config/openo_ip.txt)

# Deployments in OpenStack require a keystone file
if [ -e /opt/config/keystone.txt ]
then
	KEYSTONE_URL=$(cat /opt/config/keystone.txt)
	OPENSTACK_REGION=$(cat /opt/config/openstack_region.txt)
	DCP_CLLI="DEFAULT_KEYSTONE"
	AUTH_TYPE="USERNAME_PASSWORD"
	read -d '' CLOUD_SITES <<-EOF
        "cloud_sites": [{
                 "aic_version": "2.5",
                 "id": "$OPENSTACK_REGION",
                 "identity_service_id": "$DCP_CLLI",
                 "lcp_clli": "$OPENSTACK_REGION",
                 "region_id": "$OPENSTACK_REGION"
        }],
EOF
else
	KEYSTONE_URL="https://identity.api.rackspacecloud.com/v2.0"
	DCP_CLLI="RAX_KEYSTONE"
	AUTH_TYPE="RACKSPACE_APIKEY"
	read -d '' CLOUD_SITES <<-EOF
        "cloud_sites": [
               {
                 "id": "Dallas",
                 "aic_version": "2.5",
                 "lcp_clli": "DFW",
                 "region_id": "DFW",
                 "identity_service_id": "$DCP_CLLI"
               },

               {
                 "id": "Northern Virginia",
                 "aic_version": "2.5",
                 "lcp_clli": "IAD",
                 "region_id": "IAD",
                 "identity_service_id": "$DCP_CLLI"
               },

               {
                 "id": "Chicago",
                 "aic_version": "2.5",
                 "lcp_clli": "ORD",
                 "region_id": "ORD",
                 "identity_service_id": "$DCP_CLLI"
               }
         ],
EOF
fi

# Update the SO configuration file.
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
		$CLOUD_SITES
	    "identity_services": 
	        [
	            {"dcp_clli": "$DCP_CLLI", 
	            "identity_url": "$KEYSTONE_URL/v2.0",
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
#This script takes in input 2 nexus repos (the first one for the SO image, the second one for mariadb)
./deploy.sh $NEXUS_DOCKER_REPO $NEXUS_USERNAME $NEXUS_PASSWD $NEXUS_DOCKER_REPO $NEXUS_USERNAME $NEXUS_PASSWD
