# This Playbook supports deploying ONAP as a developer or testing environment on baremetal
# by deploying an OpenStack Cloud and deploying ONAP on top of that cloud using ONAP integration 
# deploy scripts.
#
# Cloud Deploy:  Kolla is an OpenStack Project used to containerize and deploy ONAP
# 
# ONAP Deploy: ONAP Integration Deploy Script is a script that uses an existing OpenStack Cloud
# to deploy a Rancher based Kubernetes Cluster and to deploy ONAP using OOM Helm Charts
#   
# Editing ansible/vars.yml file and ansible/inventory/onap-dev-env hosts file and running this ansible playbook simplifies
# the steps needed to deploy a fully functional testing or development environment for ONAP.
# 
##  Note: Enable/Disable baremetal bootstrap (install of prereq set of packagesprovisioning in ansible/vars.yml by 
# changing kolla_bootstrap values to <no|yes>
#
#
# Run playbooks as follows:
# Before running any playbook edit ansible/deploy-var.yml, ansible/inventory/deploy hosts and ssh-copy-id to each target host
#
#
### Kolla Stand-alone Instructions
### Edit ansible/kolla-vars.yml Kolla Values and ansible/inventory/deploy Hosts
### Install Kolla
   `ansible-playbook ansible/site.yml -i ansible/inventory/deploy --extra-vars "@ansible/kolla-vars.yml" --tags "kolla"`
   `kolla-ansible post-deploy`
   `cat /etc/kolla/admin-openrc.sh`  
#  copy admin-openrc.sh to one of the deployed to nodes
   `source /etc/kolla/admin-openrc.sh`
   `cd /usr/share/kolla`
   `./init-runonce`
#  login to horizon at dev_kolla_external_fqdn
#
#
### ONAP Stand-alone Instructions - requires working OpenStack cloud which can be deployed with Kolla above
### Edit ansible/onap-vars.yml ONAP Values
### Install ONAP
   `ansible-playbook ansible/site.yml -i ansible/inventory/deploy --extra-vars "@ansible/onap-vars.yml" --tags "onap"`
#
#
### KRD Stand-alone Instructions
### Edit ansible/krd-vars.yml KRD Values and ansible/inventory/deploy Hosts
### Install KRD
   `ansible-playbook ansible/site.yml -i ansible/inventory/deploy --extra-vars "@ansible/krd-vars.yml" --tags "krd"`

