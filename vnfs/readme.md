# ECOMP Vendor Event Listener Library


This project contains a C library that supports interfacing to AT&T's ECOMP
Vendor Event Listener. For an overview of ECOMP, see the 
[ECOMP White Paper](http://att.com/ECOMP).

Developed in 2016 for AT&T by:
 * Alok Gupta (https://github.com/ag1367)
 * Paul Potochniak (https://github.com/pp8491)
 * Gayathri Patrachari(https://github.com/gp2421)

Current Maintainers: 
 * Alok Gupta (https://github.com/ag1367)
 * Paul Potochniak (https://github.com/pp8491)
 * Gayathri Patrachari(https://github.com/gp2421)

#The description about important directories are given here

 VES5.0 - This contains the code for VES library for VES5.4.1, sample agents

 VESreporting_vFW5.0 - This is the sample virtual firewall agent

 VESreporting_vLB5.0 - This is the sample virtual load balancer agent

 VES5.0/evel/evel-library/code/evel_library - Code for VES library 'evel'

 VES5.0/evel/evel-library/code/VESreporting_HB - Sample json based heartbeat event generated periodically

 VES5.0/evel/evel-library/code/VESreporting_fault - Sample json based fault event based on low byte/packet count on interface.

 VES5.0/evel/evel-library/code/VESreporting_syslog - Sample json based syslog event based on pattern being logged into any file

 VES5.0/evel/evel-library/code/VESreporting_vFW - Sample json based Firewall application that generates measurement event periodically. 

# Info on evel Library
This library supports following events. The corresponding factory functions to generate that event is also mentioned against it
- Faults  - ::evel_new_fault
- Heartbeat - ::evel_new_heartbeat
- Measurements - ::evel_new_measurement
- State Change - ::evel_new_state_change
- Syslog - ::evel_new_syslog
- Other - ::evel_new_other
- Mobile Flow - ::evel_new_mobile_flow
- Sipsingnaling - ::evel_new_signaling
- Threshold Crossing Alert - ::evel_new_threshold_cross
- Voice Quality - ::evel_new_voice_quality

# Setting the environment
Install gcc, libcurl3-dev packages as below

  sudo apt-get install gcc
  sudo apt-get install libcurl3-dev

Clone the code from demo repository

#Compile VES library
 Go to /demo/vnfs/VES5.0/evel/evel-library/bldjobs directory and run the below commands
   make all ==> to create the VES library
   make package ==> to create the VES package if needed

   FYI - 'make install' can also be given to make a package and install the package in another machine - see /demo/vnfs/VES5.0/evel/evel-library/bldjobs/Makefile for more details 

 After compilation VES libraries are generated and are available at below location
   /demo/vnfs/VES5.0/evel/evel-library/libs/x86_64

#Loading VES the library
 Go to /demo/vnfs/VES5.0/evel/evel-library/libs/x86_64 directory and run below commands
   sudo cp libevel.so /usr/lib
   sudo ldconfig

#Compiling agent code
 After successful compiling of VES library and loading the library, go to agent directory and run 'make all'
For json heartbeat agent
 > cd /demo/vnfs/VES5.0/evel/evel-library/VESreporting_HB
 > make all

For all other jason based agents (VESreporting_fault, VESreporting_syslog and VESreporting_vFW) also, the compilation to be carried out in the same manner as mentioned above. 

But for agents in VESreporting_vFW5.0 and VESreporting_vLB5.0, special care to be taken as below
 - Copy the contents of the directory into ~/demo/vnfs/VES5.0/evel/evel-library/VESreporting directory
 - run 'make all' to create an executable (alternatively, this agent gets compiled during compilation of evel library when VESreporting directory has the agent code.

#Run the agent
 - run the application using go-client.sh command in the agent directory as below
    > sudo chmod +x go-client.sh
    > sudo ./go-client.sh

# Agent building guide

Each application that wants to send events would call evel_initialize() function to initialize the parameter with evel library. For more details about the parameters passed, see the VES5.0/evel/evel-library/code/evel_library/evel.h file. The public APIs to the library are defined in evel.h

EVEL_ERR_CODES evel_initialize(const char * const fqdn,
                               int port,
                               const char * const bakup_fqdn,
                               int bakup_port,
                               const char * const path,
                               const char * const topic,
                               int ring_buf_size,
                               int secure,
                               const char * const cert_file_path,
                               const char * const key_file_path,
                               const char * const ca_info,
                               const char * const ca_file_path,
                               long verify_peer,
                               long verify_host,
                               const char * const username,
                               const char * const password,
                               const char * const bakup_username,
                               const char * const bakup_password,
                               const char * const source_ip,
                               const char * const bakup_source_ip,
                               EVEL_SOURCE_TYPES source_type,
                               const char * const role,
                               int verbosity
                               )  {
    fprintf(stderr, "Failed to initialize the EVEL library!!!");
    exit(-1);
  }

After successful running of evel_initialize() API, call the APIs to generate the events.

For fault event generation, below APIs would be called. For other events see the APIs listed in VES5.0/evel/evel-library/code/evel_library/evel.h

  EVENT_FAULT * fault = evel_new_fault("Fault_vFW-ATT-LinkdownError",
                                       "fault0001",
                                       "My alarm condition",
                                       "It broke very badly",
                                       EVEL_PRIORITY_NORMAL,
                                       EVEL_SEVERITY_MAJOR,
                                        EVEL_SOURCE_HOST,
                             EVEL_VF_STATUS_PREP_TERMINATE);
  if (fault != NULL)
  {
    evel_fault_type_set(fault, "Bad things happen...");
    evel_fault_interface_set(fault, "My Interface Card");
    evel_fault_addl_info_add(fault, "name1", "value1");
    evel_fault_addl_info_add(fault, "name2", "value2");
    evel_rc = evel_post_event((EVENT_HEADER *)fault);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }

