Content
---

This repository contains all the HEAT templates for the instantiation of the ONAP platform, and the vFirewall and vLoadBalancer/vDNS demo applications.

The repository includes:
 - README.md: this file
 - LICENSE.TXT: the license text
 - The "boot" directory: the scripts to instantiate ONAP. 
 - The "heat" directory: contains the following three directories
 	- OpenECOMP: contains the HEAT template for the installation of the ONAP platform (openecomp_rackspace.yaml) and the environment file (openecomp_rackspace.env)
 	- vFW: contains the HEAT template for the instantiation of the vFirewall VNF (base_vfw.yaml) and the environment file (base_vfw.env)
 	- vLB: contains the HEAT template for the instantiation of the vLoadBalancer/vDNS VNF (base_vlb.yaml) and the environment file (base_vlb.env). The folder also contains the HEAT template for the DNS scaling-up scenario (dnsscaling.yaml) with its environment file (dnsscaling.env), and the HEAT template for the vLB packet generator (packet_gen_vlb.yaml) and its environment file (packet_gen_vlb.env).
 - The "vnfs" directory: the scripts and source codes for running the two demo applications.
 


ONAP HEAT Template
---

The ONAP HEAT template spins up the entire ONAP platform. The template, openecomp_rackspace.yaml, comes with an environment file, openecomp_rackspace.env, in which all the default values are defined.

The HEAT template is composed of two sections: (i) parameters, and (ii) resources. The parameter section contains the declaration and description of the parameters that will be used to spin up OpenECOMP, such as public network identifier, URLs of code and artifacts repositories, etc.
The default values of these parameters can be found in the environment file. The resource section contains the definition of:
 - ONAP Private Management Network, which ONAP components use to communicate with each other and with VNFs
 - ONAP Virtual Machines (VMs)
 - Public/private key pair used to access OpenECOMP VMs
 - Virtual interfaces towards the ONAP Private Management Network
 - Disk volumes. 

Each VM specification includes Operating System image name, VM size (i.e. flavor), VM name, etc. Each VM has two virtual network interfaces: one towards the public network and one towards the ONAP Private Management network, as described above. 
Furthermore, each VM runs a post-instantiation script that downloads and installs software dependencies (e.g. Java JDK, gcc, make, Python, ...) and ONAP software packages and docker containers from remote repositories.

When the HEAT template is executed, the Openstack HEAT engine creates the resources defined in the HEAT template, based on the parameters values defined in the environment file.

Before running HEAT, it is necessary to customize the environment file. Indeed, some parameters, namely public_net_id, pub_key, openstack_tenant_id, openstack_username, and openstack_api_key, need to be set depending on the user's environment:

        public_net_id:       INSERT YOUR NETWORK ID/NAME HERE
        pub_key:             INSERT YOUR PUBLIC KEY HERE
        openstack_tenant_id: INSERT YOUR TENANT ID HERE
        openstack_username:  INSERT YOUR USERNAME HERE
        openstack_api_key:   INSERT YOUR API KEY HERE

public_net_id is the unique identifier (UUID) or name of the public network of the cloud provider. Note that for Rackspace template, this value is already set to
   
        00000000-0000-0000-0000-000000000000


pub_key is string value of the public key that will be installed in each OpenECOMP VM. To create a public/private key pair in Linux, execute the following instruction:
   
        user@ubuntu:~$ ssh-keygen -t rsa

The following operations to create the public/private key pair occur:
   
        Generating public/private rsa key pair.
        Enter file in which to save the key (/home/user/.ssh/id_rsa): 
        Created directory '/home/user/.ssh'.
        Enter passphrase (empty for no passphrase): 
        Enter same passphrase again: 
        Your identification has been saved in /home/user/.ssh/id_rsa.
        Your public key has been saved in /home/user/.ssh/id_rsa.pub.

openstack_username, openstack_tenant_id (password), and openstack_api_key are user's credentials to access the Openstack-based cloud. Note that in the Rackspace web interface, openstack_api_key can be found by clicking on the username and then "Account Settings".


The OpenECOMP platform can be instantiated via Rackspace GUI or command line.

Instantiation via Rackspace GUI:
 - Login to Rackspace with your personal credentials
 - Click "Stack Templates" from the "Orchestration" menu
 - Click "Create Custom Template"
 - Paste or manually upload the HEAT template (openecomp.yaml)
 - Specify a name for your template in the "Template Name" form
 - Click "Create Template and Launch Stack" at the bottom of the page
 - In the "Create Stack" page, specify a name for your stack in the "Stack Name" form and select the Rackspace Region
 - In the "Advanced Option" menu, insert the values of the parameters specified in the environment file (openecomp.env)
 - Click "Create Stack"


Instantiation via Command Line:
 - Install the HEAT client on your machine, e.g. in Ubuntu (ref. http://docs.openstack.org/user-guide/common/cli-install-openstack-command-line-clients.html):

        apt-get install python-dev python-pip
        pip install python-heatclient        # Install heat client
        pip install python-openstackclient   # Install the Openstack client to support multiple services
 
 - Create a file (named i.e. ~/rackspace/openrc) that sets all the environmental variables required to access Rackspace:

        export OS_AUTH_URL=https://identity.api.rackspacecloud.com/v2.0/
        export OS_USERNAME=INSERT YOUR USERNAME HERE
        export OS_TENANT_ID=INSERT YOUR TENANT ID HERE
        export OS_REGION_NAME=INSERT THE RACKSPACE REGION ID [IAD | DFW | SYD | HKG]
        export OS_PASSWORD=INSERT YOUR PASSWORD HERE
        
 - Run the script from command line:

        source ~/rackspace/openrc
        
 - In order to install the OpenECOMP platform, type:

        heat stack-create STACK_NAME -f PATH_TO_HEAT_TEMPLATE(YAML FILE) -e PATH_TO_ENV_FILE       # Old HEAT client, OR
        openstack stack create -t PATH_TO_HEAT_TEMPLATE(YAML FILE) -e PATH_TO_ENV_FILE STACK_NAME  # New Openstack client


VNFs HEAT Templates
---

The HEAT templates for the demo apps are stored in vFW and vLB directories. 

vFW contains the HEAT template, base_vfw.yaml, and the environment file, base_vfw.env, that are used to instantiate a virtual firewall. The VNF is composed of three VMs:
  - Packet generator
  - Firewall
  - Sink

The packet generator generates traffic that passes through the firewall and reaches the sink. The firewall periodically reports the number of packets received in a unit of time to the DCAE collector. If the reported number of packets received on the firewall is above a high-water mark or below a low-water mark, OpenECOMP will enforce a configuration change on the packet generator, reducing or augmenting the quantity of traffic generated, respectively.

vLB contains the HEAT template, base_vlb.yaml, and the environment file, base_vlb.env, that are used to spin up a virtual load balancer and a virtual DNS. vLB also contains the HEAT template, packet_gen_vlb.yaml, and the environment file packet_gen_vlb.env, of a packet generator that generates DNS queries.
The load balancer periodically reports the number of DNS query packets received in a time unit to the DCAE collector. If the reported number of received packets crosses a threshold, then OpenECOMP will spin up a new DNS based on the dnsscaling.yaml HEAT template and dnsscaling.env to better balance the load of incoming DNS queries.

The vFW and vLB HEAT templates and environment files are onboarded into ONAP SDC and run automatically. The user is not required to run these templates manually.
However, before onboarding the templates following the instructions in the ONAP documentation, the user should set the following values in the environment files:

        public_net_id:       INSERT YOUR NETWORK ID/NAME HERE
        pub_key:             INSERT YOUR PUBLIC KEY HERE