#!/bin/bash

# destroy running instances if any

for container in $(docker ps -q)
do
	echo "stopping container ${container}"
	docker stop "${container}"
done

for container in $(docker ps -a -q)
do
	echo "removing container ${container}"
	docker rm -v "${container}"
done

# remove dangling resources

for volume in $(docker volume ls -qf dangling=true)
do
	echo "removing volume ${volume}"
	docker volume rm "${volume}"
done

for image in $(docker images -f dangling=true -q)
do
	echo "removing image ${image}"
	docker rmi "${image}"
done

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_IMAGE_VERSION=$(cat /opt/config/docker_version.txt)
export MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)
export DOCKER_REPOSITORY=${NEXUS_DOCKER_REPO}

cd /opt/policy
git pull

chmod +x config/pe/brmsgw-tweaks.sh
chmod +x config/pe/pap-tweaks.sh
chmod +x config/pe/pdp-tweaks.sh
chmod +x config/pe/push-policies.sh
chmod +x config/drools/apps-install.sh
chmod +x config/drools/drools-preinstall.sh
chmod +x config/drools/drools-tweaks.sh
chmod +x config/db/db.sh

# ensure brmsgw dependency configuration is set up correctly

sed -i -e "s/^BRMS_DEPENDENCY_VERSION=.*$/BRMS_DEPENDENCY_VERSION=${ARTIFACT_VERSION}/g" /opt/policy/config/pe/brmsgw.conf


if [ -e /opt/config/public_ip.txt ]
then
  IP_ADDRESS=$(cat /opt/config/public_ip.txt)
else
  IP_ADDRESS=$(ifconfig eth0 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
fi

echo $IP_ADDRESS > config/pe/ip_addr.txt

docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO

#docker pull $NEXUS_DOCKER_REPO/onap/policy-db:$DOCKER_IMAGE_VERSION
#docker tag $NEXUS_DOCKER_REPO/onap/policy-db:$DOCKER_IMAGE_VERSION onap/policy/policy-db:latest

docker pull $NEXUS_DOCKER_REPO/onap/policy-pe:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/onap/policy-pe:$DOCKER_IMAGE_VERSION onap/policy-pe:latest

docker pull $NEXUS_DOCKER_REPO/onap/policy-drools:$DOCKER_IMAGE_VERSION
docker tag $NEXUS_DOCKER_REPO/onap/policy-drools:$DOCKER_IMAGE_VERSION onap/policy-drools:latest

#docker pull $NEXUS_DOCKER_REPO/onap/policy-nexus:$DOCKER_IMAGE_VERSION
#docker tag $NEXUS_DOCKER_REPO/onap/policy-nexus:$DOCKER_IMAGE_VERSION onap/policy-nexus:latest

/opt/docker/docker-compose up -d
