## What does this API do ?
This api as of now provides a function which takes in a lits of 'LABELS' of prometheus
and returns the corresponding result_sets in a list.

For eg:
If the labels is

```
LABELS = ['irate(collectd_cpufreq{exported_instance="otconap7",cpufreq="1"}[2m])']
```

The return is:

```
[{'metric': {'cpufreq': '1',
             'endpoint': 'collectd-prometheus',
             'exported_instance': 'otconap7',
             'instance': '172.25.103.1:9103',
             'job': 'collectd',
             'namespace': 'edge1',
             'pod': 'plundering-liger-collectd-wz7xg',
             'service': 'collectd'},
  'value': [1559177169.415, '119727200']}]
```

## How to use this API ?

```
1. Copy the directory 'promql_api' to your working directory. 
```

```
2. Import the API function: query_m3db
from promql_api.prom_ql_api import query_m3db
```

```
3. have a global or local variable as 'LABELS'
LABELS = ['irate(collectd_cpufreq{exported_instance="otconap7",cpufreq="1"}[2m])']
```

```
4. Store the result set in a list:
list_of_result_sets = query_m3db(LABELS)
```

## How to troubleshoot ?

* Check the sample file - sample_promql_query.py in the repo.
* Make sure the file __init__.py is present in promql_api directory after you copy the directory.
