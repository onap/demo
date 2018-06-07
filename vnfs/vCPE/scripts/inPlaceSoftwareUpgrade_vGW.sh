root@onap-so:~# cat /root/inPlaceSoftwareUpgrade_vGW.sh
#!/bin/bash

tmp=/tmp/send$$
trap "rm -f $tmp.*; exit 1" 0 1 2 15

user="InfraPortalClient:password1$"
serviceInstance="a9a77d5a-123e-4ca2-9eb9-0b015d2ee0fb"
vnf="1ae8197f-8496-41e1-9695-dce9fd77315a"
apiVer=v6

url="http://localhost:8080/ecomp/mso/infra/serviceInstances/$apiVer/$serviceInstance/vnfs/$vnf/inPlaceSoftwareUpdate"

(
cat << 'XEOF'
{
  "requestDetails": {
    "requestInfo": {
      "source": "VID",
      "suppressRollback": false,
      "requestorId": "ek1439"
    },
    "cloudConfiguration": {
      "tenantId": "d570c718cbc545029f40e50b75eb13df",
      "lcpCloudRegionId": "RegionOne"
    },
    "requestParameters": {
      "payload": "{\"existing-software-version\": \"3.1\",\"new-software-version\": \"3.2\", \"operations-timeout\": \"3600\"}",
      "controllerType": "sdnc"
    }
  },
  "serviceInstanceId": "$serviceInstance",
  "vnfInstanceId": "$vnf"
}

XEOF
) > $tmp.content || exit 1

# Enabling debug logging for API-Handler-Infra . . .
curl -o /dev/null -w '%{http_code}' --user 'MSOClient:password1$' 'http://mso:8080/ecomp/mso/infra/logging/debug?enable=true'

# Enabling debug logging for BPMN . . .
curl -o /dev/null -w '%{http_code}' 'http://mso:8080/mso/logging/debug?enable=true'

# Enabling debug logging for SDNC-Adapter . . .
curl -o /dev/null -w '%{http_code}' --user 'MSOClient:password1$' 'http://mso:8080/adapters/rest/logging/debug?enable=true'

# Send the request
curl -v -X POST -d @$tmp.content --user "$user" "$url" --header "Content-Type:application/json"
