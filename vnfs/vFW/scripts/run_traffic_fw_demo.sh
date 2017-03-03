#!/bin/bash

#curl -X PUT -H "Authorization: Basic YWRtaW46YWRtaW4=" -H "Content-Type: application/json" -H "Cache-Control: no-cache" -H "Postman-Token: 9005870c-900b-2e2e-0902-ef2009bb0ff7" -d '{"pg-streams":{"pg-stream": [{"id":"fw_udp1", "is-enabled":"true"},{"id":"fw_udp2", "is-enabled":"true"},{"id":"fw_udp3", "is-enabled":"true"},{"id":"fw_udp4", "is-enabled":"true"},{"id":"fw_udp5", "is-enabled":"true"}]}}' "http://localhost:8183/restconf/config/sample-plugin:sample-plugin/pg-streams"

#sleep 300

while [ 1 ] 
do
curl -X PUT -H "Authorization: Basic YWRtaW46YWRtaW4=" -H "Content-Type: application/json" -H "Cache-Control: no-cache" -H "Postman-Token: 9005870c-900b-2e2e-0902-ef2009bb0ff7" -d '{"pg-streams":{"pg-stream": [{"id":"fw_udp1", "is-enabled":"true"},{"id":"fw_udp2", "is-enabled":"true"},{"id":"fw_udp3", "is-enabled":"true"},{"id":"fw_udp4", "is-enabled":"true"},{"id":"fw_udp5", "is-enabled":"true"},{"id":"fw_udp6", "is-enabled":"true"},{"id":"fw_udp7", "is-enabled":"true"},{"id":"fw_udp8", "is-enabled":"true"},{"id":"fw_udp9", "is-enabled":"true"},{"id":"fw_udp10", "is-enabled":"true"}]}}' "http://localhost:8183/restconf/config/sample-plugin:sample-plugin/pg-streams"
sleep 300
curl -X PUT -H "Authorization: Basic YWRtaW46YWRtaW4=" -H "Content-Type: application/json" -H "Cache-Control: no-cache" -H "Postman-Token: 9005870c-900b-2e2e-0902-ef2009bb0ff7" -d '{"pg-streams":{"pg-stream": [{"id":"fw_udp1", "is-enabled":"true"}]}}' "http://localhost:8183/restconf/config/sample-plugin:sample-plugin/pg-streams"
sleep 300
done
