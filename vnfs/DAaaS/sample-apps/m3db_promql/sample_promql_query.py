from promql_api.prom_ql_api import query
import pprint

QUERY_STRING = ['irate(collectd_cpufreq{exported_instance="otconap7",cpufreq="1"}[2m])']

#Other examples
#QUERY_STRING = [ 'irate(http_requests_total{code="200"}[1m])', 'collectd_cpu_percent{job="collectd", exported_instance="otconap7"}[1m]' ]
#QUERY_STRING = ['irate(collectd_cpufreq{exported_instance="otconap7",cpufreq="1"}[2m])', 'go_info']

def main():
    list_of_result_sets = query(QUERY_STRING)
    if list_of_result_sets:
        for each_result in list_of_result_sets:
                pprint.pprint(each_result)

if __name__ == "__main__":
     main()