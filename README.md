Content
---

The Demo repository contains the HEAT templates and scripts for the instantiation of the ONAP platform and use cases. The repository includes:

 - README.md: this file.
 
 - LICENSE.TXT: the license text.
 
 - pom.xml: POM file used to build the software hosted in this repository.
 
 - version.properties: current version number of the Demo repository. Format: MAJOR.MINOR.PATCH (e.g. 1.3.0)
 
 - The "boot" and "heat/ONAP/cloud-config" directories contain the scripts that install and configure ONAP. This separation is due to size limits imposed by importing files into a VM for cloud-init. The two directories contain:
    - install.sh: sets up the host VM for specific components. This script runs only once, soon after the VM is created.
    - vm\_init.sh: contains component-specific configuration, downloads and runs docker containers. For some components, this script may either call a component-specific script (cloned from Gerrit repository) or call docker-compose.
    - serv.sh: it is installed in /etc/init.d, calls vm\_init.sh at each VM (re)boot.
    - configuration files for the Bind DNS Server installed with ONAP. Currently, both simpledemo.openecomp.org and simpledemo.onap.org domains are supported.
    - sdc\_ext\_volume_partitions.txt: file that contains external volume partitions for SDC.
    
 - The "boot" directory also contains a "robot" sub-directory that includes scripts to run Robot from the VM.
 
 - The "docker\_update\_scripts" directory contains scripts that update all the docker containers of an ONAP instance (NOT UPDATED SINCE AMSTERDAM RELEASE).
 
 - The "heat" directory contains the following sub-directories:

    - OAM-Network: contains the Heat files for creating the ONAP private management network. This is required only if that network is needed out of the Heat stack.
 
 	- ONAP: contains the HEAT files for the installation of the ONAP platform.
 	
 	- vCPE: contains sub-directories with HEAT templates for the installation of vCPE Infrastructure (Radius Server, DHCP, DNS, Web Server), vBNG, vBRG Emulator, vGMUX, and vGW.
 	
 	- vFW: contains the HEAT template for the instantiation of the vFirewall VNF (base\_vfw.yaml) and the environment file (base\_vfw.env). This template is used for testing and demonstrating VNF instantiation only (no closed-loop).
 	
 	- vFWCL: contains two sub-directories, one that hosts the HEAT template for the vFirewall and vSink (vFWSNK/base\_vfw.yaml), and one that hosts the HEAT template for the vPacketGenerator (vPKG/base\_vpkg.yaml). These templates are used for testing and demonstrating VNF instantiation and closed-loop.
 	
 	- vLB: contains the HEAT template for the instantiation of the vPacketGenerator/vLoadBalancer/vDNS VNF (base\_vlb.yaml) and the environment file (base\_vlb.env). The directory also contains the HEAT template for the DNS scaling-up scenario (dnsscaling.yaml) with its environment file (dnsscaling.env).
 	
 	- vVG: contains the HEAT template for the instantiation of a volume group (base\_vvg.yaml and base\_vvg.env).
 
 - The "scripts" directory contains the deploy.sh script that uploads software artifacts to the Nexus repository during the build process.
 
 - The "tosca" directory contains an example of the TOSCA model of the vCPE infrastructure.
 
 - The "tutorials" directory contains tutorials for Clearwater\_IMS and for creating a Netconf mount point in APPC. The "VoLTE" sub-directory is currently not used.
 
 - The "vagrant" directory contains the scripts that install ONAP using Vagrant (NOT UPDATED SINCE AMSTERDAM RELEASE).
 	
 - The "vnfs" directory: contains the following directories:
 
 	- honeycomb_plugin: Honeycomb plugin that allows ONAP to change VNF configuration via RESTCONF or NETCONF protocols.
 	
 	- vCPE: contains sub-directories with the scripts that install all the components of the vCPE use case.
 	
 	- VES: source code of the ONAP Vendor Event Listener (VES) Library. The VES library used here has been cloned from the GitHub repository at https://github.com/att/evel-library on February 1, 2017. (DEPRECATED SINCE AMSTERDAM RELEASE)
 	
 	- VESreporting_vFW: VES client for vFirewall demo application. (DEPRECATED SINCE AMSTERDAM RELEASE)
 	
 	- VESreporting_vLB: VES client for vLoadBalancer/vDNS demo application. (DEPRECATED SINCE AMSTERDAM RELEASE)
 	
 	- VES5.0: source code of the ONAP Vendor Event Listener (VES) Library, version 5.0. (CURRENTLY SUPPORTED)
 	
 	- VESreporting_vFW5.0: VES v5.0 client for vFirewall demo application. (CURRENTLY SUPPORTED)
 	
 	- VESreporting_vLB5.0: VES v5.0 client for vLoadBalancer/vDNS demo application. (CURRENTLY SUPPORTED)
 	
 	- vFW: scripts that download, install and run packages for the vFirewall use case.
 	
 	- vLB: scripts that download, install and run packages for the vLoadBalancer/vDNS use case.
 	
 	- vLBMS: scripts that download, install and run packages for the vLoadBalancer/vDNS used for the Scale Out use case.
 

