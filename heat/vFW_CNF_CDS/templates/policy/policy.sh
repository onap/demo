#!/bin/bash

set -e

check(){
    if ! which curl >/dev/null 2>&1; then
        echo "Fatal error, curl command not available" >&2
        return 1
    fi

    for arg; do
        if ! test -f "$arg"; then
            echo "Fatal error, REST payload - $arg - not found in current directory" >&2
            return 1
        fi
    done
}

query_policy(){
    local mode="$1"
    local content="$2"
    local url="$3"

    declare -a flags=(-kf -H 'Content-Type: application/json'
                      -H 'ClientAuth: cHl0aG9uOnRlc3Q=' -u testpdp:alpha123
                      -H 'Environment: TEST')

    curl -X "$mode" -d @"$content" "${flags[@]}" "${PDP_URL:-https://pdp:8081}/$url"
}

case "$1" in
    -h|--help|help|?|"")
        echo "Script usage:"
        echo "$0 get|verify - query policy for CNF policy"
        echo "$0 create - Create CNF naming policy and push it"
        echo "$0 update - Update CNF naming policy and push it"
        ;;
    get|verify)
        check get.json
        query_policy POST get.json pdp/api/getConfig
        ;;
    create)
        check create.json push.json
        query_policy PUT create.json pdp/api/createPolicy
        query_policy PUT push.json pdp/api/pushPolicy
        ;;
    update)
        check create.json push.json
        query_policy PUT create.json pdp/api/updatePolicy
        query_policy PUT push.json pdp/api/pushPolicy
        ;;
    *)
        echo "Wrong usage, check '$0 -h'" >&2
        exit 1
        ;;
esac
