#!/bin/bash

vppctl packet-gen disable
vppctl packet-gen enable-stream dns1
vppctl packet-gen enable-stream dns2
sleep 300
vppctl packet-gen enable-stream dns3
vppctl packet-gen enable-stream dns4
vppctl packet-gen enable-stream dns5
