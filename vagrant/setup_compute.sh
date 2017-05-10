#!/usr/bin/env bash

set -e

ip=`ifconfig | grep 192.168.0 | cut -f2 -d: | cut -f1 -d' '`
ipindx=`echo $ip|cut -f4 -d.`
sudo ifconfig br-ex 192.168.1.$ipindx/24 up

cd devstack
cp /vagrant/compute.conf local.conf
host=`hostname`
sed -i -e "s/HOSTIP/$ip/" -e "s/HOSTNAME/$host/" local.conf
./stack.sh

sudo pvcreate /dev/sdb
sudo vgextend vagrant-vg /dev/sdb
sudo lvextend -L +800G /dev/mapper/vagrant--vg-root
sudo resize2fs /dev/mapper/vagrant--vg-root
    
sudo pvcreate /dev/sdc
sudo vgextend stack-volumes-lvmdriver-1 /dev/sdc
echo "vagrant ssh control -c '/vagrant/create_onap.sh'"
