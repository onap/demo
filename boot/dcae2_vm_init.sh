#!/bin/bash
#############################################################################
#
# Copyright (c) 2017 AT&T Intellectual Property. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#        http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#############################################################################


# prepare a curl command
# parameters: URL METHOD CURLOPTIONS EXTRA_HEADERS_AS_A_STRING AUTH_AS_USER:PASS DATA
assemble_curl_command()
{
    export http_proxy=""
    export https_proxy=""
	
    local URL="$1"
    local METHOD="$2"
    local CURLOPTIONS="$3"
    local EXTRA_HEADERS="$4"
    local AUTH="$5"
    local DATA="$6"
    local CMD=''
    if [ ! -z "$METHOD" ]; then
        CMD="curl $CURLOPTIONS $METHOD"
    else
        CMD="curl $CURLOPTIONS -X GET"
    fi
    if [ ! -z "$EXTRA_HEADERS" ]; then
        CMD="$CMD $EXTRA_HEADERS"
    fi
    if [ ! -z "$AUTH" ]; then
        CMD="$CMD $AUTH"
    fi
    if [ ! -z "$DATA" ]; then
        CMD="$CMD $DATA"
    fi
    CMD="$CMD $URL"
    echo "$CMD"
}


# Make a rest API call
# parameters: URL METHOD expected_response_code EXTRA_HEADERS_AS_A_STRING AUTH_AS_USER:PASS DATA
call_api_for_response_code()
{
    export http_proxy=""
    export https_proxy=""
	
    local CURLOPTIONS='-kIso /dev/null -w "%{http_code}"'
    read -r CMDF <<-END
$(assemble_curl_command "$1" "$2" "$CURLOPTIONS" "$4" "$5" "$6")
END
    eval "$CMDF";
}
call_api_for_response_body()
{
    export http_proxy=""
    export https_proxy=""
	
    local CURLOPTIONS='-ksb'
    read -r CMDF <<-END
$(assemble_curl_command "$1" "$2" "$CURLOPTIONS" "$4" "$5" "$6")
END
    eval "$CMDF"
}
call_api_for_response_header()
{
    export http_proxy=""
    export https_proxy=""
	
    local CURLOPTIONS='-ks -o /dev/null -D -'
    read -r CMDF <<-END
$(assemble_curl_command "$1" "$2" "$CURLOPTIONS" "$4" "$5" "$6")
END
    eval "$CMDF"
}
call_api_for_verbose()
{
    export http_proxy=""
    export https_proxy=""
	
    local CURLOPTIONS='-kIv'
    read -r CMDF <<-END
$(assemble_curl_command "$1" "$2" "$CURLOPTIONS" "$4" "$5" "$6")
END
    eval "$CMDF"
    #local TFILE=$(mktemp /tmp/curlcmd.XXXXXXXXX)
    #echo $CMD > $TFILE
    #eval $(cat $TFILE)
    #rm -f $TFILE
}


# Wait till a web service API return specified response code
# parameters: URL METHOD EXPECTED_RESP_CODE EXTRA_HEADERS_AS_A_STRING AUTH_AS_USER:PASS DATA
wait_for_api()
{
    export http_proxy=""
    export https_proxy=""
	
    local RESP="$3" 
    local ACTUALRESP
    ACTUALRESP=$(call_api_for_response_code "$1" "$2" "$3" "$4" "$5" "$6")
    while [ "$ACTUALRESP" != "$RESP" ]; do
        echo "RESP CODE $ACTUALRESP, not as expected RESP CODE $RESP @ $(date)."
        sleep 30
        ACTUALRESP=$(call_api_for_response_code "$1" "$2" "$3" "$4" "$5" "$6")
    done
    echo "RESP CODE $ACTUALRESP, matches with expected RESP CODE $RESP."
}

# Wait till a TCP port is open
# parameters: HOST PORT
wait_for_tcp_port()
{
    export http_proxy=""
    export https_proxy=""
	
    local DEST="$1"
    local PORT="$2"
    while ! nc -z -w 1 "$DEST" "$PORT"; do
        sleep 4
        echo '.'
    done
}




wait_for_aai_ready()
{
    export http_proxy=""
    export https_proxy=""
	
    # wait till A&AI up and ready
    local AAIHOST
    AAIHOST=$(cat /opt/config/aai1_ip_addr.txt)
    local AAIURL="https://$AAIHOST:8443/aai/v11/examples/cloud-regions"
    local AAIMETHOD='-X GET'
    local AAIRESP='200'
    local AAIHEADERS='-H "X-FromAppId: AAI-Temp-Tool" -H "X-TransactionId: AAI-Temp-Tool" -H "Real-Time: true" -H "Content-Type: application/json" -H "Accept: application/json"'
    local AAIAUTH='-u AAI:AAI'
    local AAIDATA=''
    echo "===> Waiting for A&AI to get ready for getting $AAIRESP from $AAIURL @ $(date)"
    wait_for_api "$AAIURL" "$AAIMETHOD" "$AAIRESP" "$AAIHEADERS" "$AAIAUTH" "$AAIDATA"
    echo "===> A&AI ready @ $(date)"
}



wait_for_multicloud_ready()
{
    export http_proxy=""
    export https_proxy=""
	
    # wait till MultiCloud up and ready
    local MCHOST
    MCHOST=$(cat /opt/config/multiserv_ip_addr.txt)
    local MCURL="http://$MCHOST:9005/api/multicloud-titaniumcloud/v0/swagger.json"
    local MCMETHOD='-X GET'
    local MCRESP='200'
    local MCHEADERS='-H "Real-Time: true" -H "Content-Type: application/json" -H "Accept: application/json"'
    local MCAUTH=''
    local MCDATA=''
    echo "===> Waiting for MultiCloud to get ready for getting $MCRESP from $MCURL @ $(date)"
    wait_for_api "$MCURL" "$MCMETHOD" "$MCRESP" "$MCHEADERS" "$MCAUTH" "$MCDATA"
    echo "===> MultiCloud ready @ $(date)"
}

