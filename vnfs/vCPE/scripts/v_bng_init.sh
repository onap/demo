#!/bin/bash

systemctl start vpp

ip route add 10.3.0.0/24 via 192.168.40.41 dev tap0

