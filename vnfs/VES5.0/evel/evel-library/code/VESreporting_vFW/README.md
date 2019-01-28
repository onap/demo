
PROJECT DESCRIPTION

---
This project contains the source code and scripts for the periodic generation of measurement events. The folder contains:

 - README.md: this file.

 - LICENSE.TXT: the license text.

 - vpp_measurement_reporter.c and other .c files: source code that uses the ECOMP Vendor Event Listener Library (VES) to generate the measurement events. Measurement event is generated periodically on each of the interfaces. It gives the number of bytes/packets that transmitted/received. The application reads meas_config.json file for parameter values and poppulate the measurement event. If eventName parameter value is not given, the application terminates. If reportingEntityName and sourceName parameter values are not given, then it gets the hostname and poppulates it. If measurementInterval is not given, it defaults to 60 seconds. 

 - Makefile: makefile that compiles vpp_measurement_reporter.c and generates vpp_measurement_reporter binary.

 - go-client.sh/go-client_2_collectors.sh: bash script that starts up the vpp_measurement_reporter. It reads input parameters like DCAE IP address and port from configuration files contained in /opt/config. Based on the collector configuration, use go-client.sh for single collector configuration, or use go-client_2_collectors.sh for 2 collectors configuration.


USAGE
-----

Update the configuration files with proper parameters values so that events generated would contain those values

To run the vpp_measurement_reporter in single collector configuration, please execute the following steps:

 - Make the go-client.sh script executable
        chmod +x go-client.sh

 - Run the go-client.sh script
        ./go-client.sh  

For 2 collectors configuration, please execute following steps:

 - Make the go-client.sh script executable
        chmod +x go-client_2_collectors.sh

 - Run the go-client_2_collectors.sh script
        ./go-client_2_collectors.sh
