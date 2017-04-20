Content
---

This repository contains all the HEAT templates for the instantiation of the ONAP platform, and the vFirewall and vLoadBalancer/vDNS demo applications.

The repository includes:
 - README.md: this file
 
 - LICENSE.TXT: the license text
 
 - The "boot" directory: the scripts that instantiate ONAP. 
 
 - The "heat" directory: contains the following three directories:
 
 	- OpenECOMP: contains the HEAT template for the installation of the ONAP platform. The template openecomp_rackspace.yaml and the environment file openecomp_rackspace.env work in Rackspace, while the template onap_openstack.yaml and the environment file onap_openstack.env work in vanilla OpenStack.
 	
 	- vFW: contains the HEAT template for the instantiation of the vFirewall VNF (base_vfw.yaml) and the environment file (base_vfw.env)
 	
 	- vLB: contains the HEAT template for the instantiation of the vLoadBalancer/vDNS VNF (base_vlb.yaml) and the environment file (base_vlb.env). The folder also contains the HEAT template for the DNS scaling-up scenario (dnsscaling.yaml) with its environment file (dnsscaling.env), and the HEAT template for the vLB packet generator (packet_gen_vlb.yaml) and its environment file (packet_gen_vlb.env).
 	
 - The "vnfs" directory: contains the following directories:
 
 	- honeycomb_plugin: Honeycomb plugin that allows ONAP to change VNF configuration via RESTCONF or NETCONF protocols.
 	
 	- VES: source code of the ECOMP Vendor Event Listener (VES) Library. The VES library used here has been cloned from the GitHub repository at https://github.com/att/evel-library on February 1, 2017.
 	
 	- VESreporting_vFW: VES client for vFirewall demo application.
 	
 	- VESreporting_vLB: VES client for vLoadBalancer/vDNS demo application.
 	
 	- vFW: scripts that download, install and run packages for the vFirewall demo application.
 	
 	- vLB: scripts that download, install and run packages for the vLoadBalancer/vDNS demo application.
 

ONAP HEAT Template for Rackspace
---

The ONAP HEAT template spins up the entire ONAP platform. The template, openecomp_rackspace.yaml, comes with an environment file, openecomp_rackspace.env, in which all the default values are defined.

The HEAT template is composed of two sections: (i) parameters, and (ii) resources. The parameter section contains the declaration and description of the parameters that will be used to spin up ONAP, such as public network identifier, URLs of code and artifacts repositories, etc.
The default values of these parameters can be found in the environment file. The resource section contains the definition of:
 - ONAP Private Management Network, which ONAP components use to communicate with each other and with VNFs
 - ONAP Virtual Machines (VMs)
 - Public/private key pair used to access ONAP VMs
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


pub_key is string value of the public key that will be installed in each ONAP VM. To create a public/private key pair in Linux, please execute the following instruction:
   
        user@ubuntu:~$ ssh-keygen -t rsa

The following operations to create the public/private key pair occur:
   
        Generating public/private rsa key pair.
        Enter file in which to save the key (/home/user/.ssh/id_rsa): 
        Created directory '/home/user/.ssh'.
        Enter passphrase (empty for no passphrase): 
        Enter same passphrase again: 
        Your identification has been saved in /home/user/.ssh/id_rsa.
        Your public key has been saved in /home/user/.ssh/id_rsa.pub.

openstack_username, openstack_tenant_id (password), and openstack_api_key are user's credentials to access the Openstack-based cloud. Note that in the Rackspace web interface, openstack_api_key can be found by clicking on the username on the top-right corner of the GUI and then "Account Settings".

DCAE spins up the data collection and analytics environment outside the HEAT template. This environment is composed of: 3-VM CDAP/Hadoop cluster, 1 VM for the DCAE data collector, and 1 VM for Postgres DB. DCAE needs to know where (i.e. Rackspace region) it has to spin up these VMs. Three parameters have to be setup to reflect the Rackspace region, namely dcae_zone, dcae_state and openstack_region. dcae_zone and dcae_state are used to compose the name of the VMs, so any meaningful name can be used. openstack_region, instead, represents the actual location, so Rackspace-specific values must be used: IAD, DFW, HKG, SYD. The example below shows a snippet of the HEAT environment file that instantiate ONAP in IAD region in Rackspace: 

        dcae_zone:        iad4
        dcae_state:       vi
        openstack_region: IAD

The ONAP platform can be instantiated via Rackspace GUI or command line.

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
        
 - In order to install the ONAP platform, type:

        heat stack-create STACK_NAME -f PATH_TO_HEAT_TEMPLATE(YAML FILE) -e PATH_TO_ENV_FILE       # Old HEAT client, OR
        openstack stack create -t PATH_TO_HEAT_TEMPLATE(YAML FILE) -e PATH_TO_ENV_FILE STACK_NAME  # New Openstack client


