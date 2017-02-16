#!/bin/bash

cd /opt/FDclient
./dnsclient.sh &>/dev/null &disown
cd ~
