#!/bin/bash

# Read configuration files
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)
GERRIT_BRANCH=$(cat /opt/config/gerrit_branch.txt)
CODE_REPO=$(cat /opt/config/remote_repo.txt)
HTTP_PROXY=$(cat /opt/config/http_proxy.txt)
HTTPS_PROXY=$(cat /opt/config/https_proxy.txt)

if [ $HTTP_PROXY != "no_proxy" ]
then
    export http_proxy=$HTTP_PROXY
    export https_proxy=$HTTPS_PROXY
fi

# Create partition and mount the external volume
cp /opt/boot/sdc_ext_volume_partitions.txt /opt/sdc_ext_volume_partitions.txt

if [[ $CLOUD_ENV == "rackspace" ]]
then
       DISK="xvdb"
else
       DISK=$(ls /dev |grep -e '^.*db$')
       sed -i "s/xvdb/$DISK/g" /opt/sdc_ext_volume_partitions.txt
fi

sfdisk /dev/$DISK < /opt/sdc_ext_volume_partitions.txt
mkfs -t ext4 /dev/$DISK"1"
mkdir -p /data
mount /dev/$DISK"1" /data
echo "/dev/"$DISK"1  /data           ext4    errors=remount-ro,noatime,barrier=0 0       1" >> /etc/fstab


# Clone Gerrit repository
cd /opt
mkdir -p /data/environments
mkdir -p /data/scripts
mkdir -p /data/logs/BE
mkdir -p /data/logs/FE
chmod 777 /data
chmod 777 /data/logs/BE
chmod 777 /data/logs/FE

git clone -b $GERRIT_BRANCH --single-branch $CODE_REPO

cat > /root/.bash_aliases << EOF
alias dcls='/data/scripts/docker_clean.sh \$1'
alias dlog='/data/scripts/docker_login.sh \$1'
alias rund='/data/scripts/docker_run.sh'
alias health='/data/scripts/docker_health.sh'
EOF

# Run docker containers. For openstack Ubuntu 16.04 images this will run as a service after the VM has restarted
./sdc_vm_init.sh
./sdc_wfd_vm_init.sh
