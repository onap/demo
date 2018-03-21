#!/bin/bash

PID=$(service bind9 status | grep active)
if [[ -z $PID ]]; then
  echo "unhealthy" > status.txt
else
  echo "healthy" > status.txt
fi