ONAP Installation in OpenStack Clouds via HEAT Template
---

The ONAP HEAT template spins up the entire ONAP platform in OpenStack-based clouds. The template, onap\_openstack.yaml, comes with an environment file, onap\_openstack.env, in which all the default values are defined.

The HEAT template is composed of two sections: (i) parameters, and (ii) resources.

 - The "parameters" section contains the declarations and descriptions of the parameters that will be used to spin up ONAP, such as public network identifier, URLs of code and artifacts repositories, etc. The default values of these parameters can be found in the environment file.

 - The "resources" section contains the definitions of:
   - ONAP Private Management Network, which is used by ONAP components to communicate with each other and with VNFs
   - ONAP Virtual Machines (VMs)
   - Public/private key pair used to access ONAP VMs
   - Virtual interfaces towards the ONAP Private Management Network
   - Disk volumes.

Each VM specification includes Operating System image name, VM size (i.e. flavor), VM name, etc. Each VM has a virtual network interface with a private IP address in the ONAP Private Management network and a floating IP that OpenStack assigns based on availability. 
Furthermore, each VM runs an install.sh script that downloads and installs software dependencies (e.g. Java JDK, gcc, make, Python, ...). install.sh finally calls vm_init.sh that downloads docker containers from remote repositories and runs them.

When the HEAT template is executed, the OpenStack HEAT engine creates the resources defined in the HEAT template, based on the parameter values defined in the environment file.

Before running HEAT, it is necessary to customize the environment file. Indeed, some parameters, namely public\_net\_id, pub\_key, openstack\_tenant\_id, openstack\_username, and openstack\_api\_key, need to be set depending on the user's environment:

        public_net_id:       PUT YOUR NETWORK ID/NAME HERE
        pub_key:             PUT YOUR PUBLIC KEY HERE
        openstack_tenant_id: PUT YOUR OPENSTACK PROJECT ID HERE
        openstack_username:  PUT YOUR OPENSTACK USERNAME HERE
        openstack_api_key:   PUT YOUR OPENSTACK PASSWORD HERE
        horizon_url:         PUT THE HORIZON URL HERE
        keystone_url:        PUT THE KEYSTONE URL HERE (do not include version number)


openstack\_region parameter is set to RegionOne (OpenStack default). If your OpenStack is using another Region, please modify this parameter.

public\_net\_id is the unique identifier (UUID) or name of the public network of the cloud provider. To get the public\_net\_id, use the following OpenStack CLI command (ext is the name of the external network, change it with the name of the external network of your installation) 

        openstack network list | grep ext | awk '{print $2}'

pub\_key is the string value of the public key that will be installed in each ONAP VM. To create a public/private key pair in Linux, please execute the following instruction:
   
        user@ubuntu:~$ ssh-keygen -t rsa

The following operations to create the public/private key pair occur:
   
        Generating public/private rsa key pair.
        Enter file in which to save the key (/home/user/.ssh/id_rsa): 
        Created directory '/home/user/.ssh'.
        Enter passphrase (empty for no passphrase): 
        Enter same passphrase again: 
        Your identification has been saved in /home/user/.ssh/id_rsa.
        Your public key has been saved in /home/user/.ssh/id_rsa.pub.

openstack\_username, openstack\_tenant\_id (password), and openstack\_api\_key are the user's credentials to access the OpenStack-based cloud.

Some global parameters used for all components are also required:

        ubuntu_1404_image: PUT THE UBUNTU 14.04 IMAGE NAME HERE
        ubuntu_1604_image: PUT THE UBUNTU 16.04 IMAGE NAME HERE
        flavor_small: PUT THE SMALL FLAVOR NAME HERE
        flavor_medium: PUT THE MEDIUM FLAVOR NAME HERE
        flavor_large: PUT THE LARGE FLAVOR NAME HERE
        flavor_xlarge: PUT THE XLARGE FLAVOR NAME HERE

