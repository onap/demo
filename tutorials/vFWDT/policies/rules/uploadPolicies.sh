#!/bin/bash

# ============LICENSE_START=======================================================
# Copyright (C) 2019 Orange
# ================================================================================
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ============LICENSE_END=========================================================

`./yq > /dev/null 2>&1`

if [ $? -ne 0 ]; then
	echo "Install yq"
	wget -cO yq https://github.com/mikefarah/yq/releases/download/2.4.0/yq_linux_amd64
	chmod 755 yq
fi

`jo -p n=1 > /dev/null 2>&1`

if [ $? -ne 0 ]; then
        echo "Install jo"
        sudo add-apt-repository -y ppa:duggan/jo
        sudo apt update
        sudo apt install jo -y
fi

echo "Uploading policies"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
PDP=`kubectl get pods -o go-template --template '{{range .items}}{{.metadata.name}}{{"\n"}}{{end}}' | grep policy-pdp`

echo $PDP
CMD='createPolicy'
MODE=$1

if [[ $MODE == "U" ]]; then
	CMD='updatePolicy'
fi

echo $CMD
SCRIPT="dt-policies.sh"
echo "#!/bin/bash" > $SCRIPT

for f in $DIR/*.json; do
	NAME=`./yq r $f policyName`
	SCOPE="$(cut -d'.' -f1 <<< $NAME )"
	RULE=`cat $f`
	BODY="\"$RULE\""
	echo "Processing $NAME rule..";
	echo "echo \"$NAME Policy\"" >> $SCRIPT
	BODY=`jo -p configBody="$BODY" -p policyName=$NAME -p policyConfigType=MicroService -p onapName=SampleDemo -p policyScope=$SCOPE`
	LINK="curl -k -v  -X PUT --header 'Content-Type: application/json' --header 'Accept: text/plain' --header 'ClientAuth: cHl0aG9uOnRlc3Q=' --header 'Authorization: Basic dGVzdHBkcDphbHBoYTEyMw==' --header 'Environment: TEST' -d '$BODY' 'https://localhost:8081/pdp/api/$CMD'"
	LINK="${LINK/\"\\\"{\\n/\"{}"
	LINK="${LINK/\\\"\"/\"}"
	LINK="${LINK//\\n/ }"
	echo "$LINK" >> $SCRIPT
	BODY=`jo -p policyType=MicroService -p pdpGroup=default -p policyName=$NAME`
	LINK="curl -k -v  -X PUT --header 'Content-Type: application/json' --header 'Accept: text/plain' --header 'ClientAuth: cHl0aG9uOnRlc3Q=' --header 'Authorization: Basic dGVzdHBkcDphbHBoYTEyMw==' --header 'Environment: TEST' -d '$BODY' 'https://localhost:8081/pdp/api/pushPolicy'"
	echo "$LINK" >> $SCRIPT
done

`kubectl cp $SCRIPT onap/$PDP:/tmp/policy-install`
`kubectl exec $PDP -- chmod 755 $SCRIPT`
`kubectl exec $PDP -- ./$SCRIPT`

