
PROJECT DESCRIPTION

---
This project contains the source code and scripts for the generation of syslog events. The folder contains:

 - README.md: this file.

 - LICENSE.TXT: the license text.

 - ves_syslog_reporter.c and other .c files: source code that uses the ECOMP Vendor Event Listener Library (VES) to generate the syslog events. Syslog events are generated based on /var/log/syslog entries. If a specific pattern is observed in the syslog file, then it generates the syslog event. The application reads syslog_config.json file for parameter values and poppulate the syslog event. If eventName, eventSourceType, syslogTag and tmp_syslogFile parameter value is not given, the application terminates. If reportingEntityName and/or sourceName parameter values are not given, then it gets the hostname and poppulates it. 

 - Makefile: makefile that compiles ves_syslog_reporter.c and generates ves_syslog_reporter binary.

 - go-client.sh/go-client_2_collectors.sh: bash script that starts up the ves_syslog_reporter. It reads input parameters like DCAE IP address and port from configuration files contained in /opt/config. Based on the collector configuration, use go-client.sh for single collector configuration, or use go-client_2_collectors.sh for 2 collectors configuration.


USAGE
-----

Update the configuration files with proper parameters values so that events generated would contain those values

To run the ves_syslog_reporter in single collector configuration, please execute the following steps:

 - Make the go-client.sh script executable
        chmod +x go-client.sh

 - Run the go-client.sh script
        ./go-client.sh  

For 2 collectors configuration, please execute following steps:

 - Make the go-client.sh script executable
        chmod +x go-client_2_collectors.sh

 - Run the go-client_2_collectors.sh script
        ./go-client_2_collectors.sh