To get the images in your OpenStack environment, use the following OpenStack CLI command:

        openstack image list | grep 'ubuntu'

To get the flavor names used in your OpenStack environment, use the following OpenStack CLI command:

        openstack flavor list

Some network parameters must be configured:
        
        dns_list: PUT THE ADDRESS OF THE EXTERNAL DNS HERE (e.g. a comma-separated list of IP addresses in your /etc/resolv.conf in UNIX-based Operating Systems). 
        external_dns: PUT THE FIRST ADDRESS OF THE EXTERNAL DNS LIST HERE (THIS WILL BE DEPRECATED SOON)
        dns_forwarder: PUT THE IP OF DNS FORWARDER FOR ONAP DEPLOYMENT'S OWN DNS SERVER
        oam_network_cidr: 10.0.0.0/16

ONAP installs a DNS server used to resolve IP addresses in the ONAP OAM private network. Originally, dns\_list and external\_dns were both used to circumvent some limitations of older OpenStack versions.

DCAE requires a parameter called dcae\_deployment\_profile. It accepts one of the following values:
 - R3MVP: Installs only the basic DCAE functionalities that will support the vFW/vDNS, vCPE and vVoLTE use cases;
 - R3: Full DCAE installation;
 - R3PLUS: This profile deploys the DCAE R3 stretch goal service components.

The recommended DCAE profile for Casablanca Release is R3. For more information about DCAE deployment with HEAT, please refer to the ONAP documentation: https://onap.readthedocs.io/en/latest/submodules/dcaegen2.git/docs/sections/installation_heat.html

The ONAP platform can be instantiated via Horizon (OpenStack dashboard) or Command Line.

Instantiation via Horizon:

 - Login to Horizon URL with your personal credentials
 - Click "Stacks" from the "Orchestration" menu
 - Click "Launch Stack"
 - Paste or manually upload the HEAT template file (onap\_openstack.yaml) in the "Template Source" form
 - Paste or manually upload the HEAT environment file (onap\_openstack.env) in the "Environment Source" form
 - Click "Next"
 - Specify a name in the "Stack Name" form
 - Provide the password in the "Password" form
 - Click "Launch" 

