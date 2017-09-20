#!/bin/bash

bash /opt/msb_vm_init.sh
sleep 2
bash /opt/vnfsdk_vm_init.sh
sleep 2
bash /opt/mvim_vm_init.sh
sleep 2
bash /opt/vfc_vm_init.sh
sleep 2
bash /opt/uui_vm_init.sh