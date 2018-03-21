#!/bin/bash

SERVICE=$(cat /opt/config/service.txt)

PID=$(pgrep $SERVICE)
if [[ -z $PID ]]; then
  echo "unhealthy" > status.txt
else
  echo "healthy" > status.txt
fi
