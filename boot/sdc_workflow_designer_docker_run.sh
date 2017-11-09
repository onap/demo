#!/bin/bash


function usage {
    echo "usage: docker_run.sh [ -r|--release <RELEASE-NAME> ]  [ -e|--environment <ENV-NAME> ] [ -p|--port <Docker-hub-port>] [ -l|--local <Run-without-pull>] [ -h|--help ]"
}


function cleanup {
	echo "performing old dockers cleanup"
	docker_ids=`docker ps -a | egrep "sdc-workflow-designer" | awk '{print $1}'`
	for X in ${docker_ids}
	do
	   docker rm -f ${X}
	done
}


RELEASE=latest
LOCAL=false
[ -f /opt/config/env_name.txt ] && DEP_ENV=$(cat /opt/config/env_name.txt) || DEP_ENV=__ENV-NAME__
[ -f /opt/config/nexus_username.txt ] && NEXUS_USERNAME=$(cat /opt/config/nexus_username.txt)    || NEXUS_USERNAME=release
[ -f /opt/config/nexus_password.txt ] && NEXUS_PASSWD=$(cat /opt/config/nexus_password.txt)      || NEXUS_PASSWD=sfWU3DFVdBr7GVxB85mTYgAW
[ -f /opt/config/nexus_docker_repo.txt ] && NEXUS_DOCKER_REPO=$(cat /opt/config/nexus_docker_repo.txt) || NEXUS_DOCKER_REPO=ecomp-nexus:${PORT}

while [ "$1" != "" ]; do
    case $1 in
        -r | --release )
            shift
            RELEASE=${1}
            ;;
        -e | --environment )
			shift
            DEP_ENV=${1}
            ;;
		-p | --port )
            shift
            PORT=${1}
			;;
		-l | --local )
		shift
		LOCAL=true
		;;
        -h | --help )
			usage
            exit
            ;;
        * ) 
    		usage
            exit 1
    esac
    shift
done

[ -f /opt/config/nexus_username.txt ] && docker login -u $NEXUS_USERNAME -p $NEXUS_PASSWD $NEXUS_DOCKER_REPO


cleanup


export IP=`ifconfig eth0 | awk -F: '/inet addr/ {gsub(/ .*/,"",$2); print $2}'`
export PREFIX=${NEXUS_DOCKER_REPO}'/onap'

# start docker
echo "docker run sdc-workflow-designer..."
if [ ${LOCAL} = false ]; then
	docker pull ${PREFIX}/sdc/sdc-workflow-designer:${RELEASE}
fi
docker run --detach --name sdc-workflow-designer --ulimit memlock=-1:-1 --memory 1g --memory-swap=1g --ulimit nofile=4096:100000 --publish 9527:8080 ${PREFIX}/sdc/sdc-workflow-designer:${RELEASE}