register_multicloud_pod25dns_with_aai()
{
    export http_proxy=""
    export https_proxy=""
	
    # Register MultiCloud with A&AI
    local CLOUD_OWNER='pod25dns'
    local CLOUD_VERSION='titanium_cloud'
    local CLOUD_REGION
    local CLOUD_ENV
    local CLOUD_IDENTITY_URL
    local DNSAAS_SERVICE_URL
    local DNSAAS_USERNAME='demo'
    local DNSAAS_PASSWORD='onapdemo'

    CLOUD_REGION="$(cat /opt/config/dnsaas_region.txt)"
    CLOUD_ENV="$(cat /opt/config/cloud_env.txt)"
    MCIP="$(cat /opt/config/multiserv_ip_addr.txt)"
    CLOUD_IDENTITY_URL="http://${MCIP}/api/multicloud-titaniumcloud/v0/${CLOUD_OWNER}_${CLOUD_REGION}/identity/v2.0"

    local RESPCODE
    DNSAAS_SERVICE_URL="$(cat /opt/config/dnsaas_keystone_url.txt)"
    # a tenant of the same name must be set up on the Deisgnate providing OpenStack
    DNSAAS_TENANT_NAME="$(cat /opt/config/dnsaas_tenant_name.txt)"
    cat >"/tmp/${CLOUD_OWNER}_${CLOUD_REGION}.json" <<EOL
{
    "cloud-owner" : "$CLOUD_OWNER",
    "cloud-region-id" : "$CLOUD_REGION",
    "cloud-region-version" : "$CLOUD_VERSION",
    "cloud-type" : "$CLOUD_ENV",
    "cloud-zone" : "cloud zone",
    "complex-name" : "complex name2",
    "identity-url": "$CLOUD_IDENTITY_URL",
    "owner-defined-type" : "owner-defined-type",
    "sriov-automation" : false,
    "esr-system-info-list" : {
        "esr-system-info" : [
            {
                "esr-system-info-id": "532ac032-e996-41f2-84ed-9c7a1766eb30",
                "cloud-domain": "Default",
                "default-tenant" : "$DNSAAS_TENANT_NAME",
                "user-name" : "$DNSAAS_USERNAME",
                "password" : "$DNSAAS_PASSWORD",
                "service-url" : "$DNSAAS_SERVICE_URL",
                "ssl-cacert": "example-ssl-cacert-val-75021",
                "ssl-insecure": true,
                "system-name": "example-system-name-val-29071",
                "system-type": "VIM",
                "ip-address": "example-ip-address-val-44432",
                "port": "example-port-val-93235",
                "type": "example-type-val-85255",
                "protocal": "example-protocal-val-52954",
                "vendor": "example-vendor-val-94515",
                "version": "example-version-val-71880"
            }
        ]
    }
}
EOL


    local REGHOST
    local REGURL
    local REGMETHOD='-X PUT'
    local REGHEADERS='-H "X-FromAppId: AAI-Temp-Tool" -H "X-TransactionId: AAI-Temp-Tool" -H "Real-Time: true" -H "Content-Type: application/json" -H "Accept: application/json"'
    local REGRESP='201'
    local REGAUTH='-u AAI:AAI'
    local REGDATA

    REGHOST="$(cat /opt/config/aai1_ip_addr.txt)"
    REGURL="https://$REGHOST:8443/aai/v11/cloud-infrastructure/cloud-regions/cloud-region/$CLOUD_OWNER/$CLOUD_REGION"
    REGDATA="-T /tmp/${CLOUD_OWNER}_${CLOUD_REGION}.json"
    echo "Register MultiCloud with A&AI owner $CLOUD_OWNER"
    RESP=$(call_api_for_response_code "$REGURL" "$REGMETHOD" "$REGRESP" "$REGHEADERS" "$REGAUTH" "$REGDATA")
    echo "RESP CODE: $RESP"
}


