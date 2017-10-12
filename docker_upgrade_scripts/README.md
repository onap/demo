#Docker Upgrade Scripts

##Description
These scripts will upgrade docker images in all ONAP components 

##Setting UP

apt-get install python-pip

pip install Fabric

Download all the files and modify onap_docker_upgrade.conf with correct configuration based on infrastructure endpoints

[keystone_auth]
user = <user_name>
password = <password>
tenant = <tenant>
url = <keystone_url>

user: User name to access the tenant
Password: Password for the user authentication
tenant: Tenant information
url: keystone endpoint URL. Currently v2.0 is only supported

[onap]
instance_prefix = <name>

instance_prefix: ONAP instance prefix configured in heat template or VM name

Deployment type: 1-nic-nofloat, 1-nic-float, 2-nic
deployment_type = <type>

deployment_type: Based on infrastructure network setup for VMs

dcae_key_path = <key_for_dcae_component>
onap_key_path = <key_for_onap_component>

Run python onap_docker_upgrade.py

To run daily basis, add the script to cron job and redirect output to a log file
