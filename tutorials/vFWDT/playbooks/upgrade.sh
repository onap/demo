#!/bin/bash

# ============LICENSE_START=======================================================
# Copyright (C) 2019 Orange
# ================================================================================
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ============LICENSE_END=========================================================

#change IP addresses and upload to playbooks folder darkstat and server.py before

SINK1=10.254.184.217
SINK2=10.254.184.210
VFW1=10.254.184.208
VFW2=10.254.184.216

echo $VFW1 > vfw_mgt_ip.txt
scp -oStrictHostKeyChecking=no -i onap.pem vfw_mgt_ip.txt root@$SINK1:/opt/config/
ssh -oStrictHostKeyChecking=no -i onap.pem root@$SINK1 /etc/init.d/darkstat stop
scp -oStrictHostKeyChecking=no -i onap.pem darkstat root@$SINK1:/usr/sbin/
ssh -oStrictHostKeyChecking=no -i onap.pem root@$SINK1 /etc/init.d/darkstat start

ssh -oStrictHostKeyChecking=no -i onap.pem root@$VFW1 "hostname > /opt/config/hostname.txt"
ssh -oStrictHostKeyChecking=no -i onap.pem root@$VFW1 "echo '1.0' > /opt/config/version.txt"
scp -oStrictHostKeyChecking=no -i onap.pem server.py root@$VFW1:/opt/config/
ssh -oStrictHostKeyChecking=no -i onap.pem root@$VFW1 "screen -d -m bash -c 'cd /opt/config && python3 /opt/config/server.py 80 > /dev/null 2>&1'"

echo $VFW2 > vfw_mgt_ip.txt
scp -oStrictHostKeyChecking=no -i onap.pem vfw_mgt_ip.txt root@$SINK2:/opt/config/
ssh -oStrictHostKeyChecking=no -i onap.pem root@$SINK2 /etc/init.d/darkstat stop
scp -oStrictHostKeyChecking=no -i onap.pem darkstat root@$SINK2:/usr/sbin/
ssh -oStrictHostKeyChecking=no -i onap.pem root@$SINK2 /etc/init.d/darkstat start

ssh -oStrictHostKeyChecking=no -i onap.pem root@$VFW2 "hostname > /opt/config/hostname.txt"
ssh -oStrictHostKeyChecking=no -i onap.pem root@$VFW2 "echo '1.0' > /opt/config/version.txt"
scp -oStrictHostKeyChecking=no -i onap.pem server.py root@$VFW2:/opt/config/
ssh -oStrictHostKeyChecking=no -i onap.pem root@$VFW2 "screen -d -m bash -c 'cd /opt/config && python3 /opt/config/server.py 80 > /dev/null 2>&1'"

