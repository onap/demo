from __future__ import print_function
from os import environ
import logging
import sysconfig
import requests
from requests.exceptions import HTTPError


#LABELS = [ 'irate(http_requests_total{code="200"}[1m])', 'collectd_cpu_percent{job="collectd", exported_instance="an11-31"}[1m]' ]
#LABELS = ['irate(collectd_cpufreq{exported_instance="otconap7",cpufreq="1"}[2m])', 'go_info']
API_VERSION = '/api/v1/query'
LIST_OF_ENV_VARIABLES = ["PROMETHEUS_SERVER_URL"]
MAP_ENV_VARIABLES = dict()
#MAP_ENV_VARIABLES['PROMETHEUS_SERVER_URL']='http://172.25.103.1:30090' #to be deleted
LOG = logging.getLogger(__name__)

def set_log_config():
    logging.basicConfig(format='%(asctime)s ::%(filename)s :: %(funcName)s :: %(levelname)s :: %(message)s', 
                    datefmt='%m-%d-%Y %I:%M:%S%p',
                    level=logging.DEBUG, 
                    filename='promql_api.log',
                    filemode='w')
    LOG.info("Set the log configs.")


def load_and_validate_env_vars(list_of_env_vars):
    LOG.info("Loading the env variables ...")
    for env_var in list_of_env_vars:
        if env_var in environ:
            LOG.info("Found env variable: {} ".format(env_var.upper()))
            MAP_ENV_VARIABLES[env_var.upper()] = environ.get(env_var)
        else:
            #MAP_ENV_VARIABLES['PROMETHEUS_SERVER_URL']='http://172.25.103.1:30090' # to be deleted
            LOG.error("Env var: {} not found ! ".format(env_var.upper()))
            raise KeyError("Env variable: {} not found ! ".format(env_var.upper()))


def query_m3db(LABELS):
    """
    Input parameters:
        LABELS : a list of the LABELS
    Return:
        returns a list of  result sets of different labels
    """

    LOG.info("Forming the get request ...")
    list_of_substrings = []
    params_map = {}
    list_of_result_sets = []
    list_of_substrings.append(MAP_ENV_VARIABLES['PROMETHEUS_SERVER_URL'])
    list_of_substrings.append(API_VERSION)
    url = ''.join(list_of_substrings)
    
    for each_label in LABELS:
        params_map['query'] = each_label
        try:
            LOG.info('API request::: URL: {} '.format(url))
            LOG.info('API request::: params: {} '.format(params_map))
            response = requests.get(url, params=params_map)
            response.raise_for_status()
        except HTTPError as http_err:
            print(f'HTTP error occurred: {http_err}')
            return None  
        except Exception as err:
            print(f'Other error occurred: {err}')
            return None 
        else:
            
            results = response.json()['data']['result']
            LOG.info('::::::::::RESULTS::::::::::::: {}'.format(each_label))
            for each_result in results:
                LOG.info(each_result)
            list_of_result_sets.append(results)
    return list_of_result_sets


# def main():
#     set_log_config()
#     load_and_validate_env_vars(LIST_OF_ENV_VARIABLES)
#     query_m3db(LABELS)

# if __name__ == "__main__":
#     main()