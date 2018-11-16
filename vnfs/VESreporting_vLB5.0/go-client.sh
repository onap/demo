#!/bin/bash

export LD_LIBRARY_PATH="/opt/VES/evel/evel-library/libs/x86_64/"
DCAE_COLLECTOR_IP=$(cat /opt/config/dcae_collector_ip.txt)
DCAE_COLLECTOR_PORT=$(cat /opt/config/dcae_collector_port.txt)
#./vpp_measurement_reporter $DCAE_COLLECTOR_IP $DCAE_COLLECTOR_PORT eth1
#./vpp_measurement_reporter 127.0.0.1 30000 127.0.0.1 31000 eth1
./vpp_measurement_reporter 127.0.0.1 30000 eth1
