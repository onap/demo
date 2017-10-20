#!/bin/bash

NEXUS_USER=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWORD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_VERSION=$(cat /opt/config/docker_version.txt)
ZONE=$(cat /opt/config/dcae_zone.txt)


rm -f /opt/config/runtime.ip.consul
rm -f /opt/config/runtime.ip.cm


docker login -u "$NEXUS_USER" -p "$NEXUS_PASSWORD" "$NEXUS_DOCKER_REPO"
docker pull "$NEXUS_DOCKER_REPO/onap/org.onap.dcaegen2.deployments.bootstrap:$DOCKER_VERSION"
#docker run -v /opt/config/priv_key:/opt/app/installer/config/key -v /opt/app/inputs.yaml:/opt/app/installer/config/inputs.yaml -e "LOCATION=$ZONE" $NEXUS_DOCKER_REPO/onap/org.onap.dcaegen2.deployments.bootstrap:$DOCKER_VERSION
docker run -d --name boot -v /opt/app/config:/opt/app/installer/config -e "LOCATION=$ZONE" "$NEXUS_DOCKER_REPO/onap/org.onap.dcaegen2.deployments.bootstrap:$DOCKER_VERSION"


# waiting for bootstrap to complete then starting nginx for proxying healthcheck calls
while [ ! -f /opt/config/runtime.ip.consul ]; do sleep 30; done

# start proxy for consul's health check
CONSULIP=$(head -1 /opt/config/runtime.ip.consul | sed 's/[[:space:]]//g')
echo "Consul is available at $CONSULIP" 

cat >./nginx.conf <<EOL
server {
    listen 80;
    server_name dcae.simpledemo.onap.org;
    location /healthcheck {
        proxy_pass http://"${CONSULIP}":8500/v1/health/state/passing;
    }
}
EOL
sudo docker run --name dcae-proxy -p 80:80 -v "$(pwd)/nginx.conf:/etc/nginx/conf.d/default.conf" -d nginx
