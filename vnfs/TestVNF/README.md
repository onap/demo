# TestVNF - NetconfServer

TestVNF - NetconfServer is a partial implementation of a netconf server for netconf termination. It uses [Apache MINA SSHD](https://github.com/apache/mina-sshd) and extends a netconf subsystem.
The system doesn't provide persistence out of the box even though it could be done through groovy files, which remain external to the system and takes actions based on netconf operations.

## Requirements
* Java 8
* Maven
* MariaDB

## Usage

### To run the maven project
Go to netconfserver and execute the following commands in a terminal.

	$ mvn package

The jar file is created in the target folder
	
	$ java -cp target/netconfsystem-jar-with-dependencies.jar com.ericsson.testvnf.server.Server <NETCONFTEMPLATESDIRECTORY> <TIMEDELAYFORSENDINGEVENTS>

Run the jar file. Also, specify the arguments.
	
NETCONFTEMPLATESDIRECTORY is the directory which contains the netconf templates, YANG models and the groovy files to be executed based on netconf RPC request.
TIMEDELAYFORSENDINGEVENTS is the time delay in milliseconds set between sending each of the events by the VNF.

The values for host and port are taken from <NETCONFTEMPLATESDIRECTORY>/netconftemplates/server-config.properties. If the config file is missing, then the server is started on the default value 0.0.0.0:2052

* __sample rpc requests__ directory contains some sample rpc requests to test the TestVNF.

* __netconftemplates__ directory contains sample netconftemplates and groovy files for performing actions based on netconf requests.

### To run in docker
Go to docker directory for details on how to run the netconf server in docker.

## Description
TestVNF - NetconfServer is a partial implementation of a netconf server for netconf termination. It uses Apache MINA SSHD and extends a netconf subsystem..
Action taken by the netconf server is kept independent of the server implementation. Groovy files used to perform actions based on netconf RPC operations are kept external to the project in a directory or in an FTP server. The location is passed while running the project. This external directory will also contain the sample templates and yang models to be used by the server.

## Example
A client is connecting to the netconf server.
The server sends back a hello message from the location <NETCONFTEMPLATESDIRECTORY>/netconftemplates/hello.xml

The client then sends an edit-config netconf request on the candidate data store.
The server parses the RPC request from the client and gets the operation and configuration datastore. Based on these the correct groovy file is selected and the data is sent to that groovy file. 

<NETCONFTEMPLATESDIRECTORY>/netconftemplates/edit-config/candidate/response.groovy is the groovy file selected in this case.

If the operation doesn't involve a configuration data store, for example, for close-session

<NETCONFTEMPLATESDIRECTORY>/netconftemplates/close-session/NA/response.groovy is executed by the system.

### Custom operations
The netconf system supports custom operations without the need for recompiling the project.
Consider a custom operation send-models which send events to a specified target one by one with a time delay.

A client can get the schema of this operation by sending a get-schema request to the server.
The server picks up the correct yang models and sends back to the client. For this, the model has to be present in the netconftemplates directory as <operation_name>-schema.yang.

<NETCONFTEMPLATESDIRECTORY>/netconftemplates/send-models-schema.yang is selected in the aforementioned case.

When the client sends a send-models request to the server, the action is performed by executing the below groovy file.

<NETCONFTEMPLATESDIRECTORY>/netconftemplates/send-models/NA/response.groovy

When an edit-config request with the same target name as specified in send-models request is sent to the netconf system, the sending of events is terminated.
