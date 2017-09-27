#!/bin/bash

# Read configuration files
NEXUS_REPO=$(cat /opt/config/nexus_repo.txt)
ARTIFACTS_VERSION=$(cat /opt/config/artifacts_version.txt)
DNS_IP_ADDR=$(cat /opt/config/dns_ip_addr.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)
GERRIT_BRANCH=$(cat /opt/config/gerrit_branch.txt)
CODE_REPO=$(cat /opt/config/remote_repo.txt)
MR_REPO=$(cat /opt/config/mr_repo.txt)

BASE=$(cat /opt/config/dcae_base_environment.txt)
PUBLIC_NET_ID=$(cat /opt/config/public_net_id.txt)
ZONE=$(cat /opt/config/dcae_zone.txt)
STATE=$(cat /opt/config/dcae_state.txt)
HORIZON_URL=$(cat /opt/config/horizon_url.txt)
OPENSTACK_USER=$(cat /opt/config/openstack_user.txt)
OPENSTACK_PASSWORD=$(cat /opt/config/openstack_password.txt)
OPENSTACK_KEYNAME=$(cat /opt/config/key_name.txt)"_"$(cat /opt/config/rand_str.txt)"_dcae"
OPENSTACK_PUBKEY=$(cat /opt/config/pub_key.txt)
OPENSTACK_AUTH_METHOD=$(cat /opt/config/openstack_auth_method.txt)
KEYSTONE_URL=$(cat /opt/config/keystone_url.txt)"/v2.0"
OPENSTACK_TENANT_ID=$(cat /opt/config/tenant_id.txt)
OPENSTACK_TENANT_NAME=OPEN-ECOMP
OPENSTACK_REGION=$(cat /opt/config/openstack_region.txt)
OPENSTACK_PRIVATE_NETWORK=$(cat /opt/config/openstack_private_network_name.txt)

NEXUS_URL_ROOT=$(cat /opt/config/nexus_repo_root.txt)
NEXUS_USER=$(cat /opt/config/nexus_username.txt)
NEXUS_PASSWORD=$(cat /opt/config/nexus_password.txt)
NEXUS_URL_SNAPSHOTS=$(cat /opt/config/nexus_url_snapshots.txt)
DOCKER_REGISTRY=$(cat /opt/config/nexus_docker_repo.txt)
DOCKER_VERSION=$(cat /opt/config/docker_version.txt)
DCAE_CODE_VERSION=$(cat /opt/config/dcae_code_version.txt)

DCAE_IP_ADDR=$(cat /opt/config/dcae_ip_addr.txt)
DCAE_COLL_IP_ADDR=$(cat /opt/config/dcae_coll_ip_addr.txt)
DCAE_DB_IP_ADDR=$(cat /opt/config/dcae_db_ip_addr.txt)
DCAE_HDP1_IP_ADDR=$(cat /opt/config/dcae_hdp1_ip_addr.txt)
DCAE_HDP2_IP_ADDR=$(cat /opt/config/dcae_hdp2_ip_addr.txt)
DCAE_HDP3_IP_ADDR=$(cat /opt/config/dcae_hdp3_ip_addr.txt)

MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)

if [[ $CLOUD_ENV != "rackspace" ]]
then
	# Add host name to /etc/host to avoid warnings in openstack images
	echo 127.0.0.1 $(hostname) >> /etc/hosts

	# Allow remote login as root
	mv /root/.ssh/authorized_keys /root/.ssh/authorized_keys.bk
	cp /home/ubuntu/.ssh/authorized_keys /root/.ssh
fi

