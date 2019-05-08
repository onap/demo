Content
---

The service_mapping direction contains a directory per VNF that adds service_mapping data for robot mondel onboarding and intantiation.
Each subdirectory has a name that must match the SERVICE Name used by robot.
In each subdirectory is one file service_mapping.json that is merged with the data in robot's testsuite/robot/assets/service_mapping.py
This way the data about basic vFW, vLB etc from previous releases can be merged with new Services/Use Cases without having to edit service_mapping.py in testsuite.
As new VNF Heat and other VIM artifacts are added the matching service_mapping.json can be added in the Demo repository.

The service mapping to heat template directory, deployment artifacts, VF to VNF mapping etc are in the below structure sa required by testsuite


GLOBAL_SERVICE_FOLDER_MAPPING - This metadata identifies the folders to be zipped and uploaded to SDC for model distribution for a given VNF
Example:
````json
"GLOBAL_SERVICE_FOLDER_MAPPING": { 
	"vCPE" : ["vCPE/infra", "vCPE/vbng", "vCPE/vbrgemu", "vCPE/vgmux", "vCPE/vgw"]
}
```

GLOBAL_SERVICE_VNF_MAPPING - Map the service to the list of VNFs to be orchestrated
Example:
````json
"GLOBAL_SERVICE_VNF_MAPPING": { 
	"vLB"  : ["vLB"]
}
```

GLOBAL_SERVICE_GEN_NEUTRON_NETWORK_MAPPING - Map the service to the list of Generic Neutron Networks to be orchestrated
Example:
````json
"GLOBAL_SERVICE_GEN_NEUTRON_NETWORK_MAPPING": {
        "vCPEvGMUX" : ["MUX_GW"]
   }
```

GLOBAL_SERVICE_DEPLOYMENT_ARTIFACT_MAPPING - Map the service to the list of Deployment Artifacts for Closed Loop Control
Example:
````json
"GLOBAL_SERVICE_DEPLOYMENT_ARTIFACT_MAPPING": {
        "vLB" :["tca_docker_k8s_v4.yaml"]
   }
```

GLOBAL_SERVICE_TEMPLATE_MAPPING - This metadata identifes the preloads that need to be done for a VNF as there may be more than one (vLB)
"template" maps to the parameters in the preload_paramenters.py - GLOBAL_PRELOAD_PARAMETERS[testcase][template] - i.e. GLOBAL_PRELOAD_PARAMETERS['Demo'][dnsscaling_preload.template']
Example:
````json
"GLOBAL_SERVICE_TEMPLATE_MAPPING": {
        "vCPE" : [{"isBase" : "true",  "template" : "vcpe_preload.template", "vnf_index": "0", "name_pattern": "base_clearwater"}]
   }
```

GLOBAL_VALIDATE_NAME_MAPPING - Used by the Heatbridge Validate Query to A&AI to locate the vserver name
Example:
````json
"GLOBAL_VALIDATE_NAME_MAPPING": {
        "vCPE" : "vgw_name_0"
   }
```