register_multicloud_pod25_with_aai()
{
    export http_proxy=""
    export https_proxy=""
	
    # Register MultiCloud with A&AI
    local CLOUD_OWNER='pod25'
    local CLOUD_VERSION='titanium_cloud'
    local CLOUD_REGION
    local DNSAAS_CLOUD_REGION
    local CLOUD_ENV
    local MCIP
    local CLOUD_IDENTITY_URL
    local KEYSTONE_URL
    local USERNAME
    local PASSWORD
    local TENANT_NAME

    CLOUD_REGION="$(cat /opt/config/openstack_region.txt)"
    DNSAAS_CLOUD_REGION="$(cat /opt/config/dnsaas_region.txt)"
    CLOUD_ENV="$(cat /opt/config/cloud_env.txt)"
    MCIP="$(cat /opt/config/multiserv_ip_addr.txt)"
    CLOUD_IDENTITY_URL="http://${MCIP}/api/multicloud-titaniumcloud/v0/${CLOUD_OWNER}_${CLOUD_REGION}/identity/v2.0"
    KEYSTONE_URL="$(cat /opt/config/openstack_keystone_url.txt)"
    if [[ "$KEYSTONE_URL" == */v3 ]]; then
        echo "$KEYSTONE_URL"
    elif [[ "$KEYSTONE_URL" == */v2.0 ]]; then
        echo "$KEYSTONE_URL"
    else
        KEYSTONE_URL="${KEYSTONE_URL}/v3"
        echo "$KEYSTONE_URL"
    fi
    USERNAME="$(cat /opt/config/openstack_user.txt)"
    PASSWORD="$(cat /opt/config/openstack_password.txt)"
    TENANT_NAME="$(cat /opt/config/tenant_name.txt)"
    cat >"/tmp/${CLOUD_OWNER}_${CLOUD_REGION}.json" <<EOL
{
    "cloud-owner" : "$CLOUD_OWNER",
    "cloud-region-id" : "$CLOUD_REGION",
    "cloud-region-version" : "$CLOUD_VERSION",
    "cloud-type" : "$CLOUD_ENV",
    "cloud-zone" : "cloud zone",
    "complex-name" : "complex name",
    "identity-url": "$CLOUD_IDENTITY_URL",
    "owner-defined-type" : "owner-defined-type",
    "sriov-automation" : false,
    "cloud-extra-info" : "{\"epa-caps\":{\"huge_page\":\"true\",\"cpu_pinning\":\"true\",\"cpu_thread_policy\":\"true\",\"numa_aware\":\"true\",\"sriov\":\"true\",\"dpdk_vswitch\":\"true\",\"rdt\":\"false\",\"numa_locality_pci\":\"true\"},\"dns-delegate\":{\"cloud-owner\":\"pod25dns\",\"cloud-region-id\":\"${DNSAAS_CLOUD_REGION}\"}}",
    "esr-system-info-list" : {
        "esr-system-info" : [
            {
                "esr-system-info-id": "432ac032-e996-41f2-84ed-9c7a1766eb29",
                "cloud-domain": "Default",
                "default-tenant" : "$TENANT_NAME",
                "user-name" : "$USERNAME",
                "password" : "$PASSWORD",
                "service-url" : "$KEYSTONE_URL",
                "ssl-cacert": "example-ssl-cacert-val-75021",
                "ssl-insecure": true,
                "system-name": "example-system-name-val-29070",
                "system-type": "VIM",
                "ip-address": "example-ip-address-val-44431",
                "port": "example-port-val-93234",
                "type": "example-type-val-85254",
                "protocal": "example-protocal-val-52954",
                "vendor": "example-vendor-val-94515",
                "version": "example-version-val-71880"
            }
        ]
    }
}
EOL
 
    local REGHOST
    local REGURL
    local REGMETHOD='-X PUT'
    local REGHEADERS='-H "X-FromAppId: AAI-Temp-Tool" -H "X-TransactionId: AAI-Temp-Tool" -H "Real-Time: true" -H "Content-Type: application/json" -H "Accept: application/json"'
    local REGRESP='201'
    local REGAUTH='-u AAI:AAI'
    local REGDATA
 
    REGHOST="$(cat /opt/config/aai1_ip_addr.txt)"
    REGURL="https://$REGHOST:8443/aai/v11/cloud-infrastructure/cloud-regions/cloud-region/$CLOUD_OWNER/$CLOUD_REGION"
    REGDATA="-T /tmp/${CLOUD_OWNER}_${CLOUD_REGION}.json"
    echo "Register MultiCloud with A&AI owner $CLOUD_OWNER"
    RESP=$(call_api_for_response_code "$REGURL" "$REGMETHOD" "$REGRESP" "$REGHEADERS" "$REGAUTH" "$REGDATA")
    echo "RESP CODE: $RESP"
}



verify_multicloud_registration() 
{
    export http_proxy=""
    export https_proxy=""
	
    local CLOUD_OWNER='pod25'
    local CLOUD_REGION
    local CLOUD_VERSION='titanium_cloud'
    local CLOUD_ENV
    local REGHOST
    local REGURL
    local REGMETHOD='-X GET'
    local REGHEADERS='-H "X-FromAppId: AAI-Temp-Tool" -H "X-TransactionId: AAI-Temp-Tool" -H "Real-Time: true" -H "Content-Type: application/json" -H "Accept: application/json"'
    local REGRESP='200'
    local REGAUTH='-u AAI:AAI'
    local REGDATA=''
    local RESPCODE

    CLOUD_REGION="$(cat /opt/config/openstack_region.txt)"
    CLOUD_ENV="$(cat /opt/config/cloud_env.txt)"
    REGHOST="$(cat /opt/config/aai1_ip_addr.txt)"

    REGURL="https://$REGHOST:8443/aai/v11/cloud-infrastructure/cloud-regions/cloud-region/${CLOUD_OWNER}/${CLOUD_REGION}?depth=all"
    # Verify MultiCloud with A&AI
    RESPCODE=$(call_api_for_response_code "$REGURL" "$REGMETHOD" "$REGRESP" "$REGHEADERS" "$REGAUTH" "$REGDATA")
    echo "Register MultiCloud with A&AI owner $CLOUD_OWNER verify response code: $RESPCODE"

    CLOUD_OWNER='pod25dns'
    REGURL="https://$REGHOST:8443/aai/v11/cloud-infrastructure/cloud-regions/cloud-region/${CLOUD_OWNER}/${CLOUD_REGION}?depth=all"
    RESPCODE=$(call_api_for_response_code "$REGURL" "$REGMETHOD" "$REGRESP" "$REGHEADERS" "$REGAUTH" "$REGDATA")
    echo "Register MultiCloud with A&AI owner $CLOUD_OWNER verify response code: $RESPCODE"
}


