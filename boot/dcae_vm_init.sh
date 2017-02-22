#!/bin/bash

cd /opt/dcae-startup-vm-controller
git pull
bash init.sh
make up
