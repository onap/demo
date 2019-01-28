
PROJECT DESCRIPTION

---
This project contains the source code and scripts for the periodic generation of heartbeat events. The folder contains:

 - README.md: this file.

 - LICENSE.TXT: the license text.

 - ves_heartbeat_reporter.c and other .c files: source code that uses the ECOMP Vendor Event Listener Library (VES) to generate the periodic hertbeat events. It reads hb_config.json file for parameter values and poppulate the heartbeat event. If eventName parameter value is not given, the application terminates. If reportingEntityName and sourceName parameter values are not given, then it gets the hostname and poppulates it. If heartbeatInterval is not given, it defaults to 60 seconds. 

 - Makefile: makefile that compiles ves_heartbeat_reporter.c and generates ves_heartbeat_reporter binary.

 - go-client.sh/go-client_2_collectors.sh: bash script that starts up the ves_heartbeat_reporter. It reads input parameters like DCAE IP address and port from configuration files contained in /opt/config. Based on the collector configuration, use go-client.sh for single collector configuration, or use go-client_2_collectors.sh for 2 collectors configuration.


USAGE
-----

Update the configuration files with proper parameters values so that events generated would contain those values

To run the ves_heartbeat_reporter in single collector configuration, please execute the following steps:

 - Make the go-client.sh script executable
        chmod +x go-client.sh

 - Run the go-client.sh script
        ./go-client.sh  

For 2 collectors configuration, please execute following steps:

 - Make the go-client.sh script executable
        chmod +x go-client_2_collectors.sh

 - Run the go-client_2_collectors.sh script
        ./go-client_2_collectors.sh