register_dns_zone_proxied_designate()
{
    export http_proxy=""
    export https_proxy=""
	
    local CLOUD_OWNER='pod25' 
    local CLOUD_REGION
    local CLOUD_VERSION='titanium_cloud'
    local CLOUD_ENV
    local DNSAAS_TENANT_NAME
    local MCHOST
    local MCURL
    local MCMETHOD='-X POST'
    local MCRESP='200'
    local MCHEADERS='-H "Content-Type: application/json" -H "Accept: application/json"'
    local MCAUTH=''
    local MCDATA=''

    ## export endpoint prefix
    local MULTICLOUD_PLUGIN_ENDPOINT

    CLOUD_REGION="$(cat /opt/config/openstack_region.txt)"
    CLOUD_ENV="$(cat /opt/config/cloud_env.txt)"
    if [ -z "$1" ]; then DCAE_ZONE="$(cat /opt/config/dcae_zone.txt)"; else DCAE_ZONE="$1"; fi
    DNSAAS_TENANT_NAME="$(cat /opt/config/dnsaas_tenant_name.txt)"
    MCHOST=$(cat /opt/config/multiserv_ip_addr.txt)
    MCURL="http://$MCHOST:9005/api/multicloud-titaniumcloud/v0/swagger.json"

    MCDATA='-d "{\"auth\":{\"tenantName\": \"'${DNSAAS_TENANT_NAME}'\"}}"'
    MULTICLOUD_PLUGIN_ENDPOINT=http://${MCHOST}/api/multicloud-titaniumcloud/v0/${CLOUD_OWNER}_${CLOUD_REGION}

     ### zone operations
     # because all VM's use 10.0.100.1 as their first DNS server, the designate DNS server as seocnd, we need to use a
     # domain outside of the first DNS server's domain
    local DCAE_DOMAIN
    local ZONENAME
    DCAE_DOMAIN="$(cat /opt/config/dcae_domain.txt)"
    ZONENAME="${DCAE_ZONE}.${DCAE_DOMAIN}."

    echo "===> Register DNS zone $ZONENAME under $DNSAAS_TENANT_NAME"


    ### Get Token
    local TOKEN

    MCURL="${MULTICLOUD_PLUGIN_ENDPOINT}/identity/v3/auth/tokens"
    echo "=====> Getting token from $MCURL"
    #TOKEN=$(call_api_for_response_header "$MCURL" "$MCMETHOD" "$MCRESP" "$MCHEADERS" "$MCAUTH" "$MCDATA" | grep 'X-Subject-Token' | sed "s/^.*: //")
    TOKEN=$(curl -v -s -H "Content-Type: application/json" -X POST -d "{\"auth\":{\"tenantName\": \"${DNSAAS_TENANT_NAME}\"}}" "${MCURL}" 2>&1 | grep X-Subject-Token | sed "s/^.*: //")
    echo "Received Keystone token $TOKEN from $MCURL"
    if [ -z "$TOKEN" ]; then
        echo "Faile to acquire token for creating DNS zone.  Exit"
        exit 1
    fi

    local PROJECTID
    PROJECTID=$(curl -v -s  -H "Content-Type: application/json" -H "X-Auth-Token: $TOKEN" -X GET "${MULTICLOUD_PLUGIN_ENDPOINT}/dns-delegate/v2/zones?name=${ZONENAME}" |grep 'project_id' |sed 's/^.*"project_id":"\([a-zA-Z0-9-]*\)",.*$/\1/')
    if [ ! -z "$PROJECTID" ]; then 
        ### query the zone with zone id
        echo "!!!!!!> zone $ZONENAME already registered by project $PROJECTID"
    else
        ### create a zone
        echo "=====> No zone of same name $ZONENAME found, creating new zone "
        curl -sv -H "Content-Type: application/json" -H "X-Auth-Token: $TOKEN" -X POST -d "{ \"name\": \"$ZONENAME\", \"email\": \"lji@research.att.com\"}" "${MULTICLOUD_PLUGIN_ENDPOINT}/dns-delegate/v2/zones"
    fi

    ### list zones
    echo "=====> Zone listing"
    curl -sv -H "Content-Type: application/json" -H "X-Auth-Token: $TOKEN" -X GET "${MULTICLOUD_PLUGIN_ENDPOINT}/dns-delegate/v2/zones" | python -m json.tool

    ### query the zone with zone name
    #echo "=====> Querying zone $ZONENAME"
    #curl -s -H "Content-Type: application/json" -H "X-Auth-Token: $TOKEN" -X GET "${MULTICLOUD_PLUGIN_ENDPOINT}/dns-delegate/v2/zones?name=${ZONENAME}"

    ### export ZONE id
    local ZONEID
    ZONEID=$(curl -v -sb  -H "Content-Type: application/json" -H "X-Auth-Token: $TOKEN" -X GET "${MULTICLOUD_PLUGIN_ENDPOINT}/dns-delegate/v2/zones?name=${ZONENAME}" |grep 'id' |sed 's/^.*"id":"\([a-zA-Z0-9-]*\)",.*$/\1/')
    echo "=====> After creation, zone $ZONENAME ID is $ZONEID"

    ### query the zone with zone id
    #echo "=====> Querying zone $ZONENAME by ID $ZONEID"
    #curl -sv -H "Content-Type: application/json" -H "X-Auth-Token: $TOKEN" -X GET "${MULTICLOUD_PLUGIN_ENDPOINT}/dns-delegate/v2/zones/${ZONEID}"
}


