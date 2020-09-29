5GC demo artifacts directory structure:

   Helm    #Directory containing helm charts that need to be packaged and attached to csar package for 5GC simulator UC.
   CBA     #Directory containing CBA content to be included to csar package

5G Core Slice creation and Core NS instantion using the Macro flow through NBI Automation.
Here we are performing the Instantiation,Day0,Day1 and Day2 configuration with help of SO,SDNC,Multicloud (k8s-plugin) and CDS.
There are two E2E workflows involved i.e Macro POST (Instantiation) and PUT (Modifiy Config) operations

POST: Instantiation,Day0 and Day1 (will update the snssai value in CNF while creation/Instantiation) will perform.
PUT: Day2 can perform (will update the snssai value for Modify Config flow).

Process:

In SDC vf Design --> create a new param as "snssai" and declare it as input in Properties Assignment section, Even in service design refer that same vf and decalre it as input.
Pls update the CBA Resource resolution param "fiveg0_snssai" to "yourvfname_snssai" (like example0_snssai)... you can see that param in the service properties Assignment

Update the SO catalogdb table as below for config-assign and config-deploy process from SO to CDS...

>select * from orchestration_flow_reference;

| 429 | Service-Macro-Create |      1 | AssignServiceInstanceBB                |  1 | 102 | NULL  | NULL                |
| 432 | Service-Macro-Create |      2 | CreateNetworkCollectionBB              |  1 | 102 | NULL  | NULL                |
| 435 | Service-Macro-Create |      3 | AssignNetworkBB                        |  1 | 102 | NULL  | NULL                |
| 438 | Service-Macro-Create |      4 | AssignVnfBB                            |  1 | 102 | NULL  | NULL                |
| 441 | Service-Macro-Create |      5 | AssignVolumeGroupBB                    |  1 | 102 | NULL  | NULL                |
| 444 | Service-Macro-Create |      6 | AssignVfModuleBB                       |  1 | 102 | NULL  | NULL                |
| 447 | Service-Macro-Create |      7 | ControllerExecutionBB                  |  1 | 102 | vnf   | config-assign       |
| 450 | Service-Macro-Create |      8 | AssignPnfBB                            |  1 | 102 | NULL  | NULL                |
| 453 | Service-Macro-Create |      9 | WaitForPnfReadyBB                      |  1 | 102 | NULL  | NULL                |
| 456 | Service-Macro-Create |     10 | ActivatePnfBB                          |  1 | 102 | NULL  | NULL                |
| 459 | Service-Macro-Create |     11 | CreateNetworkBB                        |  1 | 102 | NULL  | NULL                |
| 462 | Service-Macro-Create |     12 | ActivateNetworkBB                      |  1 | 102 | NULL  | NULL                |
| 465 | Service-Macro-Create |     15 | CreateVolumeGroupBB                    |  1 | 102 | NULL  | NULL                |
| 468 | Service-Macro-Create |     16 | ActivateVolumeGroupBB                  |  1 | 102 | NULL  | NULL                |
| 471 | Service-Macro-Create |     17 | CreateVfModuleBB                       |  1 | 102 | NULL  | NULL                |
| 474 | Service-Macro-Create |     18 | ActivateVfModuleBB                     |  1 | 102 | NULL  | NULL                |
| 477 | Service-Macro-Create |     19 | ControllerExecutionBB                  |  1 | 102 | vnf   | config-deploy       |
| 480 | Service-Macro-Create |     20 | ActivateVnfBB                          |  1 | 102 | NULL  | NULL                |
| 483 | Service-Macro-Create |     21 | ActivateNetworkCollectionBB            |  1 | 102 | NULL  | NULL                |
| 486 | Service-Macro-Create |     22 | ActivateServiceInstanceBB              |  1 | 102 | NULL  | NULL                |
