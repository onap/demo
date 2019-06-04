This python script is used to execute Traffic Distribution workflow on vFWDT service instance.
It uses AAI, OOF and APPC API to perform steps necessary to distribute traffic from one to the other 
instance of vFW. Full description of the the use case can be found in here

[ONAP vFW Traffic Distribution Use Case](https://docs.onap.org/en/latest/submodules/integration.git/docs/docs_vFWDT.html)

Usage:

`pip3 install -r requirements.txt --user`

`python3 workflow.py 55f59a16-4c10-45e1-9873-6322a25d040a 10.12.5.63 True False False True`

Input parameters:
* vnf-id of vFW VNF instance that traffic should be migrated out from
* IP of ONAP OOM Node (any)
* if script should use and build OOF response cache
* if instead of vFWDT service instance vFW or vFWCL one is used
* if only configuration information will be collected
* if APPC LCM action status should be verified and FAILURE should stop workflow

