#/bin/bash

if [ ! "$#" -eq 1 ]
then
  echo "Usage: ./update_running_streams.sh [number of active streams]"
  exit
fi

STREAMS=$1
BODY=""

for((i=1; $i<= $STREAMS; i++)); do
  BODY+="{\"id\":\"fw_udp$i\", \"is-enabled\":\"true\"},"
done

BODY='{"pg-streams":{"pg-stream": ['${BODY%?}']}}'

curl -X PUT -H "Authorization: Basic YWRtaW46YWRtaW4=" -H "Content-Type: application/json" -H "Cache-Control: no-cache" -H "Postman-Token: 9005870c-900b-2e2e-0902-ef2009bb0ff7" -d "$BODY" "http://localhost:8183/restconf/config/sample-plugin:sample-plugin/pg-streams"