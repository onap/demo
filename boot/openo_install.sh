#!/bin/bash

# Read configuration files
NEXUS_REPO=$(cat /opt/config/nexus_repo.txt)
ARTIFACTS_VERSION=$(cat /opt/config/artifacts_version.txt)
DNS_IP_ADDR=$(cat /opt/config/dns_ip_addr.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)
VNFSDK_BRANCH=$(cat /opt/config/vnfsdk_branch.txt)
MTU=$(/sbin/ifconfig | grep MTU | sed 's/.*MTU://' | sed 's/ .*//' | sort -n | head -1)
VNFSDK_REPO=$(cat /opt/config/vnfsdk_repo.txt)

# Add host name to /etc/host to avoid warnings in openstack images
if [[ $CLOUD_ENV != "rackspace" ]]
then
    echo 127.0.0.1 $(hostname) >> /etc/hosts

    # Allow remote login as root
    mv /root/.ssh/authorized_keys /root/.ssh/authorized_keys.bk
    cp /home/ubuntu/.ssh/authorized_keys /root/.ssh
fi

# Set private IP in /etc/network/interfaces manually in the presence of public interface
# Some VM images don't add the private interface automatically, we have to do it during the component installation
if [[ $CLOUD_ENV == "openstack_nofloat" ]]
then
    LOCAL_IP=$(cat /opt/config/local_ip_addr.txt)
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
    echo "    address $LOCAL_IP" >> /etc/network/interfaces
    echo "    netmask $NETMASK" >> /etc/network/interfaces
    echo "    mtu $MTU" >> /etc/network/interfaces
    ifup eth1
fi

# Download dependencies
echo "deb http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
echo "deb-src http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
apt-get update
apt-get install --allow-unauthenticated -y apt-transport-https ca-certificates wget openjdk-8-jdk git unzip mysql-client-core-5.6 ntp ntpdate make

# Download scripts from Nexus
curl -k $NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/vnfsdk_vm_init.sh -o /opt/vnfsdk_vm_init.sh
curl -k $NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/msb_vm_init.sh -o /opt/msb_vm_init.sh
curl -k $NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/mvim_vm_init.sh -o /opt/mvim_vm_init.sh
curl -k $NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/vfc_vm_init.sh -o /opt/vfc_vm_init.sh
curl -k $NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/uui_vm_init.sh -o /opt/uui_vm_init.sh
curl -k $NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/openo_all_serv.sh -o /opt/openo_all_serv.sh
curl -k $NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/openo_serv.sh -o /opt/openo_serv.sh
curl -k $NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/cli_install.sh -o /opt/cli_install.sh
curl -k $NEXUS_REPO/org.onap.demo/boot/$ARTIFACTS_VERSION/esr_vm_init.sh -o /opt/esr_vm_init.sh
chmod +x /opt/vnfsdk_vm_init.sh
chmod +x /opt/msb_vm_init.sh
chmod +x /opt/mvim_vm_init.sh
chmod +x /opt/vfc_vm_init.sh
chmod +x /opt/uui_vm_init.sh
chmod +x /opt/openo_all_serv.sh
chmod +x /opt/openo_serv.sh
chmod +x /opt/cli_install.sh
chmod +x /opt/esr_vm_init.sh
mv /opt/openo_serv.sh /etc/init.d
update-rc.d openo_serv.sh defaults

# Download and install docker-engine and docker-compose
echo "deb https://apt.dockerproject.org/repo ubuntu-xenial main" | sudo tee /etc/apt/sources.list.d/docker.list
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

# Clone Gerrit repository and run docker containers
cd /opt
git clone -b $VNFSDK_BRANCH --single-branch $VNFSDK_REPO

source ./cli_install.sh

# Rename network interface in openstack Ubuntu 16.04 images. Then, reboot the VM to pick up changes
if [[ $CLOUD_ENV != "rackspace" ]]
then
    sed -i "s/GRUB_CMDLINE_LINUX=.*/GRUB_CMDLINE_LINUX=\"net.ifnames=0 biosdevname=0\"/g" /etc/default/grub
    grub-mkconfig -o /boot/grub/grub.cfg
    sed -i "s/ens[0-9]*/eth0/g" /etc/network/interfaces.d/*.cfg
    sed -i "s/ens[0-9]*/eth0/g" /etc/udev/rules.d/70-persistent-net.rules
    echo 'network: {config: disabled}' >> /etc/cloud/cloud.cfg.d/99-disable-network-config.cfg
    echo "APT::Periodic::Unattended-Upgrade \"0\";" >> /etc/apt/apt.conf.d/10periodic
    reboot
fi

./openo_all_serv.sh