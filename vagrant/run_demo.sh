#!/usr/bin/env bash
set -ex

sudo apt-get install -y virtualbox
ver=$(apt-cache policy vagrant | grep Installed | cut -d ':' -f 3)
if [[ "$ver" != "1.8.7" ]]; then
    wget --no-check-certificate https://releases.hashicorp.com/vagrant/1.8.7/vagrant_1.8.7_x86_64.deb
    sudo dpkg -i vagrant_1.8.7_x86_64.deb
fi

vagrant destroy -f
vagrant up
vagrant ssh control -c "/vagrant/create_onap.sh"
sudo sed -i "/.*simpledemo.openecomp.org.*/d" /etc/hosts
cat hosts | sudo tee -a /etc/hosts
sleep 300
ssh -o StrictHostKeyChecking=no ubuntu@portal.api.simpledemo.openecomp.org -i onap "curl sina.com.cn"
