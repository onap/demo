#!/bin/bash
#   COPYRIGHT NOTICE STARTS HERE

#   Copyright 2020 . Samsung Electronics Co., Ltd.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

#   COPYRIGHT NOTICE ENDS HERE

set -e

get_pod_name() {
    kubectl -n ${NAMESPACE} get pod -l app="$1" -o jsonpath='{ .items[0].metadata.name }'
}

exec_in_pod() {
    local pod="$1"
    local container="$2"
    shift 2
    local cmd="$@"
    kubectl --namespace ${NAMESPACE} exec ${pod} --container ${container} -- sh -c "${cmd}"
}

pod_logs() {
    local pod="$1"
    local container="$2"
    local options="$3"
    shift 3
    kubectl --namespace ${NAMESPACE} logs ${pod} --container ${container} ${options:+"${options}"}
}

k8s_tail() {
    local operation="$1"
    local file_to_tail="$2"
    local pod_label="$3"
    local container="$4"
    if [ -z ${container} ]; then
        container=${pod_label}
    fi
    mkdir -p ${CACHE_FILE_DIR}
    local pod=$(get_pod_name ${pod_label})
    file_path_savable_form="$(sed 's#/#__#g' <<<"$file_to_tail")"
    LINECOUNT_CACHE_FILE=${CACHE_FILE_DIR}/${pod}-${container}-${file_path_savable_form}
    local line_count=0
    if [ "${operation}" == "start" ]; then
        if [ "${file_to_tail}" == "POD_LOG" ]; then
            line_count=$(pod_logs ${pod} ${container} "" | wc -l)
        else
            if ! line_count=$(exec_in_pod ${pod} ${container} "wc -l ${file_to_tail}"); then
                echo "Failed to get file ${file_to_tail} line count, maybe it does not exist. Using linecount 0." >&2
                line_count=0
            else
                # parse out linecount from wc -l output
                line_count=$(echo -e "${line_count}" | tail -1 | cut -d' ' -f1)
            fi
        fi
        echo "${line_count}" > ${LINECOUNT_CACHE_FILE}
        echo "Saved file ${file_to_tail} of POD ${pod_label} linecount ${line_count} into file ${LINECOUNT_CACHE_FILE}"
    fi
    if [ "${operation}" == "collect" ]; then
        if [ ! -f ${LINECOUNT_CACHE_FILE} ]; then
            echo "Linecount cache file ${LINECOUNT_CACHE_FILE} not found. Either tailing was not started or POD has restarted. Colected from log beginning." >&2
            start_line=0
        else
            start_line=$(cat ${LINECOUNT_CACHE_FILE})
        fi
        echo "Tail pod's ${pod_label} container ${container} file ${file_to_tail} starting from line ${start_line} onwards" >&2
        if [ "${file_to_tail}" == "POD_LOG" ]; then
            current_line_count=$(pod_logs ${pod} ${container} "" | wc -l)
            lines_from_end=$(( ${current_line_count}-${start_line} ))
            echo "Execute kubectl logs for ${pod} POD's ${container} container. Log ${lines_from_end} lines from end"
            pod_logs ${pod} ${container} --tail=${lines_from_end}
        else
            echo "Execute cmd "tail -n +${start_line} ${file_to_tail}" inside ${pod} POD's ${container} container"
            exec_in_pod ${pod} ${container} "tail -n +${start_line} ${file_to_tail}"
        fi
    fi
}

tail_it() {
    local operation=$1
    for tail in "${TAILS[@]}"
    do
        tailarray=(${tail})
        result=$(k8s_tail ${operation} ${tailarray[@]})
        if [ "${operation}" == "collect" ]; then
            mkdir -p ${RESULT_DIR}
            log_file=$(basename ${tailarray[0]})
            if [ "${log_file}" == "POD_LOG" ]; then
               log_file=${log_file}.log
            fi
            pod=${tailarray[1]}
            container="${pod:-tailarray[2]}"
            out_file=${RESULT_DIR}/${pod}_${container}_${log_file}
            echo -e "${result}" > ${out_file}
            echo "Saved tail content to log file ${out_file}"
        else
            echo -e "${result}"
        fi
    done
}


##### MAIN #####
if [ -f ./tail_config ]; then
. ./tail_config
fi
NAMESPACE=${NAMESPACE:-onap}
CACHE_FILE_DIR=${CACHE_FILE_DIR:-.k8s_tail}
RESULT_DIR=${RESULT_DIR:-./tail_results}
DEFAULT_TAILS=()
TAILS=("${TAILS[@]:-${DEFAULT_TAILS[@]}}")

case "$1" in
    -h|--help|help|?|"")
        echo "Script usage:"
        echo "$0 start   - Start pods' log tailing"
        echo "$0 collect - Collect all logs currently tailed or get all logs as whole"
        echo ""
        echo "If start is not called before collect is called, collect gathers all logs completely. "
        echo "Otherwise logs are collected only from the linecount of the time of start call."
        echo ""
        echo "Log files collected is configured with 'tail_config' file in the same directory with the script."
        echo "Config file needs to fontain bash array variable named TAILS=() and format of each entry in array is"
        echo "    <file path> <POD's app label name> [<container name>]"
        echo "where <file path> is actual file path inside the pod or special string 'POD_LOG' that means kubectl logs"
        echo "where optional <container name> is to specify POD's container if many containers in the pod. By default same name as pod is used."
        echo ""
        echo 'Example: TAILS=('
        echo '  "/app/logs/apih/metrics.log so"'
        echo '  "/app/logs/bpmn/debug.log so-bpmn-infra"'
        echo '  "/app/logs/vnfm-adapter/debug.log so-vnfm-adapter"'
        echo '  "/var/log/onap/sdnc/karaf.log sdnc"'
        echo '  "/tmp/app.out network-name-gen"'
        echo '  "POD_LOG cds-blueprints-processor"'
        echo '  "POD_LOG multicloud multicloud"'
        echo '  "POD_LOG multicloud-k8s multicloud-k8s"'
        echo '  "POD_LOG multicloud-k8s framework-artifactbroker"'
        echo '  "/app/logs/openstack/error.log so-openstack-adapter"'
        echo '  "/app/logs/openstack/debug.log so-openstack-adapter"'
        echo ' )'
        echo ""
        ;;
    start|collect)
        tail_it $1
        ;;
    *)
        echo "Wrong usage, check '$0 -h'" >&2
        exit 1
        ;;
esac
