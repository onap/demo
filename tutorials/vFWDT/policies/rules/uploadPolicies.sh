#!/bin/bash

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

