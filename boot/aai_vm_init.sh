#!/bin/bash

NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)
NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt)
DMAAP_TOPIC=$(cat /opt/config/dmaap_topic.txt)
GITLAB_CERTNAME=$(cat /opt/config/gitlab_certname.txt)
GITLAB_USERNAME=$(cat /opt/config/gitlab_username.txt)
GITLAB_PASSWD=$(cat /opt/config/gitlab_password.txt)

# Pull HBase container from a public docker hub
docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO
docker pull $NEXUS_USERNAME/aaidocker/aai-hbase-1.2.3
docker rm -f hbase-1.2.3
docker run -d --net=host --name="hbase-1.2.3" aaidocker/aai-hbase-1.2.3

# Wait 3 minutes before instantiating the A&AI container
sleep 180

docker pull $NEXUS_DOCKER_REPO/openecomp/ajsc-aai:latest
docker rm -f aai-service
docker run --name=aai-service --net=host -v /etc/ssl/certs/ca-certificates.crt:/etc/ssl/certs/ca-certificates.crt -it -e GITLAB_CERTNAME=$GITLAB_CERTNAME -e GITLAB_USERNAME=$GITLAB_USERNAME -e GITLAB_PASSWORD=$GITLAB_PASSWD -e AAI_REPO_PATH=r/aai -e AAI_CHEF_ENV=simpledemo -d -e AAI_CHEF_LOC=/var/chef/aai-data/environments -e docker_gitbranch=master $NEXUS_DOCKER_REPO/openecomp/ajsc-aai:latest

docker pull $NEXUS_DOCKER_REPO/openecomp/model-loader:latest
docker rm -f model-loader-service

docker run --name=model-loader-service -it -d -e DISTR_CLIENT_ASDC_ADDRESS=c2.vm1.asdc.simpledemo.openecomp.org:8443 -e DISTR_CLIENT_ENVIRONMENT_NAME=$DMAAP_TOPIC -e DISTR_CLIENT_USER=aai -e DISTR_CLIENT_PASSWORD=OBF:1ks51l8d1o3i1pcc1r2r1e211r391kls1pyj1z7u1njf1lx51go21hnj1y0k1mli1sop1k8o1j651vu91mxw1vun1mze1vv11j8x1k5i1sp11mjc1y161hlr1gm41m111nkj1z781pw31kku1r4p1e391r571pbm1o741l4x1ksp -e APP_SERVER_BASE_URL=https://c1.vm1.aai.simpledemo.openecomp.org:8443 -e APP_SERVER_KEYSTORE_PASSWORD=OBF:1i9a1u2a1unz1lr61wn51wn11lss1unz1u301i6o -e APP_SERVER_AUTH_USER=ModelLoader -e APP_SERVER_AUTH_PASSWORD=OBF:1qvu1v2h1sov1sar1wfw1j7j1wg21saj1sov1v1x1qxw $NEXUS_DOCKER_REPO/openecomp/model-loader:latest