# Set private IP in /etc/network/interfaces manually in the presence of public interface
# Some VM images don't add the private interface automatically, we have to do it during the component installation
if [[ $CLOUD_ENV == "openstack_nofloat" ]]
then
	CIDR=$(cat /opt/config/oam_network_cidr.txt)
	BITMASK=$(echo $CIDR | cut -d"/" -f2)

	# Compute the netmask based on the network cidr
	if [[ $BITMASK == "8" ]]
	then
		NETMASK=255.0.0.0
	elif [[ $BITMASK == "16" ]]
	then
		NETMASK=255.255.0.0
	elif [[ $BITMASK == "24" ]]
	then
		NETMASK=255.255.255.0
	fi

	echo "auto eth1" >> /etc/network/interfaces
	echo "iface eth1 inet static" >> /etc/network/interfaces
	echo "    address $DCAE_IP_ADDR" >> /etc/network/interfaces
	echo "    netmask $NETMASK" >> /etc/network/interfaces
	echo "    mtu $MTU" >> /etc/network/interfaces
	ifup eth1
fi

# Download dependencies
echo "deb http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
echo "deb-src http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
apt-get update
apt-get install --allow-unauthenticated -y apt-transport-https ca-certificates wget make openjdk-8-jdk git ntp ntpdate

# Download scripts from Nexus
curl -k $NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/dcae_vm_init.sh -o /opt/dcae_vm_init.sh
curl -k $NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/dcae_serv.sh -o /opt/dcae_serv.sh
chmod +x /opt/dcae_vm_init.sh
chmod +x /opt/dcae_serv.sh
mv /opt/dcae_serv.sh /etc/init.d
update-rc.d dcae_serv.sh defaults

# Download and install docker-engine and docker-compose
echo "deb https://apt.dockerproject.org/repo ubuntu-trusty main" | sudo tee /etc/apt/sources.list.d/docker.list
apt-get update
apt-get install -y linux-image-extra-$(uname -r) linux-image-extra-virtual
apt-get install -y --allow-unauthenticated docker-engine

mkdir /opt/docker
curl -L https://github.com/docker/compose/releases/download/1.9.0/docker-compose-`uname -s`-`uname -m` > /opt/docker/docker-compose
chmod +x /opt/docker/docker-compose

# Set the MTU size of docker containers to the minimum MTU size supported by vNICs. OpenStack deployments may need to know the external DNS IP
DNS_FLAG=""
if [ -s /opt/config/dns_ip_addr.txt ]
then
	DNS_FLAG=$DNS_FLAG"--dns $(cat /opt/config/dns_ip_addr.txt) "
fi
if [ -s /opt/config/external_dns.txt ]
then
	DNS_FLAG=$DNS_FLAG"--dns $(cat /opt/config/external_dns.txt) "
fi
echo "DOCKER_OPTS=\"$DNS_FLAG--mtu=$MTU\"" >> /etc/default/docker

cp /lib/systemd/system/docker.service /etc/systemd/system
sed -i "/ExecStart/s/$/ --mtu=$MTU/g" /etc/systemd/system/docker.service
service docker restart

# DNS IP address configuration
echo "nameserver "$DNS_IP_ADDR >> /etc/resolvconf/resolv.conf.d/head
resolvconf -u

# Clone Gerrit repository
cd /opt
git clone -b $GERRIT_BRANCH --single-branch $CODE_REPO dcae-startup-vm-controller

# Build a configuration file for the DCAE Controller.
cd /opt/dcae-startup-vm-controller
mkdir -p /opt/app/dcae-controller
cat > /opt/app/dcae-controller/config.yaml << EOF_CONFIG
BASE: $BASE
ZONE: $ZONE
DNS-IP-ADDR: $DNS_IP_ADDR
STATE: $STATE
DCAE-VERSION: $DCAE_CODE_VERSION
HORIZON-URL: $HORIZON_URL/$OPENSTACK_USER
KEYSTONE-URL: $KEYSTONE_URL
OPENSTACK-TENANT-ID: $OPENSTACK_TENANT_ID
OPENSTACK-TENANT-NAME: $OPENSTACK_TENANT_NAME
OPENSTACK-REGION: $OPENSTACK_REGION
OPENSTACK-PRIVATE-NETWORK: $OPENSTACK_PRIVATE_NETWORK
OPENSTACK-USER: $OPENSTACK_USER
OPENSTACK-PASSWORD: $OPENSTACK_PASSWORD
OPENSTACK-AUTH-METHOD: $OPENSTACK_AUTH_METHOD
OPENSTACK-KEYNAME: $OPENSTACK_KEYNAME
OPENSTACK-PUBKEY: $OPENSTACK_PUBKEY

