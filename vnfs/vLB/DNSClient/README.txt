README
---

Run the dnsclient.sh script to start the FDClient Java binary. The DNSMembershipManager should be activated first. FDClient will periodically send keep-alive messages to the FDServer.
- java FDClient [process id] [server IP address] [port] [timeout (sec)] [debug]
  - [process id]: ID of this process
  - [server IP address]: IP address of the remote server
  - [port]: port where the server listens for incoming packets
  - [timeout (sec)]: how often send a packet to the server
  - [debug]: verbose mode (TRUE: >=1, FALSE: <=0)
- Example: java FDClient p1 192.168.9.100 8888 10 1

Please change the default parameters in the dnsclient.sh script as appropriate for your configuration.
  
