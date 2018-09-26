#!/bin/bash
#############################################################################
#
# Copyright (c) 2017-2018 AT&T Intellectual Property. All rights reserved.
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




################################## start of vm_init #####################################

set -ex

URL_ROOT='nexus.onap.org/service/local/repositories/raw/content'
REPO_BLUEPRINTS='org.onap.dcaegen2.platform.blueprints'
REPO_DEPLOYMENTS='org.onap.dcaegen2.deployments'
if [ -e /opt/config/dcae_deployment_profile.txt ]; then
  DEPLOYMENT_PROFILE=$(cat /opt/config/dcae_deployment_profile.txt)
fi
DEPLOYMENT_PROFILE=${DEPLOYMENT_PROFILE:-R3}

NEXUS_USER=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWORD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_VERSION=$(cat /opt/config/docker_version.txt)

MYFLOATIP=$(cat /opt/config/dcae_float_ip.txt)
MYLOCALIP=$(cat /opt/config/dcae_ip_addr.txt)
HTTP_PROXY=$(cat /opt/config/http_proxy.txt)
HTTPS_PROXY=$(cat /opt/config/https_proxy.txt)

if [ $HTTP_PROXY != "no_proxy" ]
then
    export http_proxy=$HTTP_PROXY
    export https_proxy=$HTTPS_PROXY
fi

# clean up old network configuration in docker engine
docker network rm config_default

docker login -u "$NEXUS_USER" -p "$NEXUS_PASSWORD" "$NEXUS_DOCKER_REPO"

if [[ $DEPLOYMENT_PROFILE == R1* || $DEPLOYMENT_PROFILE == R2* ]]; then
  echo "R1 and R2 deployment profiles are not supported in Casablanca Heat deployment"
elif [[ $DEPLOYMENT_PROFILE == R3* ]]; then
  RELEASE_TAG='R3'

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

  # wait for essentials to become ready
  echo "Waiting for Consul to come up ready"
  while ! nc -z localhost 8500; do sleep 1; done
  echo "Waiting for Postgres DB to come up ready"
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

  if [[ "$DEPLOYMENT_PROFILE" == "R3" || "$DEPLOYMENT_PROFILE" == "R3PLUS" ]]; then
    echo "Bring up DCAE platform components"
    /opt/docker/docker-compose -f docker-compose-3.yaml up -d

    if [[ "$DEPLOYMENT_PROFILE" == "R3PLUS" ]]; then
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
    location /R3MIN{
        try_files /r3mvp_healthy.yaml =404;
    }
    location /R3 {
        try_files /r3_healthy.yaml =404;
    }
    location /R3PLUS {
        try_files /r3plus_healthy.yaml =404;
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
echo "                             http://${MYFLOATIP}:${HEALTHPORT}/R3"
echo "                             http://${MYFLOATIP}:${HEALTHPORT}/R3MIN"
echo "                             http://${MYFLOATIP}:${HEALTHPORT}/R3PLUS"

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
  echo ">>>  $SERVICES"
  PLT_CONSUL=$(echo "$SERVICES" |grep "consul")
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
        -n "$MVP_TCA" ]]; then
    echo "${DATA}" > /tmp/healthcheck/r3mvp_healthy.yaml
    echo "${DATA}" > /tmp/healthcheck/services.yaml
    echo ">>>>>> enough services satisfying R3MIN service deployment"
  else
    echo ">>>>>> not enough services satisfying R3MIN service deployment"
  fi

  if [[ -n "$PLT_CONSUL" && -n "$PLT_CBS" && -n "$PLT_CM" && -n "$PLT_DH" && \
        -n "$PLT_PH" && -n "$PLT_SCH" && -n "$PLT_INV" && -n "$PLT_PG_INVENTORY" ]]; then
    echo ">>>>>> enough services satisfying R3 platform deployment"
    echo "${DATA}" > /tmp/healthcheck/r3_healthy.yaml

    if [[ -n "$PLUS_MHB" && -n "$PLUS_PRH" && -n "$PLUS_MPR" && -n "$PLUS_TRAP"  && -n "$MVP_HR" && -n "$MVP_HE" ]]; then
      echo ">>>>>> enough services satisfying R3PLUS deployment"
      echo "${DATA}" > /tmp/healthcheck/r3plus_healthy.yaml
    else
      echo ">>>>>> not enough services satisfying R3PLUS service deployment"
    fi
  else
    echo ">>>>>> not enough services satisfying R3 platform or R3PLUS service deployment"
  fi
  
  sleep 60
done

