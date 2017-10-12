#!/usr/bin/env bash
set -ex

sudo apt-get install -y python-openstackclient python-heatclient

source /vagrant/openrc
cp /demo/heat/ONAP/* .

# Parameters used across all ONAP components
pub_net=$(openstack network list -f value|grep public | cut -f1 -d' ')
sed -i "s/public_net_id:.*/public_net_id: $pub_net/" onap_openstack.env
sed -i "s/ubuntu_1404_image:.*/ubuntu_1404_image: ubuntu1404/" onap_openstack.env
sed -i "s/ubuntu_1604_image:.*/ubuntu_1604_image: ubuntu1604/" onap_openstack.env
sed -i "s/flavor_small:.*/flavor_small: m1.small/" onap_openstack.env
sed -i "s/flavor_medium:.*/flavor_medium: m1.medium/" onap_openstack.env
sed -i "s/flavor_large:.*/flavor_large: m1.large/" onap_openstack.env
sed -i "s/flavor_xlarge:.*/flavor_xlarge: m1.xlarge/" onap_openstack.env
rm -rf onap onap.pub
ssh-keygen  -t rsa -N ''  -f onap
cp onap onap.pub /vagrant
pub_key=$(cat onap.pub)
sed -i "s,pub_key:.*,pub_key: $pub_key," onap_openstack.env
tenant=$(openstack project list -f value | grep " admin" | cut -f1 -d' ')
sed -i  "s/openstack_tenant_id:.*/openstack_tenant_id: $tenant/" onap_openstack.env
sed -i  "s/openstack_username:.*/openstack_username: admin/" onap_openstack.env
sed -i  "s/openstack_api_key:.*/openstack_api_key: apikey/" onap_openstack.env
sed -i  "s,horizon_url:.*,horizon_url: http://192.168.0.10/dashboard," onap_openstack.env
sed -i  "s,keystone_url:.*,keystone_url: http://192.168.0.10/identity/,"  onap_openstack.env

# Network parameters
sed -i  "s/dns_list:.*/dns_list: 8.8.8.8/" onap_openstack.env
sed -i  "s/external_dns:.*/external_dns: 8.8.8.8/" onap_openstack.env

openstack stack delete --yes --wait ONAP || true
openstack stack create -t onap_openstack.yaml -e onap_openstack.env ONAP

sleep 300
sudo sed -i "/.*simpledemo.openecomp.org.*/d" /etc/hosts
vms=$(grep "_vm:" onap_openstack.yaml | cut -f1 -d"_")
for vm in $vms
do
    ip=$(openstack server list --name $vm -f yaml | grep Networks | cut -f2 -d",")
    echo "$ip $vm.api.simpledemo.openecomp.org" | sudo tee -a /etc/hosts
done
ssh -o StrictHostKeyChecking=no ubuntu@portal.api.simpledemo.openecomp.org -i onap "curl sina.com.cn"