register_dns_zone_designate()
{
    export http_proxy=""
    export https_proxy=""
	
    local HEADER_CONTENT_TYPE_JSON="Content-Type: application/json"
    local HEADER_ACCEPT_JSON="Accept: application/json"
    local HEADER_TOKEN
    local DCAE_ZONE
    local DCAE_DOMAIN
    local ZONE_NAME
    local ZONE_ID
    local KEYSTONE_URL
    local API_ENDPOINT
    local API_DATA
    local TENANT_NAME
    local TENANT_ID
    local ZONE_PROJECT_ID
    
    if [ -z "$1" ]; then DCAE_ZONE="$(cat /opt/config/dcae_zone.txt)"; else DCAE_ZONE="$1"; fi
    DCAE_DOMAIN="$(cat /opt/config/dcae_domain.txt)"
    ZONE_NAME="${DCAE_ZONE}.${DCAE_DOMAIN}."

    TENANT_NAME="$(cat /opt/config/tenant_name.txt)"
    TENANT_ID="$(cat /opt/config/tenant_id.txt)"

    KEYSTONE_URL="$(cat /opt/config/openstack_keystone_url.txt)"
    if [[ "$KEYSTONE_URL" == */v3 ]]; then
        echo "$KEYSTONE_URL"
    elif [[ "$KEYSTONE_URL" == */v2.0 ]]; then
        echo "$KEYSTONE_URL"
    else
        KEYSTONE_URL="${KEYSTONE_URL}/v2.0"
        echo "$KEYSTONE_URL"
    fi

    USERNAME="$(cat /opt/config/openstack_user.txt)"
    PASSWORD="$(cat /opt/config/openstack_password.txt)"


    API_ENDPOINT="${KEYSTONE_URL}/tokens"
    API_DATA="{\"auth\": {\"project\": \"${TENANT_NAME}\", \"tenantId\": \"${TENANT_ID}\", \"passwordCredentials\": {\"username\": \"${USERNAME}\", \"password\": \"${PASSWORD}\"}}}"
    
    echo "===> Getting token from ${API_ENDPOINT}"
    RESP=$(curl -s -v -H "${HEADER_CONTENT_TYPE_JSON}" -d "${API_DATA}" "${API_ENDPOINT}")

    TOKEN="$(echo ${RESP} | jq -r .access.token.id)"
    if [ -z "$TOKEN" ]; then
        echo "Faile to acquire token for creating DNS zone.  Exit"
        exit 1
    fi
    HEADER_TOKEN="X-Auth-Token: ${TOKEN}" 
 

    DESIGNATE_URL=$(echo ${RESP} | jq -r '.access.serviceCatalog[] | select(.name=="designate") | .endpoints[0].publicURL')
    if [ -z ${DESIGNATE_URL} ]; then
        echo "Fail to find Designate API endpoint.  Exit"
        exit 1
    fi


    API_ENDPOINT="${DESIGNATE_URL}/v2/zones"
    echo "===> Register DNS zone $ZONE_NAME at Designate API endpoint ${API_ENDPOINT}"
   
    RESP=$(curl -v -s -H "$HEADER_TOKEN" "$API_ENDPOINT")
    ZONE_ID=$(echo $RESP |jq -r --arg ZONE_NAME "$ZONE_NAME" '.zones[] |select(.name==$ZONE_NAME) |.id')
    if [ -z "$ZONE_ID" ]; then
        echo "======> Zone ${ZONE_NAME} does not exist.  Create"
        API_DATA="{\"name\": \"${ZONE_NAME}\", \"email\": \"dcae@onap.org\", \"type\": \"PRIMARY\", \"ttl\": 7200, \"description\": \"DCAE DNS zoen created for ONAP deployment $DCAE_ZONE\"}"
        RESP=$(curl -s -v -X POST -H "$HEADER_ACCEPT_JSON" -H "$HEADER_CONTENT_TYPE_JSON" -H "$HEADER_TOKEN" -d "$API_DATA" "$API_ENDPOINT")
        ZONE_ID=$(echo $RESP | jq .id)

        if [ -z "$ZONE_ID" ]; then
            echo "Fail to create DNS zone ${ZONE_NAME}.  Exit"
            exit 1
        fi
    else
        echo "======> Zone ${ZONE_NAME} already exists."
        API_ENDPOINT="${DESIGNATE_URL}/v2/zones/${ZONE_ID}"
        RESP=$(curl -s -v -H "$HEADER_ACCEPT_JSON" -H "$HEADER_TOKEN" "$API_ENDPOINT")
        ZONE_PROJECT_ID=$(echo $RESP | jq -r .project_id)
        if [ "$ZONE_PROJECT_ID" != "noauth-project" ] && [ "$ZONE_PROJECT_ID" != "$TENANT_ID" ]; then
            echo "======> Zone ${ZONE_NAME} owned by other projects, may have problem creating records"
        else
            echo "======> Zone ${ZONE_NAME} okay to create new records"
        fi
    fi
}

delete_dns_zone()
{
    export http_proxy=""
    export https_proxy=""
	
    local CLOUD_OWNER='pod25'
    local CLOUD_REGION
    local CLOUD_VERSION='titanium_cloud'
    local CLOUD_ENV
    local DCAE_ZONE
    local DNSAAS_TENANT_NAME
    local MCHOST
    local MCURL
    local MCMETHOD='-X GET'
    local MCRESP='200'
    local MCHEADERS='-H "Real-Time: true" -H "Content-Type: application/json" -H "Accept: application/json"'
    local MCAUTH=''
    local MCDATA=''
    local MULTICLOUD_PLUGIN_ENDPOINT

    CLOUD_REGION="$(cat /opt/config/openstack_region.txt)"
    CLOUD_ENV="$(cat /opt/config/cloud_env.txt)"
    DCAE_ZONE="$(cat /opt/config/dcae_zone.txt)"
    DNSAAS_TENANT_NAME="$(cat /opt/config/dnsaas_tenant_name.txt)"
    MCHOST=$(cat /opt/config/multiserv_ip_addr.txt)
    MCURL="http://$MCHOST:9005/api/multicloud-titaniumcloud/v0/swagger.json"

    local DCAE_DOMAIN
    local ZONENAME
    DCAE_DOMAIN="$(cat /opt/config/dcae_domain.txt)"
    ZONENAME="${DCAE_ZONE}.${DCAE_DOMAIN}."

    MCDATA='"{\"auth\":{\"tenantName\": \"'${DNSAAS_TENANT_NAME}'\"}}"'
    MULTICLOUD_PLUGIN_ENDPOINT=http://${MCHOST}/api/multicloud-titaniumcloud/v0/${CLOUD_OWNER}_${CLOUD_REGION}

    ### Get Token
    local TOKEN
    TOKEN=$(curl -v -s -H "Content-Type: application/json" -X POST -d "{\"auth\":{\"tenantName\": \"${DNSAAS_TENANT_NAME}\"}}" "${MULTICLOUD_PLUGIN_ENDPOINT}/identity/v3/auth/tokens"  2>&1 | grep X-Subject-Token | sed "s/^.*: //")

    local ZONEID
    ZONEID=$(curl -v -s  -H "Content-Type: application/json" -H "X-Auth-Token: $TOKEN" -X GET "${MULTICLOUD_PLUGIN_ENDPOINT}/dns-delegate/v2/zones?name=${ZONENAME}" |sed 's/^.*"id":"\([a-zA-Z0-9-]*\)",.*$/\1/')

    curl -s -H "Content-Type: application/json" -H "X-Auth-Token: $TOKEN" -X DELETE "${MULTICLOUD_PLUGIN_ENDPOINT}/dns-delegate/v2/zones/${ZONEID}"
}

