from promql_api.prom_ql_api import query_m3db
import pprint

LABELS = ['irate(collectd_cpufreq{exported_instance="otconap7",cpufreq="1"}[2m])']

def main():
    list_of_result_sets = query_m3db(LABELS)
    for each_result in list_of_result_sets:
        pprint.pprint(each_result)

if __name__ == "__main__":
     main()