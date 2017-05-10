#!/usr/bin/env bash

set -e

sudo apt-get update -y
sudo apt-get install git -y
git clone https://github.com/openstack-dev/devstack
cd devstack; git checkout stable/newton
sudo apt-get install openvswitch-switch -y
sudo ovs-vsctl add-br br-ex
sudo ip addr flush enp0s9
sudo ovs-vsctl add-port br-ex enp0s9
