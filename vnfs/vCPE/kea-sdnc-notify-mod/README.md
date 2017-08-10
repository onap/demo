#
# kea-sdnc-notify-mod

Kea module utilizing hooks api to notify SDNC of macaddr, yiaddr, dhcp-msg-name via HTTP POST request.

# ONAP installation of vDHCP will install and configure on the vDHCP VM
#
# for local development/testing follow these steps.
#

#
# install kea on ubuntu
apt-get install  kea-dhcp4-server


#
# Usage
Install the compiled library from the ./build direction to  /usr/local/lib/kea-sdnc-notify.so

Configure /etc/kea/kea-sdnc-notify.conf using the ./etc example

Configure /etc/kea/kea-dhcp4.conf usign the ./etc example

The hook will POST json from the the variables from the DHCP Acknowledgement message to the SDNC "url" in the form of "url+macaddr"
"url": "http://localhost/sdnc.php?macaddr="

Example: http://localhost/sdnc.php?macaddr=aa:bb:cc:dd:ee:ff

In the ONAP installation instead of localhost it will be to either a DMaaP Topic or to SDNC directly.


## START DHCP
[as root]
cd /etc/init.d 
./kea-dhcp4-server start or
./kea-dhcp4-server restart 

./kea-dhcp4-server stop 

## logs are in 
/var/log/kea-dhcp4.log

## Build requirements
This software has been developed on Ubuntu 16.04.  

apt-get install g++ libcurl4-gnutls-dev libboost-dev kea-dev

./build.sh

## Installation in  ONAP
The vdhcp install scripts for ONAP will do the build on the vDHCP VM from these source files and copy the resulting library into /usr/local/lib and the configuration files into /etc/kea

## Testing locally

#### USE THIS ON OPENSTACK UBUNTU 16.04 GUEST VM

Create a veth pair:

ip link add veth0 type veth peer name veth1

ip link set veth0 up && ip link set veth1 up

ip address add dev veth0 10.3.0.1/24

dhclient -d -v veth1

dhclient -d -v veth1 -r (to release) 

