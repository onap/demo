
PROJECT DESCRIPTION
---
This project is a port of the VES event library and reporting client to run on DANOS,  an open source virtual router. DANOS has an embedded stateless firewall that can be used to demonstrate ONAP closed loop control as an alternative to the rudimentary demonstration VNF we have with the original ONAP vFW. As a real VNF , DANOS has features that we can use for better demonstrations of ONAP capability over time so making it easier to do ONAP demosntrations with a real VNF will be beneficial.

The project is meant to be used to build a customer ISO of DANOS with ves/libevel integrated into the build and then that ISO can be used to generate a glance image for ONAP testing.

The project will be both the ONAP source code for the VES event reporting and the instructions on how to crate the DANOS image that could be used in a glance repository.

Heat templates , environment files , preload data and service mapping will be in the normal sections of the repository for those things (demo/vnfs)

Generaly after downloading this directoy you will use the DANOS package build tools to run make which will create the debian file for inclusing in a DANO ISO build.

See  DANOS_BUILD.md  for details.

This project contains the source code and scripts for the periodic generation of network measurement reports for the DANOS virtual router with embeded firewall. It calls the DANOS API's to pull statistics for the ONAP vFW closed loop use case and could be extended for more statistics as needed. The project also includes a systemd start script to be compatible with the DANOS run time environment. The important part of the folder are:

 - README.md: this file.

 - LICENSE.TXT: the license text.

 - vpp_measurement_reporter_danos.c: source code that uses the ECOMP Vendor Event Listener Library (VES) to read metrics from the network interface and send periodic measurement reports to the VES collector in DCAE. The VES library used here has been cloned from the GitHub repository at https://github.com/att/evel-library on February 1, 2017.

 - evel/*  source code for the Event Library that needs to be built with the vpp_measurement_reporter_danos as a shared library and included in the debian install

 - debian/* debian package mandaatory files reqiured to specify the control and rules needed to build the vpp_measurement_report_danos debian file.

 - Makefile: makefile that compiles libevel, vpp_measurement_reporter_danos.c and generates vpp_measurement_reporter binary.


USAGE
---

vpp_measurement_report_danos can be started via systemctl once the DANOS Virtual Machine is instantiated.


CONFIGURATION
---
Consult the onap wiki for instructions on how to use netconf to configuration the DANOS router for the vFWCL use case.

[fill in url to page on wiki.onap.org ]

