#!/bin/bash
export LD_LIBRARY_PATH="/opt/VES/libs/x86_64/"
DCAE_COLLECTOR_IP=$(cat /opt/config/dcae_collector_ip.txt)
DCAE_COLLECTOR_PORT=$(cat /opt/config/dcae_collector_port.txt)
DCAE_COLLECTOR_IP2=$(cat /opt/config/dcae_collector_ip2.txt)
DCAE_COLLECTOR_PORT2=$(cat /opt/config/dcae_collector_port2.txt)

#Usage
#./afx_ves_reporter <DCAE FQDN>|<IP address> <port> <credential file> <debug level>
# OR
#./afx_ves_reporter <DCAE FQDN>|<IP address> <port> <credential file> <debug level> <DCAE FQDN2>|<IP address2> <port2> <credential file2>

./afx_ves_reporter $DCAE_COLLECTOR_IP $DCAE_COLLECTOR_PORT ./vescred 0 0
