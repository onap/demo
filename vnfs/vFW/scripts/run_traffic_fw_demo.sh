#!/bin/bash

while [ 1 ] 
do
curl -X PUT -H "Authorization: Basic YWRtaW46YWRtaW4=" -H "Content-Type: application/json" -H "Cache-Control: no-cache" -H "Postman-Token: 9005870c-900b-2e2e-0902-ef2009bb0ff7" -d '{"streams": {"active-streams": 10}}' http://localhost:8183/restconf/config/stream-count:stream-count/streams
sleep 300
curl -X PUT -H "Authorization: Basic YWRtaW46YWRtaW4=" -H "Content-Type: application/json" -H "Cache-Control: no-cache" -H "Postman-Token: 9005870c-900b-2e2e-0902-ef2009bb0ff7" -d '{"streams": {"active-streams": 1}}' http://localhost:8183/restconf/config/stream-count:stream-count/streams
sleep 300
done