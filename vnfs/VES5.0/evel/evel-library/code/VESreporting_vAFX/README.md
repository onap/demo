PROJECT DESCRIPTION

The AFX agent is for AT&T internal use only.

The AFX VES Agent collects and reports telemetry data from vAFX platform to VES Collector.

This is a multi-threaded application consists of Event Handler thread, Heartbeat thread, link monitor thread, Service Monitor thread, Scaling Measurement thread and BGP (Border Gateway Protocol) Monitoring thread.

It uses VES Library 5.4.1 version for invoking REST API to send messages to VES Open DCAE collector. Event handler thread is part of VES-Library that invokes CURL REST API calls and sends data to VES Collector over http(s). Event handler monitors data from multiple threads and sends the data once it is available.


Afxagent does following initializations

Uses the VES library API  evel_initialize() to initialize the VES library with information such as collector IP, port number, username, password etc
Reads the afxmodules.conf file for thread listings and compares with internal list of threads
Creates the threads based on afxmodules.conf thread listings.

The contents of afxmodules.conf would be as below. If any of events not required, then you can remove it, so that only the required threads and the corresponding events are generated.

HeartBeat
LinkMonitor
ServiceMonitor
ScalingMeasurements
SyslogBgp

The explanation for each entry in afxmodules.conf is given below

HeartBeat :: This generates periodic heartbeat event

LinkMonitor :: This thread monitors the status of vNICs using ethtool interface. The list of vNICs that need to be monitored are configured in “afxintf.conf” file. When the status of vNICs change by either Ethernet cable pulled out or Ethernet interface going up and down the changed status is reported.

ServiceMonitor :: This thread monitors the state of the afx@input* and afx@output* services by running “/bin/systemctl status afx@input*” or “/bin/systemctl status afx@output*” command. If there is a change in status from “active” to “Inactive” then Major severity fault is raised. If the change in status is from “inactive” to “active”, then Normal severity fault is raised. If both services status is inactive, then critical severity fault is raised.

ScalingMeasurements :: This thread reports CPU load, memory usage and vNIC data in Scaling measurement messages periodically.

SyslogBgp :: BGP monitoring thread forwards syslog messages in platform along with changes to BGP peer connections. At startup the thread reads “afxfilter.txt” file for the list of Tags including “exabgp”. The Thread forwards only those messages containing the tags in Syslog event to Collector. The thread also monitors BGP messages for connection to peer routers. A Fault event is reported when new connections are made or when existing connections are shut down.

Compiling
=========
Compile the code as below

Please note that VES library is compiled already and loaded
go to /demo/vnfs/VES5.0/evel/evel-library/code/VESreporting_vAFX directory
run the command 'make all'

Make a zip file containing below files
/demo/vnfs/VES5.0/evel/evel-library/code/VESreporting_vAFX/afx_ves_reporter
/demo/vnfs/VES5.0/evel/evel-library/code/VESreporting_vAFX/install
/demo/vnfs/VES5.0/evel/evel-library/code/VESreporting_vAFX/vescred
/demo/vnfs/VES5.0/evel/evel-library/code/VESreporting_vAFX/run
/demo/vnfs/VES5.0/evel/evel-library/code/VESreporting_vAFX/afxmodules.conf
/demo/vnfs/VES5.0/evel/evel-library/code/VESreporting_vAFX/afxintf.conf
/demo/vnfs/VES5.0/evel/evel-library/code/VESreporting_vAFX/afxfilter.txt
libevel.so or libevel.a (or both) from /demo/vnfs/VES5.0/evel/evel-library/libs/x86_64


Running AFXAGENT
================

Install below packages in the machine where the afxagent is intend to run
 
 sudo apt-get install exabgp
 sudo apt-get install ethtool
 sudo apt-get install libcurl3-dev

[1] Getting started.
- FTP or SCP the zipped file created above and unzip into machine where afxagent is suppose to run

[2] Setting up the dynamic linked library of VES
- Take the backup of existing /usr/lib/libevel.so file using the below command.
	cp /usr/lib/libevel.so /usr/lib/libevel.so_bakup
- Go to unziped  directory where libevel.so file is there and run all these commands from there.
	sudo cp libevel.so /usr/lib 
	sudo ldconfig
[3] Updating the interface info in afxintf.conf file 
- Run the "ifconfig" command and list the interfaces in afxintf.conf one below the other. For e.g, if the "ifconfig" shows 2 interfaces "lo" and "ens2", then you should have only below contents in afxintf.conf file. If you have more interfaces, you need to list those interfaces along with other keyword incremented (e.g next one would be, <interface name> DNS3_ RSLOTPORT_VLAN_CLASSTYPE) accordingly.

lo   DNS1_RSLOTPORT_VLAN_CLASSTYPE
ens2   DNS2_RSLOTPORT_VLAN_CLASSTYPE

[4] Collector information update
- Get the collector IP address, port number, collector login / password
- Update the "vescred.conf" file with collector login and password. If the login/passwd is Sample1 / Passwd1, then "vescred.conf" file should look as below.
Sample1
Passwd1

-Update the file "run" in the below format
./afx_ves_reporter <collector IP address> <collector port number> <credential file vescred.conf> <https/http=1/0> <debug=1/0>

If collector URL is given as below https://zldcmtn23bdce2coll01.ed926b.mtn23b.tci.att.com:8443/eventListener/v5,  

then your run file would look like below

unset http_proxy
unset https_proxy 
./afx_ves_reporter zldcmtn23bdce2coll01.ed926b.mtn23b.tci.att.com 8443 ./vescred.conf 1 1

[5] Install HTTPS certificates. See the onap documentation

[6] Running of afxagent
- Once the run is file is updated, now we have to run the VES agent using below command.
	sudo ./run

[7] Stopping of afxagent
- You can do CTRL C to stop if are seeing the terminal where afxagent is running. Otherwise, get the process ID and kill it as below
	ps -aef | grep run
	Kill <run process id>

[8] Sample events
Event samples are given in sample_afxagent_events.txt file