NEXUS-URL-ROOT: $NEXUS_URL_ROOT
NEXUS-USER: $NEXUS_USER
NEXUS-PASSWORD: $NEXUS_PASSWORD
NEXUS-URL-SNAPSHOTS: $NEXUS_URL_SNAPSHOTS
NEXUS-RAWURL: $NEXUS_REPO

DOCKER-REGISTRY: $DOCKER_REGISTRY
DOCKER-VERSION: $DOCKER_VERSION

GIT-MR-REPO: $MR_REPO

public_net_id: $PUBLIC_NET_ID
dcae_ip_addr: $DCAE_IP_ADDR
dcae_pstg00_ip_addr: $DCAE_DB_IP_ADDR
dcae_coll00_ip_addr: $DCAE_COLL_IP_ADDR
dcae_cdap00_ip_addr: $DCAE_HDP1_IP_ADDR
dcae_cdap01_ip_addr: $DCAE_HDP2_IP_ADDR
dcae_cdap02_ip_addr: $DCAE_HDP3_IP_ADDR

EOF_CONFIG

# For non-Rackspace environment DCAE needs the OS image names and flavors
if [[ $CLOUD_ENV != "rackspace" ]]
then
	UBUNTU_1404_IMAGE=$(cat /opt/config/ubuntu_1404_image.txt)
	UBUNTU_1604_IMAGE=$(cat /opt/config/ubuntu_1604_image.txt)
	FLAVOR_SMALL=$(cat /opt/config/flavor_small.txt)
	FLAVOR_MEDIUM=$(cat /opt/config/flavor_medium.txt)
	FLAVOR_LARGE=$(cat /opt/config/flavor_large.txt)
	FLAVOR_XLARGE=$(cat /opt/config/flavor_xlarge.txt)

cat >> /opt/app/dcae-controller/config.yaml << EOF_CONFIG
UBUNTU-1404-IMAGE: $UBUNTU_1404_IMAGE
UBUNTU-1604-IMAGE: $UBUNTU_1604_IMAGE
FLAVOR-SMALL: $FLAVOR_SMALL
FLAVOR-MEDIUM: $FLAVOR_MEDIUM
FLAVOR-LARGE: $FLAVOR_LARGE
FLAVOR-XLARGE: $FLAVOR_XLARGE

EOF_CONFIG
fi

# Add floating IP section to DCAE config file for OpenStack deployments that use floating IPs
if [[ $CLOUD_ENV == "openstack" ]]
then
	# Read floating IP mapping
	DCAE_FLOAT_IP_ADDR=$(cat /opt/config/dcae_float_ip.txt)
	DCAE_COLL_FLOAT_IP=$(cat /opt/config/dcae_coll_float_ip.txt)
	DCAE_DB_FLOAT_IP=$(cat /opt/config/dcae_db_float_ip.txt)
	DCAE_HDP1_FLOAT_IP=$(cat /opt/config/dcae_hdp1_float_ip.txt)
	DCAE_HDP2_FLOAT_IP=$(cat /opt/config/dcae_hdp2_float_ip.txt)
	DCAE_HDP3_FLOAT_IP=$(cat /opt/config/dcae_hdp3_float_ip.txt)

cat >> /opt/app/dcae-controller/config.yaml << EOF_CONFIG
dcae_float_ip_addr: $DCAE_FLOAT_IP_ADDR
dcae_pstg00_float_ip_addr: $DCAE_DB_FLOAT_IP
dcae_coll00_float_ip_addr: $DCAE_COLL_FLOAT_IP
dcae_cdap00_float_ip_addr: $DCAE_HDP1_FLOAT_IP
dcae_cdap01_float_ip_addr: $DCAE_HDP2_FLOAT_IP
dcae_cdap02_float_ip_addr: $DCAE_HDP3_FLOAT_IP
EOF_CONFIG
fi

# Run docker containers
cd /opt
./dcae_vm_init.sh