list_dns_zone() 
{
    export http_proxy=""
    export https_proxy=""
	
    local CLOUD_OWNER='pod25'
    local CLOUD_REGION
    local CLOUD_VERSION='titanium_cloud'
    local CLOUD_ENV
    local DCAE_ZONE
    local DNSAAS_TENANT_NAME
    local MCHOST
    local MCURL
    local MCMETHOD='-X GET'
    local MCRESP='200'
    local MCHEADERS='-H "Real-Time: true" -H "Content-Type: application/json" -H "Accept: application/json"'
    local MCAUTH=''
    local MCDATA=''
    local MULTICLOUD_PLUGIN_ENDPOINT

    CLOUD_REGION="$(cat /opt/config/openstack_region.txt)"
    CLOUD_ENV="$(cat /opt/config/cloud_env.txt)"
    DCAE_ZONE="$(cat /opt/config/dcae_zone.txt)"
    DNSAAS_TENANT_NAME="$(cat /opt/config/dnsaas_tenant_name.txt)"
    MCHOST=$(cat /opt/config/multiserv_ip_addr.txt)
    MCURL="http://$MCHOST:9005/api/multicloud-titaniumcloud/v0/swagger.json"

    MCDATA='"{\"auth\":{\"tenantName\": \"'${DNSAAS_TENANT_NAME}'\"}}"'
    MULTICLOUD_PLUGIN_ENDPOINT=http://${MCHOST}/api/multicloud-titaniumcloud/v0/${CLOUD_OWNER}_${CLOUD_REGION}

    ### Get Token
    local TOKEN
    TOKEN=$(curl -v -s -H "Content-Type: application/json" -X POST -d "{\"auth\":{\"tenantName\": \"${DNSAAS_TENANT_NAME}\"}}" "${MULTICLOUD_PLUGIN_ENDPOINT}/identity/v3/auth/tokens"  2>&1 | grep X-Subject-Token | sed "s/^.*: //")

    local DCAE_DOMAIN
    local ZONENAME
    DCAE_DOMAIN="$(cat /opt/config/dcae_domain.txt)"
    ZONENAME="${DCAE_ZONE}.${DCAE_DOMAIN}."
    local ZONEID
    ZONEID=$(curl -v -s  -H "Content-Type: application/json" -H "X-Auth-Token: $TOKEN" -X GET "${MULTICLOUD_PLUGIN_ENDPOINT}/dns-delegate/v2/zones?name=${ZONENAME}" |sed 's/^.*"id":"\([a-zA-Z0-9-]*\)",.*$/\1/')

    curl -v -s  -H "Content-Type: application/json" -H "X-Auth-Token: $TOKEN" -X GET "${MULTICLOUD_PLUGIN_ENDPOINT}/dns-delegate/v2/zones/${ZONEID}/recordsets"
}

################################## start of vm_init #####################################

# prepare the configurations needed by DCAEGEN2 installer
rm -rf /opt/app/config
mkdir -p /opt/app/config


# private key
sed -e 's/\\n/\n/g' /opt/config/priv_key | sed -e 's/^[ \t]*//g; s/[ \t]*$//g' > /opt/app/config/key
chmod 777 /opt/app/config/key

# move keystone url file
#cp /opt/config/keystone_url.txt /opt/app/config/keystone_url.txt


URL_ROOT='nexus.onap.org/service/local/repositories/raw/content'
REPO_BLUEPRINTS='org.onap.dcaegen2.platform.blueprints'
REPO_DEPLOYMENTS='org.onap.dcaegen2.deployments'
if [ -e /opt/config/dcae_deployment_profile.txt ]; then
  DEPLOYMENT_PROFILE=$(cat /opt/config/dcae_deployment_profile.txt)
fi
DEPLOYMENT_PROFILE=${DEPLOYMENT_PROFILE:-R1}

NEXUS_USER=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWORD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_VERSION=$(cat /opt/config/docker_version.txt)
# use rand_str as zone
ZONE=$(cat /opt/config/rand_str.txt)
MYFLOATIP=$(cat /opt/config/dcae_float_ip.txt)
MYLOCALIP=$(cat /opt/config/dcae_ip_addr.txt)
HTTP_PROXY=$(cat /opt/config/http_proxy.txt)
HTTPS_PROXY=$(cat /opt/config/https_proxy.txt)

if [ $HTTP_PROXY != "no_proxy" ]
then
    export http_proxy=$HTTP_PROXY
    export https_proxy=$HTTPS_PROXY
fi

# start docker image pulling while we are waiting for A&AI to come online
docker login -u "$NEXUS_USER" -p "$NEXUS_PASSWORD" "$NEXUS_DOCKER_REPO"