Instantiation via Command Line:

 - Install the HEAT client on your machine, e.g. in Ubuntu (ref. http://docs.openstack.org/user-guide/common/cli-install-openstack-command-line-clients.html):

        apt-get install python-dev python-pip
        pip install python-heatclient        # Install heat client
        pip install python-openstackclient   # Install the Openstack client to support multiple services
 
 - Create a file (named i.e. ~/openstack/openrc) that sets all the environmental variables required to access the OpenStack platform:

        export OS_AUTH_URL=INSERT THE AUTH URL HERE
        export OS_USERNAME=INSERT YOUR USERNAME HERE
        export OS_TENANT_ID=INSERT YOUR TENANT ID HERE
        export OS_REGION_NAME=INSERT THE REGION HERE
        export OS_PASSWORD=INSERT YOUR PASSWORD HERE
   
   Alternatively, you can download the OpenStack RC file from the dashboard: Compute -> Access & Security -> API Access -> Download RC File
   
 - Source the script or RC file from command line:

        source ~/openstack/openrc
        
 - In order to install the ONAP platform, type:

        openstack stack create -t PATH_TO_HEAT_TEMPLATE(YAML FILE) -e PATH_TO_ENV_FILE STACK_NAME  # New Openstack client, OR
        heat stack-create STACK_NAME -f PATH_TO_HEAT_TEMPLATE(YAML FILE) -e PATH_TO_ENV_FILE       # Old HEAT client


vFirewall Use Case
---

The use case is composed of three virtual functions (VFs): packet generator, firewall, and traffic sink. These VFs run in three separate VMs. The packet generator sends packets to the packet sink through the firewall. The firewall reports the volume of traffic passing though to the ONAP DCAE collector. To check the traffic volume that lands at the sink VM, you can access the link http://sink\_ip\_address:667 through your browser and enable automatic page refresh by clicking the "Off" button. You can see the traffic volume in the charts.

The packet generator includes a script that periodically generates different volumes of traffic. The closed-loop policy has been configured to re-adjust the traffic volume when high-water or low-water marks are crossed.

__Closed-Loop for vFirewall demo:__

Through the ONAP Portal's Policy Portal, we can find the configuration and operation policies that are currently enabled for the vFirewall use case. 

- The configuration policy sets the thresholds for generating an onset event from DCAE to the Policy engine. Currently, the high-water mark is set to 700 packets while the low-water mark is set to 300 packets. The measurement interval is set to 10 seconds.
- When a threshold is crossed (i.e. the number of received packets is below 300 packets or above 700 packets per 10 seconds), the Policy engine executes the operational policy to request APPC to adjust the traffic volume to 500 packets per 10 seconds.
- APPC sends a request to the packet generator to adjust the traffic volume.
- Changes to the traffic volume can be observed through the link http://sink\_ip\_address:667.

__Adjust packet generator:__

The packet generator contains 10 streams: fw\_udp1, fw\_udp2, fw\_udp3, . . . , fw\_udp10. Each stream generates 100 packets per 10 seconds. A script in /opt/run\_traffic\_fw\_demo.sh on the packet generator VM starts automatically and alternates high traffic (i.e. 10 active streams at the same time) and low traffic (1 active stream) every 5 minutes.

To enable a stream, include *{"id":"fw_udp1", "is-enabled":"true"}* in the *pg-stream* bracket. 

To adjust the traffic volume produced by the packet generator, run the following command in a shell, replacing PacketGen_IP in the HTTP argument with localhost (if you run it in the packet generator VM) or the packet generator IP address:


    curl -X PUT -H "Authorization: Basic YWRtaW46YWRtaW4=" -H "Content-Type: application/json" -H "Cache-Control: no-cache" -d '{"pg-streams":{"pg-stream": [{"id":"fw_udp1", "is-enabled":"true"},{"id":"fw_udp2", "is-enabled":"true"},{"id":"fw_udp3", "is-enabled":"true"},{"id":"fw_udp4", "is-enabled":"true"},{"id":"fw_udp5", "is-enabled":"true"}]}}' "http://PacketGen_IP:8183/restconf/config/sample-plugin:sample-plugin/pg-streams"

The command above enables 5 streams.


vLoadBalancer/vDNS Use Case (old scale out use case)
---

The use case is composed of three VFs: packet generator, load balancer, and DNS server. These VFs run in three separate VMs. The packet generator issues DNS lookup queries that reach the DNS server via the load balancer. DNS replies reach the packet generator via the load balancer as well. The load balancer reports the average amount of traffic per DNS over a time interval to the DCAE collector. When the average amount of traffic per DNS server crosses a predefined threshold, the closed-loop is triggered and a new DNS server is instantiated. 

To test the application, you can run a DNS query from the packet generator VM:

    dig @vLoadBalancer_IP host1.dnsdemo.onap.org

The output below means that the load balancer has been set up correctly, has forwarded the DNS queries to one DNS instance, and the packet generator has received the DNS reply message. 
 
    ; <<>> DiG 9.10.3-P4-Ubuntu <<>> @192.168.9.111 host1.dnsdemo.onap.org
    ; (1 server found)
    ;; global options: +cmd
    ;; Got answer:
    ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 31892
    ;; flags: qr aa rd; QUERY: 1, ANSWER: 1, AUTHORITY: 1, ADDITIONAL: 2
    ;; WARNING: recursion requested but not available
    
    ;; OPT PSEUDOSECTION:
    ; EDNS: version: 0, flags:; udp: 4096
    ;; QUESTION SECTION:
    ;host1.dnsdemo.onap.org.		IN	A
    
    ;; ANSWER SECTION:
    host1.dnsdemo.onap.org.	604800	IN	A	10.0.100.101
    
    ;; AUTHORITY SECTION:
    dnsdemo.onap.org.	604800	IN	NS	dnsdemo.onap.org.
    
    ;; ADDITIONAL SECTION:
    dnsdemo.onap.org.	604800	IN	A	10.0.100.100
    
    ;; Query time: 0 msec
    ;; SERVER: 192.168.9.111#53(192.168.9.111)
    ;; WHEN: Fri Nov 10 17:39:12 UTC 2017
    ;; MSG SIZE  rcvd: 97
 

__Closedloop for vLoadBalancer/vDNS:__

Through the Policy Portal (accessible via the ONAP Portal), we can find the configuration and operation policies that are currently enabled for the vLoadBalancer/vDNS application. 
+ The configuration policy sets the thresholds for generating an onset event from DCAE to the Policy engine. Currently, the threshold is set to 200 packets, while the measurement interval is set to 10 seconds. 
+ Once the threshold is crossed (e.g. the number of received packets is above 200 packets per 10 seconds), the Policy engine executes the operational policy. The Policy engine queries A&AI to fetch the VNF UUID and sends a request to SO to spin up a new DNS instance for the VNF identified by that UUID. 
+ SO spins up a new DNS instance.


To change the volume of queries generated by the packet generator, run the following command in a shell, replacing PacketGen_IP in the HTTP argument with localhost (if you run it in the packet generator VM) or the packet generator IP address:

    curl -X PUT -H "Authorization: Basic YWRtaW46YWRtaW4=" -H "Content-Type: application/json" -H "Cache-Control: no-cache" -d '{"pg-streams":{"pg-stream": [{"id":"dns1", "is-enabled":"true"}]}}' "http://PacketGen_IP:8183/restconf/config/sample-plugin:sample-plugin/pg-streams"  
 
+ *{"id":"dns1", "is-enabled":"true"}* shows the stream *dns1* is enabled. The packet generator sends requests in the rate of 100 packets per 10 seconds.  

+ To increase the amount of traffic, you can enable more streams. The packet generator has 10 streams, *dns1*, *dns2*, *dns3* to *dns10*. Each of them generates 100 packets per 10 seconds. To enable the streams, please add *{"id":"dnsX", "is-enabled":"true"}* to the pg-stream bracket of the curl command, where *X* is the stream ID.

For example, if you want to enable 3 streams, the curl command will be:

    curl -X PUT -H "Authorization: Basic YWRtaW46YWRtaW4=" -H "Content-Type: application/json" -H "Cache-Control: no-cache" -d '{"pg-streams":{"pg-stream": [{"id":"dns1", "is-enabled":"true"}, {"id":"dns2", "is-enabled":"true"},{"id":"dns3", "is-enabled":"true"}]}}' "http://PacketGen_IP:8183/restconf/config/sample-plugin:sample-plugin/pg-streams"

When the VNF starts, the packet generator is automatically configured to run 5 streams.

vVolumeGroup Use Case
---

The vVG directory contains the HEAT template (base\_vvg.yaml) and environment file (base\_vvg.env) used to spin up a volume group in OpenStack and attach it to an existing ONAP instance.

The HEAT environment file contains two parameters:

    volume_size: 100
    nova_instance: 1234456

volume\_size is the size (in gigabytes) of the volume group. nova\_instance is the name or UUID of the VM to which the volume group will be attached. This parameter should be changed appropriately.


VF Module Scale Out Use Case
---

The Scale Out use case shows how users/network operators can add capacity to an existing VNF. ONAP Casablanca release supports scale out with manual trigger from VID and closed-loop enabled automation from Policy. This is demonstrated against the vLB/vDNS VNFs. For Casablanca, both APPC and SDNC controllers are supported. APPC is the official controller used for this use case and it can be used to scale multiple VNF types. SDNC is experimental for now and it can scale only the vDNS VNF developed for ONAP.

This repository hosts the source code and scripts that implement the vLB/vDNS VNFs. The remainder of this section describes the use case at high level, using APPC as VNF controller. 

Scaling VF modules manually requires the user/network operator to trigger the scale out operation from the VID portal. VID translates the operation into a call to SO. Scaling VF modules in an automated manner instead requires the user/network operator to design and deploy a closed loop for scale out that includes policies (e.g. threshold-crossing conditions), guard policies that determine when it's safe to scale out, and microservices that analyze events coming from the network in order to discover situations. Both manual and automated scale out activate the scale out workflow in the Service Orchestrator (SO). The workflow runs as follows: 
 
 - SO sends a request to APPC to run health check against the VNF;
 - If the VNF is healthy, SO instantiates a new VF module and sends a request to APPC to reconfigure the VNF;
 - APPC reconfigures the VNF, without interrupting the service;
 - SO sends a request to APPC to run health check against the VNF again, to validate that the scale out operation didn't impact the running VNF.
	
For this use case, we created a new version of the vLB/vDNS, contained in vnfs/VLBMS. Unlike the vLB/vDNS VNF described before, in this modified version the vLB and the vDNS do not run any automated discovery service. Instead, the vLB has a Northbound API that allows an upstream system (e.g. ONAP) to change the internal configuration by updating the list of active vDNS instances (i.e. VNF reconfiguration). The Northbound API framework has been built using FD.io-based Honeycomb 1707, and supports both RESTconf and NETCONF protocols. Below is an example of vDNS instances contained in the vLB, in JSON format:

		{
    	"vlb-business-vnf-onap-plugin": {
      		"vdns-instances": {
        		"vdns-instance": [
          		{
          		"ip-addr": "192.168.10.211",
          		"oam-ip-addr": "10.0.150.2",
          		"enabled": true
          		},
          		{
          		"ip-addr": "192.168.10.212",
          		"oam-ip-addr": "10.0.150.4",
          		"enabled": true
          		}]
      		}
    	}
    }

The parameters required for VNF reconfiguration (i.e. "ip-addr", "oam-ip-addr", and "enabled" in case of vLB/vDNS) can be specified in the VID GUI when triggering the workflow manually or in CLAMP when designing a closed loop for the automated case. In both cases, the format used for specifying the parameters and their values is a JSON path. SO will use the provided paths to access parameters' name and value in the VF module preload received from SDNC before instantiating a new VF module.

VID accepts a JSON array in the "Configuration Parameter" box (see later), for example:

    [{"ip-addr":"$.vf-module-topology.vf-module-parameters.param[10].value","oam-ip-addr":"$.vf-module-topology.vf-module-parameters.param[15].value","enabled":"$.vf-module-topology.vf-module-parameters.param[22].value"}]
	
CLAMP, instead, accepts a YAML file in the "Payload" box in the Policy Creation form, for example:

    requestParameters: '{"usePreload":true,"userParams":[]}'
    configurationParameters: '[{"ip-addr":"$.vf-module-topology.vf-module-parameters.param[10].value","oam-ip-addr":"$.vf-module-topology.vf-module-parameters.param[15].value","enabled":"$.vf-module-topology.vf-module-parameters.param[22].value"}]'

Note that Policy requires an additional object, called "requestParameters" in which "usePreload" should be set to true and the "userParams" array should be left empty.

The JSON path to the parameters used for VNF reconfiguration, including array locations, should be set as described above. Finally, although the VNF supports to update multiple vDNS records in the same call, for Casablanca release APPC updates a single vDNS instance at a time.

When using APPC, before running scale out, the user needs to create a VNF template using the Controller Design Tool (CDT), a design-time tool that allows users to create and on-board VNF templates into the APPC. The template describes which control operation can be executed against the VNF (e.g. scale out, health check, modify configuration, etc.), the protocols that the VNF supports, port numbers, VNF APIs, and credentials for authentication. Being VNF agnostic, APPC uses these templates to "learn" about specific VNFs and the supported operations.  

CDT requires two input: 1) the list of parameters that APPC will receive (ip-addr, oam-ip-addr, enabled in the example above); 2) the VNF API that APPC will use to reconfigure the VNF.

