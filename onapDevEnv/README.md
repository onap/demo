# Run playbooks as follows:
### Edit ansible/vars.yml Kolla Values and ansible/inventory/onap-dev-env Hosts
### Install Kolla
   `ansible-playbook ansible/site.yml --extra-vars "@vars.yml" --tags "kolla"`
### Edit ansible/vars.yml ONAP Values
### Install ONAP
   `ansible-playbook ansible/site.yml --extra-vars "@vars.yml" --tags "onap"`