if [ "$DEPLOYMENT_PROFILE" == "R1" ]; then
  RELEASE_TAG='releases'
  # download blueprint input template files
  rm -rf /opt/app/inputs-templates
  mkdir -p /opt/app/inputs-templates
  wget -P /opt/app/inputs-templates https://${URL_ROOT}/${REPO_BLUEPRINTS}/${RELEASE_TAG}/input-templates/inputs.yaml
  wget -P /opt/app/inputs-templates https://${URL_ROOT}/${REPO_BLUEPRINTS}/${RELEASE_TAG}/input-templates/cdapinputs.yaml
  wget -P /opt/app/inputs-templates https://${URL_ROOT}/${REPO_BLUEPRINTS}/${RELEASE_TAG}/input-templates/phinputs.yaml
  wget -P /opt/app/inputs-templates https://${URL_ROOT}/${REPO_BLUEPRINTS}/${RELEASE_TAG}/input-templates/dhinputs.yaml
  wget -P /opt/app/inputs-templates https://${URL_ROOT}/${REPO_BLUEPRINTS}/${RELEASE_TAG}/input-templates/invinputs.yaml
  wget -P /opt/app/inputs-templates https://${URL_ROOT}/${REPO_BLUEPRINTS}/${RELEASE_TAG}/input-templates/vesinput.yaml
  wget -P /opt/app/inputs-templates https://${URL_ROOT}/${REPO_BLUEPRINTS}/${RELEASE_TAG}/input-templates/tcainputs.yaml
  wget -P /opt/app/inputs-templates https://${URL_ROOT}/${REPO_BLUEPRINTS}/${RELEASE_TAG}/input-templates/he-ip.yaml
  wget -P /opt/app/inputs-templates https://${URL_ROOT}/${REPO_BLUEPRINTS}/${RELEASE_TAG}/input-templates/hr-ip.yaml

  # generate blueprint input files
  pip install --upgrade jinja2
  wget https://${URL_ROOT}/${REPO_DEPLOYMENTS}/${RELEASE_TAG}/scripts/detemplate-bpinputs.py \
    && \
    (python detemplate-bpinputs.py /opt/config /opt/app/inputs-templates /opt/app/config; \
     rm detemplate-bpinputs.py)

  # Run docker containers
  cd /opt


  docker pull "$NEXUS_DOCKER_REPO/onap/org.onap.dcaegen2.deployments.bootstrap:$DOCKER_VERSION" \
    && docker pull nginx &

  #########################################
  # Wait for then register with A&AI
  ########################################

  DNSAAS_PROXYED=$(tr '[:upper:]' '[:lower:]' < /opt/config/dnsaas_config_enabled.txt)
  if [ "$DNSAAS_PROXYED" == 'true' ]; then
    echo "Using proxyed DNSaaS service, performing additional registration and configuration"
    wait_for_aai_ready

    register_multicloud_pod25_with_aai
    register_multicloud_pod25dns_with_aai

    verify_multicloud_registration

    wait_for_multicloud_ready
    register_dns_zone_proxied_designate "$ZONE" 
    echo "Registration and configuration for proxying DNSaaS completed."
  else
    echo "Using Designate DNSaaS service, performing additional registration and configuration"
    register_dns_zone_designate "$ZONE" 
  fi

  #########################################
  # Start DCAE Bootstrap container
  #########################################

  chmod 777 /opt/app/config
  rm -f /opt/config/runtime.ip.consul
  rm -f /opt/config/runtime.ip.cm


  #docker login -u "$NEXUS_USER" -p "$NEXUS_PASSWORD" "$NEXUS_DOCKER_REPO"
  #docker pull "$NEXUS_DOCKER_REPO/onap/org.onap.dcaegen2.deployments.bootstrap:$DOCKER_VERSION"
  docker run -d --name boot -v /opt/app/config:/opt/app/installer/config -e "LOCATION=$ZONE" "$NEXUS_DOCKER_REPO/onap/org.onap.dcaegen2.deployments.bootstrap:$DOCKER_VERSION"


  # waiting for bootstrap to complete then starting nginx for proxying healthcheck calls
  echo "Waiting for Consul to become accessible"
  while [ ! -f /opt/app/config/runtime.ip.consul ]; do echo "."; sleep 30; done


  # start proxy for consul's health check
  CONSULIP=$(head -1 /opt/app/config/runtime.ip.consul | sed 's/[[:space:]]//g')
  echo "Consul is available at $CONSULIP" 
fi

if [[ $DEPLOYMENT_PROFILE == R2* ]]; then
  RELEASE_TAG='R2'
  set +e
  rm -rf /opt/app/inputs-templates
  mkdir -p /opt/app/inputs-templates
  wget -P /opt/app/inputs-templates https://${URL_ROOT}/${REPO_DEPLOYMENTS}/${RELEASE_TAG}/heat/docker-compose-1.yaml
  wget -P /opt/app/inputs-templates https://${URL_ROOT}/${REPO_DEPLOYMENTS}/${RELEASE_TAG}/heat/docker-compose-2.yaml
  wget -P /opt/app/inputs-templates https://${URL_ROOT}/${REPO_DEPLOYMENTS}/${RELEASE_TAG}/heat/docker-compose-3.yaml
  wget -P /opt/app/inputs-templates https://${URL_ROOT}/${REPO_DEPLOYMENTS}/${RELEASE_TAG}/heat/docker-compose-4.yaml
  wget -P /opt/app/inputs-templates https://${URL_ROOT}/${REPO_DEPLOYMENTS}/${RELEASE_TAG}/heat/register.sh
  wget -P /opt/app/inputs-templates https://${URL_ROOT}/${REPO_DEPLOYMENTS}/${RELEASE_TAG}/heat/setup.sh
  wget -P /opt/app/inputs-templates https://${URL_ROOT}/${REPO_DEPLOYMENTS}/${RELEASE_TAG}/heat/teardown.sh

  pip install --upgrade jinja2
  wget https://${URL_ROOT}/${REPO_DEPLOYMENTS}/${RELEASE_TAG}/scripts/detemplate-bpinputs.py \
    && \
    (python detemplate-bpinputs.py /opt/config /opt/app/inputs-templates /opt/app/config; \
     rm detemplate-bpinputs.py)

  if [ -e /opt/app/config/register.sh ]; then
    chmod +x /opt/app/config/register.sh
  fi
  if [ -e /opt/app/config/setup.sh ]; then
    chmod +x /opt/app/config/setup.sh
  fi
  if [ -e /opt/app/config/build-plugins.sh ]; then
    chmod +x /opt/app/config/build-plugins.sh
  fi
  set -e

  cd /opt/app/config
  # deploy essentials
  /opt/docker/docker-compose -f docker-compose-1.yaml up -d
  echo "Waiting for Consul to come up ready"
  while ! nc -z localhost 8500; do sleep 1; done
  echo "Waiting for DB to come up ready"
  while ! nc -z localhost 5432; do sleep 1; done
  echo "Waiting for CBS to come up ready"
  while ! nc -z localhost 10000; do sleep 1; done
  echo "All dependencies are up, proceed to the next phase"
  sleep 30

  echo "Setup CloudifyManager and Registrator"
  ./setup.sh
  sleep 10

  export http_proxy=""
  export https_proxy=""


  ./register.sh

  echo "Bring up DCAE MIN service components for R2 use cases"
  /opt/docker/docker-compose -f docker-compose-2.yaml up -d

  if [[ "$DEPLOYMENT_PROFILE" == "R2" || "$DEPLOYMENT_PROFILE" == "R2PLUS" ]]; then
    echo "Bring up DCAE platform components"
    /opt/docker/docker-compose -f docker-compose-3.yaml up -d

    if [ "$DEPLOYMENT_PROFILE" == "R2PLUS" ]; then
      echo "Bring up additional (plus) DCAE service components"
      /opt/docker/docker-compose -f docker-compose-4.yaml up -d
    fi
  fi

  # start proxy for consul's health check
  CONSULIP=$(cat /opt/config/dcae_ip_addr.txt)
  echo "Consul is available at $CONSULIP"
