#!/bin/bash

systemctl start vpp
systemctl start honeycomb

/opt/bind_nic.sh
/opt/set_nat.sh