Below is an example of the parameters file (yaml format), which we call parameters.yaml:

		version: V1
		vnf-parameter-list:
		- name: ip-addr
		  type: null
		  description: null
		  required: "true"
		  default: null
		  source: Manual
		  rule-type: null
		  request-keys: null
		  response-keys: null
		- name: oam-ip-addr
		  type: null
		  description: null
		  required: "true"
		  default: null
		  source: Manual
		  rule-type: null
		  request-keys: null
		  response-keys: null
		- name: enabled
		  type: null
		  description: null
		  required: "true"
		  default: null
		  source: Manual
		  rule-type: null
		  request-keys: null
		  response-keys: null

Here is an example of API for the vLB VNF used for this use case. We name the file after the vnf-type contained in SDNC (i.e. Vloadbalancerms..base_vlb..module-0.xml):

		<vlb-business-vnf-onap-plugin xmlns="urn:opendaylight:params:xml:ns:yang:vlb-business-vnf-onap-plugin">
    		<vdns-instances>
        		<vdns-instance>
            		<ip-addr>${ip-addr}</ip-addr>
            		<oam-ip-addr>${oam-ip-addr}</oam-ip-addr>
            		<enabled>${enabled}</enabled>
        		</vdns-instance>
    		</vdns-instances>
		</vlb-business-vnf-onap-plugin>

