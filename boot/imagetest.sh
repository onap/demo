#!/bin/bash

HTTP_PROXY=$(cat /opt/config/http_proxy.txt)
HTTPS_PROXY=$(cat /opt/config/https_proxy.txt)

image=$(cat /etc/issue | cut -d' ' -f2 | cut -c1-5)
echo $image
if [ ${image} == '16.04' ]
then
    echo "[Service]" > /etc/systemd/system/docker.service.d/http-proxy.conf
    echo "Environment=\"http_proxy=http://\"$HTTP_PROXY\"" >> /etc/systemd/system/docker.service.d/http-proxy.conf
    echo "Environment=\"https_proxy=https://\"$HTTPS_PROXY\"" >>/etc/systemd/system/docker.service.d/http-proxy.conf
    echo "Environment=\"HTTP_PROXY=HTTP://\"$HTTP_PROXY\"" >>/etc/systemd/system/docker.service.d/http-proxy.conf
    echo "Environment=\"HTTPS_PROXY=HTTPS://\"$HTTPS_PROXY\"" >>/etc/systemd/system/docker.service.d/http-proxy.conf
elif [ ${image} == '14.04' ]
then
    echo " export http_proxy=$HTTP_PROXY " > /etc/default/docker
    echo " export https_proxy=$HTTPS_PROXY " >> /etc/default/docker
else echo " It's not a 16 nor a 14 Ubuntu image"
fi
