## What does this API do ?
This api as of now provides a function which takes in a list of 'QUERY_STRINGs' with respect to prometheus
and returns the corresponding result_sets in a list.

For eg:
If the QUERY_STRING is

```
QUERY_STRING = ['irate(collectd_cpufreq{exported_instance="otconap7",cpufreq="1"}[2m])']
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
i
## How to use this API ?

```
1. Copy the directory 'promql_api' to your working directory. 
```

```
2. Import the API function: query
from promql_api.prom_ql_api import query
```

```
3. have a global or local variable as 'QUERY_STRING'
QUERY_STRING = ['irate(collectd_cpufreq{exported_instance="otconap7",cpufreq="1"}[2m])']
```

```
4. Store the result set in a list:
list_of_result_sets = query(QUERY_STRING)
```

## Troubleshooting tips

* Check the sample file - sample_promql_query.py in the repo ( sample-apps/m3db_promql)
* Make sure the file "__init__.py" is present in promql_api directory after you copy the directory.
* Make sure environment variables like "DATA_ENDPOINT" are correctly set.
* For custom and advanced querying https://prometheus.io/docs/prometheus/latest/querying/api/
* Logs are generated in the directory where the query function is called.
* sample log file - promql_api.log

```
 05-30-2019 08:47:53PM ::prom_ql_api.py :: set_log_config :: INFO :: Set the log configs.
 05-30-2019 08:47:53PM ::prom_ql_api.py :: load_and_validate_env_vars :: INFO :: Loading the env variables ...
 05-30-2019 08:47:53PM ::prom_ql_api.py :: load_and_validate_env_vars :: ERROR :: Env var: DATA_ENDPOINT not         found !
 05-30-2019 08:47:53PM ::prom_ql_api.py :: query :: INFO :: Forming the get request ...
 05-30-2019 08:47:53PM ::prom_ql_api.py :: query :: INFO :: API request::: URL: http://172.25.103.1:30090/api/v1/    query
 05-30-2019 08:47:53PM ::prom_ql_api.py :: query :: INFO :: API request::: params: {'query':                         'irate(collectd_cpufreq{exported_instance="otconap7",cpufreq="1"}[2m])'}
 05-30-2019 08:47:53PM ::connectionpool.py :: _new_conn :: DEBUG :: Starting new HTTP connection (1): 172.25.103.1   05-30-2019 08:47:53PM ::connectionpool.py :: _make_request :: DEBUG :: http://172.25.103.1:30090 "GET /api/v1/      query?query=irate%28collectd_cpufreq%7Bexported_instance%3D%22otconap7%22%2Ccpufreq%3D%221%22%7D%5B2m%5D%29 HTTP/1. 1" 200 370
 05-30-2019 08:47:53PM ::prom_ql_api.py :: query :: INFO :: ::::::::::RESULTS:::::::::::::                           irate(collectd_cpufreq{exported_instance="otconap7",cpufreq="1"}[2m])
 05-30-2019 08:47:53PM ::prom_ql_api.py :: query :: INFO :: {'metric': {'cpufreq': '1', 'endpoint': 'collectd-       prometheus', 'exported_instance': 'otconap7', 'instance': '172.25.103.1:9103', 'job': 'collectd', 'namespace':      'edge1', 'pod': 'plundering-liger-collectd-wz7xg', 'service': 'collectd'}, 'value': [1559249299.084, '236300']}
 ```

 * Tested Error scenario: Configure QUERY_STRING as :
 ```
 QUERY_STRING = ['irate(collectd_cpufreq{exported_instance="otconap7", cpufreq="1"}[2m])', 'collectd_cpu_percent{job="collectd" exported_instance="an11-31"}[1m]']
 ```
 O/P :
 ```
 Check logs..HTTP error occurred: 400 Client Error: Bad Request for url: http://172.25.103.1:30090/api/v1/query?query=collectd_cpu_percent%7Bjob%3D%22collectd%22+exported_instance%3D%22an11-31%22%7D%5B1m%5D
[{'metric': {'cpufreq': '1',
             'endpoint': 'collectd-prometheus',
             'exported_instance': 'otconap7',
             'instance': '172.25.103.1:9103',
             'job': 'collectd',
             'namespace': 'edge1',
             'pod': 'plundering-liger-collectd-wz7xg',
             'service': 'collectd'},
  'value': [1559343866.631, '119798600']}]
{'error': 'parse error at char 37: missing comma before next identifier '
          '"exported_instance"',
 'errorType': 'bad_data'}
```
