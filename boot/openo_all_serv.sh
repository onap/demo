#!/bin/bash

bash /opt/msb_vm_init.sh &>/dev/null &disown
bash /opt/vnfsdk_vm_init.sh &>/dev/null &disown
bash /opt/mvim_vm_init.sh &>/dev/null &disown
bash /opt/vfc_vm_init.sh &>/dev/null &disown
bash /opt/uui_vm_init.sh &>/dev/null &disown
bash /opt/esr_vm_init.sh &>/dev/null &disown