#!/bin/bash

if [ -z $1 ]; then
  echo "Missing number of streams to enable. Exit (1)"
  exit
fi

if [ "$1" -gt "10" ] || [ "$1" -lt "1" ]; then
  echo "The number of streams to enable must be between 1 and 10. Exit(1)"
  exit
fi

STREAMS=$1

# Disable all the streams
vppctl packet-generator disable-stream

# Enable streams based on client input
for((i=1; i <= $STREAMS; i++)); do
  vppctl packet-generator enable-stream fw_udp$i
done