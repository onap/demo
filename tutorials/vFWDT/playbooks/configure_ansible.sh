#!/bin/bash

# ============LICENSE_START=======================================================
# Copyright (C) 2019 Orange
# ================================================================================
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ============LICENSE_END=========================================================


ANSIBLE=`kubectl get pods -o go-template --template '{{range .items}}{{.metadata.name}}{{"\n"}}{{end}}' | grep appc-ansible`
echo $ANSIBLE

kubectl cp playbooks/onap.pem onap/$ANSIBLE:/opt/ansible-server/Playbooks/
echo "Key file uploaded"

cp workflow/Ansible_inventory playbooks/
kubectl cp playbooks/Ansible_inventory onap/$ANSIBLE:/opt/ansible-server/Playbooks/
echo "Ansible_inventory file uploaded"

kubectl exec -n onap $ANSIBLE -- chmod 400 /opt/ansible-server/Playbooks/onap.pem 
echo "Key file configured"

#kubectl exec -n onap $ANSIBLE -- sed -i 's#\(private_key_file *= *\).*#\1/opt/ansible-server/Playbooks/onap.pem#'  /etc/ansible/ansible.cfg
printf '[defaults]\nhost_key_checking = False\nprivate_key_file = /opt/ansible-server/Playbooks/onap.pem\n' > playbooks/ansible.cfg
kubectl cp playbooks/ansible.cfg onap/$ANSIBLE:/etc/ansible/
echo "Ansible conf modified"

kubectl exec -n onap $ANSIBLE -- ansible -i /opt/ansible-server/Playbooks/Ansible_inventory vpgn,vfw-sink -m ping
echo "Hosts PING test completed"

sudo kubectl cp playbooks/vfw-sink onap/$ANSIBLE:/opt/ansible-server/Playbooks/
echo "vFW-SINK Playbooks uploaded"

sudo kubectl cp playbooks/vpgn onap/$ANSIBLE:/opt/ansible-server/Playbooks/
echo "vPGN Playbooks uploaded"

APPCDB=`kubectl get pods -o go-template --template '{{range .items}}{{.metadata.name}}{{"\n"}}{{end}}' | grep appc-db-0`
echo $APPCDB

kubectl exec -n onap $APPCDB -- mysql -u sdnctl -pgamma sdnctl -e'SELECT * FROM DEVICE_AUTHENTICATION WHERE PROTOCOL LIKE "ANSIBLE";'
kubectl exec -n onap $APPCDB -- mysql -u sdnctl -pgamma sdnctl -e'UPDATE DEVICE_AUTHENTICATION SET URL = "http://appc-ansible-server:8000/Dispatch" WHERE PROTOCOL LIKE "ANSIBLE" AND PASSWORD IS NULL;'
kubectl exec -n onap $APPCDB -- mysql -u sdnctl -pgamma sdnctl -e'UPDATE DEVICE_AUTHENTICATION SET PASSWORD = "admin" WHERE PROTOCOL LIKE "ANSIBLE" AND PASSWORD IS NULL;'
kubectl exec -n onap $APPCDB -- mysql -u sdnctl -pgamma sdnctl -e'SELECT * FROM DEVICE_AUTHENTICATION WHERE PROTOCOL LIKE "ANSIBLE";'
echo "APPC database configured for LCM commands"
