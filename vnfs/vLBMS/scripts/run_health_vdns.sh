#!/bin/bash

PID=$(service bind9 status | grep inactive)
if [[ -z $PID ]]; then
	echo "healthy" > status.txt
else
	echo "unhealthy" > status.txt
fi
