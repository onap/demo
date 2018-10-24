# Running TestVNF in Docker

## Create the docker image for the netconfserver.
Start docker. 
Go to NetconfServerDocker directory. Place the jar created by building the netconfserver maven project into this directory.
The directory also contains the DockerFile to create a docker image.
Run the command:

	$ docker build -t testnetconfserver .
	
A docker image testnetconfserver is now created.

## Running docker containers
First run a mariadb container, which then can be linked to netconfserver container.

	$ docker run -it --name=testmariadbserver -v /mariadb/:/var/lib/mysql -e MYSQL_USER=root -e MYSQL_DATABASE=netconf_db -e MYSQL_PASSWORD=root -e MYSQL_ROOT_PASSWORD=root -p 3306:3306 -d jbergstroem/mariadb-alpine
	
Next, run the container for netconfserver. Also link the mariadb container to it.

	$ docker run -e NETCONFTEMPLATESDIRECTORY="/var/netconf/netconftemplates" -e TIMEDELAYFORSENDINGEVENTS="__timeDelayForSendingEvents__" -p 4002:2052  -it --link testmariadbserver:mariadb --name testnetconfserver -v "__host_location__":"/var/netconf/netconftemplates" testnetconfserver
	
	__timeDelayForSendingEvents__ needs to be replaced with an actual value required in milliseconds (for example: 15000)
	__host_location__ needs to be replaced with the actual location in the host machine which contains the netconftemplates (for example: /home/ubuntu/netconftemplates).

The container testmariadbserver is linked to container testnetconfserver.
So as per the above command, in the groovy files used to execute actions by the netconfserver, use 'mariadb' as the host name to connect to the mariadb database.

