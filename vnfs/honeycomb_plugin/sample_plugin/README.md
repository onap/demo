# Honeycomb plugin for VPP's packet generator plugin

This plugin provides REST API to enable/disable streams on the VPP's packet generator. 


Buid instructions:

From the sample_pluging folder execute: mvn -s settings.xml clean install.


Run instruction:

From the sample_pluging folder execute: ./sample-distribution/target/sample-distribution-1.0.0-hc/sample-distribution-1.0.0/honeycomb


In order to allow access to REST API from an external machine, modify the "restconf-binding-address" parameter in sample-distribution/target/sample-distribution-1.0.0-hc/sample-distribution-1.0.0/config/honeycomb.json
