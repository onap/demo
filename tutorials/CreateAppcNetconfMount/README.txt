Tutorial for SDNC

Demonstrates creating the Netconf mount in APPC for the vFW traffic generator from SDNC while processing the VNF_TOPOLOGY_ACTIVE directed graph.

The vFW demo use case in ONAP requires APPC to have a Netconf Mount to the traffic generator. That mount is created by a step in the robot framework but is actually an interesting way to demonstrate capabilities in SDNC  since the data needed to create the netconf mount point all exists in SDNC. This also demonstrates the type of changes that can be done outside of an SDNC release with just Directed Graph changes.
This tutorial starts with a running ONAP platform configuration and would enhance the existing vFW use case. Instructions and background on the ONAP installation, use case, and interaction are at https://wiki.onap.org/display/DW/Installing+and+Running+the+ONAP+Demos. 


Directed Graphs for the Tutorial:
	VNF-API_vnf-topology-activate.json
	VNF-API_vnf-topology-delete.json

RestAPICallNode Template:
	netconf-mount-template.xml

SLIAPI:ExecuteGraph input payload for testing:
	slitester.vnf-topology-activate.json