To create the VNF template in CDT, the following steps are required:
 - Connect to the CDT GUI: http://APPC-IP:8080 (in Heat-based ONAP deployments) or http://ANY-K8S-IP:30289 (in OOM/K8S-based ONAP deployments)
 - Click "My VNF" Tab. Create your user ID, if necessary
 - Click "Create new VNF" entering the VNF type as reported in VID or AAI, e.g. vLoadBalancerMS/vLoadBalancerMS 0
 - Select "ConfigScaleOut" action
 - Create a new template identifier using the vnf-type name in SDNC as template name, e.g. Vloadbalancerms..dnsscaling..module-1
 - Select protocol (Netconf-XML), VNF username (admin), and VNF port number (2831 for NETCONF)
 - Click "Parameter Definition" Tab and upload the parameters (.yaml) file
 - Click "Template Tab" and upload API template (.yaml) file
 - Click "Reference Data" Tab
 - Click "Save All to APPC"

For health check operation, we just need to specify the protocol, the port number and username of the VNF (REST, 8183, and "admin" respectively, in the case of vLB/vDNS) and the API. For the vLB/vDNS, the API is: 

    restconf/operational/health-vnf-onap-plugin:health-vnf-onap-plugin-state/health-check

Note that we don't need to create a VNF template for health check, so the "Template" flag can be set to "N". Again, the user has to click "Save All to APPC" to update the APPC database.

