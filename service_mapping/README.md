Content
---

The service_mapping direction contains a directory per VNF that adds service_mapping data for robot mondel onboarding and intantiation.
Each subdirectory has a name that must match the SERVICE Name used by robot.
In each subdirectory is one file service_mapping.json that is merged with the data in robot's testsuite/robot/assets/service_mapping.py
This way the data about basic vFW, vLB etc from previous releases can be merged with new Services/Use Cases without having to edit service_mapping.py in testsuite.
As new VNF Heat and other VIM artifacts are added the matching service_mapping.json can be added in the Demo repository.

The service mapping to heat template directory, deployment artifacts, VF to VNF mapping etc are the same structures as in the testsuite service_mapping.py.

