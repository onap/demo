#!/bin/bash
set -o xtrace  # print commands during script execution
set -o errexit # exit on command errors

VPP_SOURCE_REPO_URL=$(cat /opt/config/vpp_source_repo_url.txt)
VPP_SOURCE_REPO_RELEASE_TAG=$(cat /opt/config/vpp_source_repo_release_tag.txt)
VPP_PATCH_URL=$(cat /opt/config/vpp_patch_url.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)
ERROR_MESSAGE='Execution of vBRG script failed.'

# Convert Network CIDR to Netmask
cdr2mask () {
	# Number of args to shift, 255..255, first non-255 byte, zeroes
	set -- $(( 5 - ($1 / 8) )) 255 255 255 255 $(( (255 << (8 - ($1 % 8))) & 255 )) 0 0 0
	[ $1 -gt 1 ] && shift $1 || shift
	echo ${1-0}.${2-0}.${3-0}.${4-0}
}


# Enable IPV4 forwarding through kernel
    sed -i 's/^.*\(net.ipv4.ip_forward\).*/\1=1/g' /etc/sysctl.conf
    sysctl -p /etc/sysctl.conf

# Download required dependencies
    echo "deb http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
    echo "deb-src http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
    apt-get update
    apt-get install --allow-unauthenticated -y wget openjdk-8-jdk apt-transport-https ca-certificates g++ libcurl4-gnutls-dev
    sleep 1

# Install the tools required for download codes
    apt-get install -y expect git patch make autoconf libtool linux-image-extra-`uname -r`

# Download and build the VPP codes
    cd /opt
    git clone ${VPP_SOURCE_REPO_URL} -b ${VPP_SOURCE_REPO_RELEASE_TAG} vpp
    wget -O Vpp-Integrate-FreeRADIUS-Client-for-vBNG.patch ${VPP_PATCH_URL}
    cd vpp
    # The patch will place a "dummy" version of dhcp.api.h so the build will succeed
    mkdir -p build-root/build-vpp-native/vpp/vnet/dhcp/
    patch -p1 < ../Vpp-Integrate-FreeRADIUS-Client-for-vBNG.patch
    UNATTENDED='y' make install-dep

# Check VPP build status
    if [[ $? -ne 0 ]]
    then
        echo $ERROR_MESSAGE 'Reason: VPP build failed' > /opt/script_status.txt
        exit
    fi

# Install the FreeRADIUS client since we need the lib
    cd /opt
    git clone https://github.com/FreeRADIUS/freeradius-client.git
    cd freeradius-client
    ./configure
    make && make install
    cd /usr/local/lib && ln -s -f libfreeradius-client.so.2.0.0 libfreeradiusclient.so
    ldconfig

    cd /opt/vpp/build-root
    ./bootstrap.sh
    make V=0 PLATFORM=vpp TAG=vpp install-deb

# Check vpp/build-root status
    if [[ $? -ne 0 ]]
    then
        echo $ERROR_MESSAGE 'Reason: vpp/build-root build failed' > /opt/script_status.txt
        exit
    fi

# Install additional dependencies for vpp
    apt-get install -y python-cffi python-ply python-pycparser

# Install the VPP package
    cd /opt/vpp/build-root
    dpkg -i *.deb

# Check VPP package installation status
    if [[ $? -ne 0 ]]
    then
        echo $ERROR_MESSAGE 'Reason: VPP package installation failed' > /opt/script_status.txt
        exit
    fi

    systemctl stop vpp

# Disable automatic upgrades
    if [[ $CLOUD_ENV != "rackspace" ]]
    then
        echo "APT::Periodic::Unattended-Upgrade \"0\";" >> /etc/apt/apt.conf.d/10periodic
        sed -i 's/\(APT::Periodic::Unattended-Upgrade\) "1"/\1 "0"/' /etc/apt/apt.conf.d/20auto-upgrades
    fi

# Indicate script has finished executing
    echo 'Execution of vBNG build script completed' > /opt/script_status.txt