fi

cat >./nginx.conf <<EOL
server {
    listen 80;
    server_name dcae.simpledemo.onap.org;
    root /www/healthcheck;

    location /healthcheck {
        try_files /services.yaml =404;
    }
    location /R1 {
        proxy_pass http://${CONSULIP}:8500/v1/health/state/passing;
    }
    location /R2MIN{
        try_files /r2mvp_healthy.yaml =404;
    }
    location /R2 {
        try_files /r2_healthy.yaml =404;
    }
    location /R2PLUS {
        try_files /r2plus_healthy.yaml =404;
    }
}
EOL

HEALTHPORT=8000
docker run -d \
--name dcae-health \
-p ${HEALTHPORT}:80 \
-v "$(pwd)/nginx.conf:/etc/nginx/conf.d/default.conf" \
-v "/tmp/healthcheck:/www/healthcheck" \
--label "SERVICE_80_NAME=dcae-health" \
--label "SERVICE_80_CHECK_HTTP=/healthcheck" \
--label "SERVICE_80_CHECK_INTERVAL=15s" \
--label "SERVICE_80_CHECK_INITIAL_STATUS=passing" \
 nginx

echo "Healthcheck API available at http://${MYFLOATIP}:${HEALTHPORT}/healthcheck"
echo "                             http://${MYFLOATIP}:${HEALTHPORT}/R1"
echo "                             http://${MYFLOATIP}:${HEALTHPORT}/R2MIN"
echo "                             http://${MYFLOATIP}:${HEALTHPORT}/R2PLUS"

# run forever for updating health status based on consul
set +e
while :
do
  rm -rf /tmp/healthcheck/*
  # all registered services
  SERVICES=$(curl -s http://consul:8500/v1/agent/services |jq '. | to_entries[] | .value.Service')
  # passing services
  SERVICES=$(curl -s http://consul:8500/v1/health/state/passing | jq '.[] | .ServiceName')

  # remove empty lines/entries
  SERVICES=$(echo "$SERVICES" | sed '/^\s*\"\"\s*$/d' |sed '/^\s*$/d')

  SERVICES_JSON=$(echo "$SERVICES" | sed 's/\"$/\",/g' | sed '$ s/.$//')

  echo "$(date): running healthy services:"
  echo ">>> " $SERVICES
  PLT_CONSUL=$(echo "$SERVICES" |grep consul)
  PLT_CBS=$(echo "$SERVICES" |grep "config_binding_service")
  MVP_PG_HOLMES=$(echo "$SERVICES" |grep "pgHolmes")
  MVP_VES=$(echo "$SERVICES" |grep "mvp.*ves")
  MVP_TCA=$(echo "$SERVICES" |grep "mvp.*tca")
  MVP_HR=$(echo "$SERVICES" |grep "mvp.*holmes-rule")
  MVP_HE=$(echo "$SERVICES" |grep "mvp.*holmes-engine")

  PLT_CM=$(echo "$SERVICES" |grep "cloudify.*manager")
  PLT_DH=$(echo "$SERVICES" |grep "deployment.*handler")
  PLT_PH=$(echo "$SERVICES" |grep "policy.*handler")
  PLT_SCH=$(echo "$SERVICES" |grep "service.*change.*handler")
  PLT_INV=$(echo "$SERVICES" |grep "inventory")
  PLT_PG_INVENTORY=$(echo "$SERVICES" |grep "pgInventory")

  PLUS_MHB=$(echo "$SERVICES" |grep "heartbeat")
  PLUS_PRH=$(echo "$SERVICES" |grep "prh")
  PLUS_MPR=$(echo "$SERVICES" |grep "mapper")
  PLUS_TRAP=$(echo "$SERVICES" |grep "snmptrap")

  DATA="{\"healthy\" : \"$(date)\", \"healthy_services\": [${SERVICES_JSON}]}"
  if [[ -n "$PLT_CONSUL" && -n "$PLT_CBS" && -n "$MVP_PG_HOLMES" && -n "$MVP_VES" && \
        -n "$MVP_TCA" && -n "$MVP_HR" && -n "$MVP_HE" ]]; then
    echo "${DATA}" > /tmp/healthcheck/r2mvp_healthy.yaml
    echo "${DATA}" > /tmp/healthcheck/services.yaml
    echo ">>>>>> enough services satisfying R2MIN service deployment"
  else
    echo ">>>>>> not enough services satisfying R2MIN service deployment"
  fi

  if [[ -n "$PLT_CONSUL" && -n "$PLT_CBS" && -n "$PLT_CM" && -n "$PLT_DH" && \
        -n "$PLT_PH" && -n "$PLT_SCH" && -n "$PLT_INV" && -n "$PLT_PG_INVENTORY" ]]; then
    echo ">>>>>> enough services satisfying R2 platform deployment"
    echo "${DATA}" > /tmp/healthcheck/r2_healthy.yaml

    if [[ -n "$PLUS_MHB" && -n "$PLUS_PRH" && -n "$PLUS_MPR" && -n "$PLUS_TRAP" ]]; then
      echo ">>>>>> enough services satisfying R2PLUS deployment"
      echo "${DATA}" > /tmp/healthcheck/r2plus_healthy.yaml
    else
      echo ">>>>>> not enough services satisfying R2PLUS service deployment"
    fi
  else
    echo ">>>>>> not enough services satisfying R2 platform or R2PLUS service deployment"
  fi
  sleep 60
done

