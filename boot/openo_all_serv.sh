#!/bin/bash
HTTP_PROXY=$(cat /opt/config/http_proxy.txt)
HTTPS_PROXY=$(cat /opt/config/https_proxy.txt)

if [ $HTTP_PROXY != "no_proxy" ]
then
    export http_proxy=$HTTP_PROXY
    export https_proxy=$HTTPS_PROXY
fi

bash /opt/msb_vm_init.sh &>/dev/null &disown
bash /opt/vnfsdk_vm_init.sh &>/dev/null &disown
bash /opt/mvim_vm_init.sh &>/dev/null &disown
bash /opt/vfc_vm_init.sh &>/dev/null &disown
bash /opt/uui_vm_init.sh &>/dev/null &disown
bash /opt/esr_vm_init.sh &>/dev/null &disown