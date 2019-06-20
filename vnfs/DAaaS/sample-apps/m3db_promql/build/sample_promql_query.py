# -------------------------------------------------------------------------
#   Copyright (c) 2019 Intel Corporation Intellectual Property
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
#
# -------------------------------------------------------------------------


from promql_api.prom_ql_api import query
from promql_api.prom_ql_api import query_range
import pprint

QUERY_STRING = ['irate(collectd_cpufreq{exported_instance="otconap7",cpufreq="1"}[2m])']
MAP_OF_PARAMETERS = {'query': 'up', 'start': '2019-06-19T20:10:30.781Z', 'end': '2019-06-19T20:10:45.781Z', 'step': '15s'}

#Other examples
#QUERY_STRING = [ 'irate(http_requests_total{code="200"}[1m])', 'collectd_cpu_percent{job="collectd", exported_instance="otconap7"}[1m]' ]
#QUERY_STRING = ['irate(collectd_cpufreq{exported_instance="otconap7",cpufreq="1"}[2m])', 'go_info']
#MAP_OF_PARAMETERS = {'query': 'up', 'start': '2019-06-19T20:10:30.781Z', 'end': '2019-06-19T20:10:45.781Z', 'step': '15s', 'timeout':'600s'}

def demo_query():
        list_of_result_sets = query(QUERY_STRING)
        if list_of_result_sets:
                for each_result in list_of_result_sets:
                    pprint.pprint(each_result)

def demo_query_range():
        list_of_result_sets = query_range(MAP_OF_PARAMETERS)
        pprint.pprint(list_of_result_sets)


def main():
        demo_query()
        demo_query_range()

    
if __name__ == "__main__":
        main()
