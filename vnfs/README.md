OpenECOMP DEMO
===

This repository contains the source code and scripts to run the vFirewall and vLoadBalancer/vDNS demo applications for OpenECOMP.

The repository includes:

  - README.md: this file.

  - LICENSE.TXT: the license text.

  - honeycomb_plugin: Honeycomb plugin that allows upstream systems to change VNF configuration via RESTCONF or NETCONF protocols.

  - VES: source code of the ECOMP Vendor Event Listener (VES) Library. The VES library used here has been cloned from the GitHub repository at https://github.com/att/evel-library on February 1, 2017.

  - VESreporting_vFW: VES client for vFirewall demo application.

  - VESreporting_vLB: VES client for vLoadBalancer/vDNS demo application.

  - vFW: scripts that download, install and run packages for the vFirewall demo application.

  - vLB: scripts that download, install and run packages for the vLoadBalancer/vDNS demo application.


Usage
===

The content of this repository is downloaded and run automatically when the VNFs are instantiated. The user is not supposed to manually download and install the software contained in this repository.

Two demo applications, vFirewall and vLoadBalancer/vDNS are included. 

vFirewall
---

The vFirewall app contains 3 VMs: a firewall, a packet generator, and a packet sink. 

The packet generator sends packets to the packet sink through the firewall. The firewall reports the volume of traffic from the packet generator to the sink to OpenECOMP DCAE’s collector. To check the traffic volume to the sink, you can access the link http://sink_IP:667 through your browser. You can see the traffic volume in the charts. 

The packet generator includes a script that periodically generates different volumes of traffic. 

The closedloop policy has been configured to re-adjust the traffic volume when it is needed.

__Closedloop for vFirewall demo:__

Through the OpenECOMP Portal’s Policy Portal, we can find the configuration and operation policies that is currently enabled for the vFirewall application. 
+ The configuration policy sets the thresholds for generating an onset event from DCAE to the Policy engine. Currently the threshold is set to below 300 packets or above 700 packets per 10 seconds. 
+ Once the threshold is crossed, the Policy engine executes the operational policy to request APP-C to change the configuration of the packet gen. 
+ APP-C sends a request to the packet generator to adjust the traffic volume to 500 packets per 10 seconds. 
+ The traffic volume can be observed through the link http://sink_IP:667.

__Adjust packet generator:__

The packet generator contains 10 streams: fw_udp1, fw_udp2, fw_udp3,...fw_udp10. Each stream generates 100 packets per 10 seconds. 

To enable a stream, include *{"id":"fw_udp1", "is-enabled":"true"}* in the *pg-stream* bracket. 

To adjust the traffic volume sending from the packet generator, run the following command that enable 5 streams in a shell with localhost or the correct packet generator IP address in the http argument:

```
curl -X PUT -H "Authorization: Basic YWRtaW46YWRtaW4=" -H "Content-Type: application/json" -H "Cache-Control: no-cache" -d '{"pg-streams":{"pg-stream": [{"id":"fw_udp1", "is-enabled":"true"},{"id":"fw_udp2", "is-enabled":"true"},{"id":"fw_udp3", "is-enabled":"true"},{"id":"fw_udp4", "is-enabled":"true"},{"id":"fw_udp5", "is-enabled":"true"}]}}' "http://PacketGen_IP:8183/restconf/config/sample-plugin:sample-plugin/pg-streams"
```

A script in /opt/run_traffic_fw_demo.sh on the packet generator VM starts automatically and alternate the volume of traffic every 10 minutes. 

vLoadBalancer/vDNS
---

The vLoadBalancer/vDNS app contains 2 VMs in the base model: a load balancer and a DNS instance. When there are too many DNS queries, the Closedloop is triggered and a new DNS instance will be spun up. 

To test the app, in the command prompt:

``` 
# nslookup host1.dnsdemo.openecomp.org *vLoadBalancer_IP*

Server:     *vLoadBalancer_IP*
Address:    *vLoadBalancer_IP*

Name:     host1.dnsdemo.openecomp.org
Address:  10.0.100.101

```

That means the load balancer has been set up correctly and has forwarded the DNS queries to the DNS instance. 

__Closedloop for vLoadBalancer/vDNS:__

Through the OpenECOMP Portal’s Policy Portal, we can find the configuration and operation policies that is currently enabled for the vLoadBalancer/vDNS application. 
+ The configuration policy sets the thresholds for generating an onset event from DCAE to the Policy engine. Currently the threshold is set to above 200 packets per 10 seconds. 
+ Once the threshold is crossed, the Policy engine executes the operational policy to query A&AI and send a request to MSO for spinning up a new DNS instance. 
+ A new DNS instance will be spun up after the threshold is crossed. It can take a few minutes for this to happen. 


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
+ *{"id":"dns1", "is-enabled":"true"}* shows the stream *dns1* is enabled. The packet gen sends requests in the rate of 100 packets per 10 seconds.  

+ To increase the amount of traffic, we can enable more streams. The packet gen has total 10 streams, *dns1*, *dns2*, *dns3* to *dns10*. Each of them produces 100 packets per 10 seconds. To enable the streams, includes *{"id":"dnsX", "is-enabled":"true"}* where *X* is the stream ID in the pg-stream bracket of the curl command.  
For example, if we want to enable 3 streams, the curl command should be:

```
curl -X PUT -H "Authorization: Basic YWRtaW46YWRtaW4=" -H "Content-Type: application/json" -H "Cache-Control: no-cache" -d '{"pg-streams":{"pg-stream": [{"id":"dns1", "is-enabled":"true"}, {"id":"dns2", "is-enabled":"true"},{"id":"dns3", "is-enabled":"true"}]}}' "http://vLoadBalancer_IP:8183/restconf/config/sample-plugin:sample-plugin/pg-streams"
```