At this time, CDT doesn't allow users to provide VNF password from the GUI. To update the VNF password we need to log into the APPC Maria DB container and change the password manually:

    mysql -u sdnctl -p (type "gamma" when password is prompted)
    use sdnctl;
    UPDATE DEVICE_AUTHENTICATION SET PASSWORD='admin' WHERE VNF_TYPE='vLoadBalancerMS/vLoadBalancerMS 0'; (use your VNF type)

To trigger the scale out workflow manually, the user/network operator can log into VID from the ONAP Portal (demo/demo123456! as username/password), select "VNF Changes" and then the "New (+)" button. The user/network operator needs to fill in the "VNF Change Form" by selecting Subscriber, Service Type, NF Role, Model Version, VNF, Scale Out from the Workflow dropdown window, and insert the JSON path array described above in the "Configuration Parameter" box. After clicking "Next", in the following window the user/network operator has to select the VF Module to scale by clicking on the VNF and then on the appropriate VF Module checkbox. Finally, by clicking on the "Schedule" button, the scale out use case will run as described above.

Automated scale out requires the user to onboard a DCAE blueprint in SDC when creating the service. To design a closed loop for scale out, the user needs to access the CLAMP GUI (https://clamp.api.simpledemo.onap.org:30258/designer/index.html) and execute the following operations:
- Click the "Closed loop" dropdown window and select "Open CL"
- Select the closed loop model and click "OK"
- In the next screen, click the "Policy" box to create a policy for closed loop, including guard policies
- After creating the policies, click "TCA" and review the blueprint uploaded during service creation and distributed by SDC to CLAMP
- Click the "Manage" dropdown and then "Submit" to push the policies to the Policy Engine
- From the same "Manage" dropdown, click "Deploy" to deploy the TCA blueprint to DCAE

The vLB/vDNS VNF generates traffic and reports metrics to the VES collector in the DCAE platform. The number of incoming packets to the vLB is used to evaluate the policy defined for closed loop. If the provided threshold is crossed, DCAE generates an event that reaches the Policy Engine, which in turn activates the scale out closed loop described above.

For more information about scale out, known issues and resolution, and material used for running the use case, please look at the wiki page: https://wiki.onap.org/display/DW/Running+Scale+Out+Use+Case+for+Casablanca
 

ONAP Use Cases HEAT Templates
---

USE CASE VNFs SHOULD BE INSTANTIATED VIA ONAP. THE USER IS NOT SUPPOSED TO DOWNLOAD THE HEAT TEMPLATES AND RUN THEM MANUALLY.

The vFWCL directory contains two HEAT templates, one for creating a packet generator (vPKG/base\_vpkg.yaml) and one for creating a firewall and a packet sink (vFWSNK/base\_vfw.yaml). This use case supports VNF onboarding, instantiation, and closed-loop. The vFW directory, instead, contains a single HEAT template (base\_vfw) that spins up the three VFs. This use case supports VNF onboarding and instantiation only (no support for closed-loop). For Amsterdam Release, the HEAT templates in vFWCL are recommended, so that users can test and demonstrate the entire ONAP end-to-end flow.

The vLB directory contains a base HEAT template (base\_vlb.yaml) that install a packet generator, a load balancer, and a DNS instance, plus another HEAT template (dnsscaling.yaml) for the DNS scaling scenario, in which another DNS server is instantiated.

Before onboarding the VNFs in SDC, the user should set the following values in the HEAT environment files:

    image_name: PUT THE VM IMAGE NAME HERE
    flavor_name: PUT THE VM FLAVOR NAME HERE
    public_net_id: PUT THE PUBLIC NETWORK ID HERE
    dcae_collector_ip: PUT THE ADDRESS OF THE DCAE COLLECTOR HERE (NOTE: this is not required for vFWCL/vPKG/base\_vpkg.env)
    pub_key: PUT YOUR KEY HERE
    cloud_env: PUT openstack OR rackspace HERE
    
image\_name, flavor\_name, \public\_net\_id, and pub\_key can be obtained as described in the ONAP Section. For deployment in OpenStack, cloud\_env must be openstack.

The DNS scaling HEAT environment file for the vLoadBalancer use case also requires you to specify the private IP of the load balancer, so that the DNS can connect to the vLB:
    
    vlb_private_ip_1: PUT THE PRIVATE ADDRESS OF THE VLB IN THE ONAP NETWORK SPACE HERE

As an alternative, it is possible to set the HEAT environment variables after the VNF is onboarded via SDC by appropriately preloading data into SDNC. That data will be fetched and used by SO to overwrite the default parameters in the HEAT environment file before the VNF is instantiated. For further information about SDNC data preload, please visit the wiki page: https://wiki.onap.org/display/DW/Tutorial_vIMS+%3A+SDNC+Updates
    
Each VNF has a MANIFEST.json file associated with the HEAT templates. During VNF onboarding, SDC reads the MANIFEST.json file to understand the role of each HEAT template that is part of the VNF (e.g. base template vs. non-base template). VNF onboarding requires users to create a zip file that contains all the HEAT templates and the MANIFEST file. To create the zip file, you can run the following command from shell:

    cd VNF_FOLDER  (this is the folder that contains the HEAT templates and the MANIFEST file)
    zip ZIP_FILE_NAME.zip *
    
For information about VNF onboarding via the SDC portal, please refer to the wiki page: https://wiki.onap.org/display/DW/Design

NF Change Management use case
---

For the Beijing release, we focused on in-place software upgrades, with vendor-specific details encapsulated in Ansible scripts provided by NF vendors. In-place software upgrades use direct communication between the controller (SDNC or APPC) and the NF instance to trigger the software upgrade, with the upgrade executing on the instance without relinquishing any of the physical resources. Both L1 - L3 and L4+ NFs are supported in the ONAP release via SDN-C and APP-C respectively. 
The change management workflow is defined as a composition of building blocks that include locking and unlocking the NF instance, executing health checks, and executing the software upgrade. 

 - The CM workflow for the in-place software upgrade is defined and executed by the service orchestrator (SO). 
 - A&AI is used to lock/unlock the NF instance 
 - The pre/post health checks and software upgrade execution are implemented in APPC (L4+ NFs) and SDNC (L1-L3 NFs) by leveraging Ansible services to communicate with the NF instances. 
 - The user (or, operator) interfaces with the CM workflow using ONAP's VID. SO communicates with A&AI using a REST API and with the controllers SDNC/APPC via DMaaP. 

We setup the use case demonstration for the software upgrade on the virtual gateway (vGW) as part of the vCPE use case in ONAP's Beijing release.
The main script for invoking SO in-place software upgrade workflow is in [demo.git]/vnfs/vCPE/scripts/inPlaceSoftwareUpgrade\_vGW.txt . The workflow can be tested without VID by using this script.
To execute the script, the user/operator would login to the SO container and copy/paste the script. One would have to install vim to edit the script and curl to execute commands within the script:

 - apt-get update 
 - apt-get install vim 
 - apt-get install curl 

Check in VID for the available instances - service ID and instance ID - and replace those IDs in the script. Since the use case is for vGW, the controller type is SDNC.

Next, the user/operator would login to the SDNC container and appropriately configure the Ansible playbooks:

 - Add the ssh key of the vGW on the Ansible server
 - Update VNF IP in DG config
 -- docker exec -it sdnc_controller_container bash 
 - Change the following line in /opt/onap/sdnc/data/properties/lcm-dg.properties with IP of VNF:
 -- ansible.nodelist=['10.12.5.85']
 - Update VNF IP in Ansible Server
 -- docker exec -ti sdnc_ansible_container /bin/bash
 -- Add VNF IP in /opt/onap/sdnc/Playbooks/Ansible_inventory
 - Update the Playbooks /opt/onap/sdnc/Playbooks
 -- ansible_upgradesw@0.00.yml
 -- ansible_precheck@0.00.yml
 -- ansible_postcheck@0.00.yml
