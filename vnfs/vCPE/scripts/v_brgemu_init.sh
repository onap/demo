#!/bin/bash

systemctl start vpp
systemctl start honeycomb

/opt/set_nat.sh
