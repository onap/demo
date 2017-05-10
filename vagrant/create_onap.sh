#!/usr/bin/env bash

source devstack/openrc admin
cd /vagrant/resource
pub_net=$(openstack network list -f value|grep public | cut -f1 -d' ')
sed -i  "s/public_net_id:.*/public_net_id: $pub_net/" onap_openstack.env
tenant=$(openstack project list -f value | grep " admin" | cut -f1 -d' ')
sed -i  "s/openstack_tenant_id:.*/openstack_tenant_id: $tenant/" onap_openstack.env
openstack stack create -t onap_openstack.yaml -e onap_openstack.env ONAP