ONAP HEAT Template for vanilla OpenStack
---

The HEAT template for vanilla OpenStack is similar to the HEAT template for Rackspace. The main difference is the way resource-intensive VMs are defined. Unlike OpenStack, Rackspace requires to explicitly create a local disk for memory- or CPU-intensive VMs.

The HEAT template for vanilla OpenStack replicates typical application deployments in OpenStack. VMs have a private IP address in the ONAP Private Management Network space and use floating IP addresses. A router that connects the ONAP Private Management Network to the external network is also created.

In addition to the parameters described in the previous section, the HEAT template for vanilla OpenStack uses the following parameters to define the image name and flavor of a VM:

        ubuntu_1404_image: PUT THE UBUNTU 14.04 IMAGE NAME HERE
        ubuntu_1604_image: PUT THE UBUNTU 16.04 IMAGE NAME HERE
        flavor_small: PUT THE SMALL FLAVOR NAME HERE
        flavor_medium: PUT THE MEDIUM FLAVOR NAME HERE
        flavor_large: PUT THE LARGE FLAVOR NAME HERE
        flavor_xlarge: PUT THE XLARGE FLAVOR NAME HERE
  
Parameters for network configuration are also used:

        aai_float_ip
        appc_float_ip
        ...
        vid_float_ip
        
        external_dns
        oam_network_cidr
        aai_ip_addr
        appc_ip_addr
        ...
        vid_ip_addr
        
These parameters are used to configure the ONAP internal DNS VM. The external_dns parameter is a comma-separated list of IP addresses (they can be obtained from /etc/resolv.conf in many UNIX-based Operating Systems). The IP address of the ONAP VMs must comply with the oam_network_cidr parameter, and viceversa. Except for external_dns, the other network parameters are already set. They should work for many deployments. 


VNFs HEAT Templates
---

The HEAT templates for the demo applications are stored in heat/vFW and heat/vLB directories. 

vFW contains the HEAT template, base_vfw.yaml, and the environment file, base_vfw.env, that are used to instantiate a virtual firewall. The VNF is composed of three VMs:
  - Packet generator
  - Firewall
  - Sink

The packet generator generates traffic that passes through the firewall and reaches the sink. The firewall periodically reports the number of packets received in a unit of time to the DCAE collector. If the reported number of packets received on the firewall is above a high-water mark or below a low-water mark, ONAP will enforce a configuration change on the packet generator, reducing or augmenting the quantity of traffic generated, respectively.

vLB contains the HEAT template, base_vlb.yaml, and the environment file, base_vlb.env, that are used to spin up a virtual load balancer and a virtual DNS. vLB also contains the HEAT template, packet_gen_vlb.yaml, and the environment file packet_gen_vlb.env, of a packet generator that generates DNS queries.
The load balancer periodically reports the number of DNS query packets received in a time unit to the DCAE collector. If the reported number of received packets crosses a threshold, then ONAP will spin up a new DNS based on the dnsscaling.yaml HEAT template and dnsscaling.env to better balance the load of incoming DNS queries.

The vFW and vLB HEAT templates and environment files are onboarded into ONAP SDC and run automatically. The user is not required to run these templates manually.
However, before onboarding the templates following the instructions in the ONAP documentation, the user should set the following values in the environment files:

        public_net_id:       INSERT YOUR NETWORK ID/NAME HERE
        pub_key:             INSERT YOUR PUBLIC KEY HERE
        

ONAP Demo applications
---

Demo applications are installed and run automatically when the VNFs are instantiated. The user is not supposed to download and install the demo applications manually.

Two demo applications, vFirewall and vLoadBalancer/vDNS are included. 

vFirewall
---

The vFirewall application contains 3 VMs: a firewall, a packet generator, and a packet sink. 

The packet generator sends packets to the packet sink through the firewall. The firewall reports the volume of traffic from the packet generator to the sink to ONAP DCAE’s collector. To check the traffic volume to the sink, you can access the link http://sink_ip_address:667 through your browser. You can see the traffic volume in the charts. 

The packet generator includes a script that periodically generates different volumes of traffic. 

The closed-loop policy has been configured to re-adjust the traffic volume when it is needed.

__Closedloop for vFirewall demo:__

Through the ONAP Portal’s Policy Portal, we can find the configuration and operation policies that is currently enabled for the vFirewall application. 
+ The configuration policy sets the thresholds for generating an onset event from DCAE to the Policy engine. Currently the thresholds are set to 300 packets and 700 packets, while the measurement interval is set to 10 seconds. 
+ Once one of the thresholds is crossed (e.g. the number of received packets is below 300 packets or above 700 per 10 seconds), the Policy engine executes the operational policy to request APP-C to change the configuration of the packet generator. 
+ APP-C sends a request to the packet generator to adjust the traffic volume to 500 packets per 10 seconds. 
+ The traffic volume can be observed through the link http://sink_ip_address:667.

