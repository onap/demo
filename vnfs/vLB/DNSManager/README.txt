README
---

Run the dnsmembership.sh script before activating DNS clients. dnsmembership.sh starts the DNSMembershipManager Java class, which uses the FDServer Java class to keep track of the remote DNS servers that are currently active. Example:

./dnsmembership.sh

The script will call the DNSMembershipManager binary with default parameters:
- java DNSMembershipManager [port] [timeout (sec)] [threshold] [debug]
  - [port]: port where the server listens for incoming packets
  - [timeout (sec)]: how often update the status of remote processes (clients)
  - [threshold]: how many missed packet are required to suspect a remote process to be dead
  - [debug]: verbose mode (TRUE: >=1, FALSE: <=0) 
- Example: java DNSMembershipManager 8888 10 3 1

Please change the default parameters in the dnsmembership.sh script as appropriate for your configuration.
  
