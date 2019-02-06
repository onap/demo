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
#
### Edit ansible/vars.yml Kolla Values and ansible/inventory/onap-dev-env Hosts
### Install Kolla
   `ansible-playbook ansible/site.yml -i ansible/inventory/onap-dev-env --extra-vars "@ansible/vars.yml" --tags "kolla"`
### Edit ansible/vars.yml ONAP Values
### Install ONAP
   `ansible-playbook ansible/site.yml -i ansible/inventory/onap-dev-env --extra-vars "@ansible/vars.yml" --tags "onap"`