__Adjust packet generator:__

The packet generator contains 10 streams: fw_udp1, fw_udp2, fw_udp3,...fw_udp10. Each stream generates 100 packets per 10 seconds. 

To enable a stream, include *{"id":"fw_udp1", "is-enabled":"true"}* in the *pg-stream* bracket. 

To adjust the traffic volume sending from the packet generator, run the following command that enable 5 streams in a shell with localhost or the correct packet generator IP address in the http argument:

```
curl -X PUT -H "Authorization: Basic YWRtaW46YWRtaW4=" -H "Content-Type: application/json" -H "Cache-Control: no-cache" -d '{"pg-streams":{"pg-stream": [{"id":"fw_udp1", "is-enabled":"true"},{"id":"fw_udp2", "is-enabled":"true"},{"id":"fw_udp3", "is-enabled":"true"},{"id":"fw_udp4", "is-enabled":"true"},{"id":"fw_udp5", "is-enabled":"true"}]}}' "http://PacketGen_IP:8183/restconf/config/sample-plugin:sample-plugin/pg-streams"
```

A script in /opt/run_traffic_fw_demo.sh on the packet generator VM starts automatically and alternate the volume of traffic every 5 minutes. 

vLoadBalancer/vDNS
---

The vLoadBalancer/vDNS app contains 2 VMs in the base model: a load balancer and a DNS instance. When there are too many DNS queries, the closed-loop is triggered and a new DNS instance will be spun up. 

To test the application, in the command prompt:

``` 
# nslookup host1.dnsdemo.openecomp.org *vLoadBalancer_IP*

Server:     *vLoadBalancer_IP*
Address:    *vLoadBalancer_IP*

Name:     host1.dnsdemo.openecomp.org
Address:  10.0.100.101

```

That means the load balancer has been set up correctly and has forwarded the DNS queries to the DNS instance. 

__Closedloop for vLoadBalancer/vDNS:__

Through the Policy Portal (accessible via the ONAP Portal), we can find the configuration and operation policies that are currently enabled for the vLoadBalancer/vDNS application. 
+ The configuration policy sets the thresholds for generating an onset event from DCAE to the Policy engine. Currently the threshold is set to 200 packets, while the measurement interval is set to 10 seconds. 
+ Once the threshold is crossed (e.g. the number of received packets is above 200 packets per 10 seconds), the Policy engine executes the operational policy to query A&AI and send a request to MSO for spinning up a new DNS instance. 
+ A new DNS instance will be then spun up.


__Generate DNS queries:__

To generate DNS queries to the vLoadBalancer/vDNS instance, a separate packet generator is prepared for this purpose. 

1. Spin up the heat template in the repository: https://link_to_repo/demo/heat/vLB/packet_gen_vlb.yaml. 

2. Log in to the packet generator instance through ssh.

3. Change the IP address in the config file /opt/config/vlb_ipaddr.txt to the public IP address of the LoadBalancer instance. 

4. Execute the script /opt/vdnspacketgen_change_streams_ports.sh to restart sending the DNS queries to the new LoadBalancer address. 

5. To change the volume of queries, execute the following command in a command prompt with the updated vLoadBalancer_IP address or localhost in the http argument:
 
```  
curl -X PUT -H "Authorization: Basic YWRtaW46YWRtaW4=" -H "Content-Type: application/json" -H "Cache-Control: no-cache" -d '{"pg-streams":{"pg-stream": [{"id":"dns1", "is-enabled":"true"}]}}' "http://vLoadBalancer_IP:8183/restconf/config/sample-plugin:sample-plugin/pg-streams"  
```  
+ *{"id":"dns1", "is-enabled":"true"}* shows the stream *dns1* is enabled. The packet generator sends requests in the rate of 100 packets per 10 seconds.  

+ To increase the amount of traffic, we can enable more streams. The packet generator has 10 streams, *dns1*, *dns2*, *dns3* to *dns10*. Each of them generates 100 packets per 10 seconds. To enable the streams, please insert *{"id":"dnsX", "is-enabled":"true"}* where *X* is the stream ID in the pg-stream bracket of the curl command.  
For example, if we want to enable 3 streams, the curl command should be:

```
curl -X PUT -H "Authorization: Basic YWRtaW46YWRtaW4=" -H "Content-Type: application/json" -H "Cache-Control: no-cache" -d '{"pg-streams":{"pg-stream": [{"id":"dns1", "is-enabled":"true"}, {"id":"dns2", "is-enabled":"true"},{"id":"dns3", "is-enabled":"true"}]}}' "http://vLoadBalancer_IP:8183/restconf/config/sample-plugin:sample-plugin/pg-streams"
```

        