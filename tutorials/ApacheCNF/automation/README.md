# CNF automation

0. Make sure you have python 3.8.x installed and default interpreter, what is required by onap-pythonsdk
1. Install required packages with pipenv `pipenv install`
2. Run virtual environment `pipenv shell`. In case of problems you maye try also `--fancy` option
   
   **In case of problems with pipenv:** `venv` can be used as well. In that case, please install all required python packages in correct version according the list in `Pipfile`
3. Add kubeconfig file for k8s cluster that will host your CNF. Apache can be deployed on any standard cluster but with defult values it need LoadBlanacer
   - `artifacts/kubeconfig`
4. Prepare onboarding packages `cd ../templates/ && make && cd ../automation/`
5. Modify `service_config.yaml`. Please note that the configuration file has yaml syntax but also is jinja templated
   and values defined in the configuration file may be used also in the jinja templaring process. Templating is an iterative process unless all the values are
   not resolved. Please not that in most cases you don't have to modify this file at all, despite the configuration of your k8s cluster.
   We recommend to modify only values from the 'UserParams' section:
   - cnf_name - name of CNF
   - k8s_namespace - k8s namespace to use for deployment of CNF
   - k8s_version - version of the k8s cluster (important for proper helm templating)
   - k8s_region - name of the k8s region that we want to create in ONAP
   - release_name - name of the rleease of the helm application (user for naming of k8s resources)
   - profile_source - source of the k8s profile with values - in our case it may be used to change from LoandBalanser to NodePort service type
   - skip_day_2 - it defined the SKIP_POST_INSTANTIATION flag in SDC models. The value is used in the SDC service model name
6. Verify service_config.yaml by running `python config.py`
7. __Important:__ Before running python scripts, some settings for `onapsdk` with information about ONAP endpoints (and socks) have to be exported. 
   All settings for ONAP instance are located in `automation/onap_settings.py` file. To export that settings please run command inside `pipenv` or `venv` shell
   ```shell
   (automation) ubuntu@onap:~/automation$ export ONAP_PYTHON_SDK_SETTINGS="onap_settings"
   ```
8. Run script `python create_cloud_regions.py` in order to create **k8s or openstack cloud region**
9. Onboard CNF `python onboard.py`
10. Instantiate CNF `python instantiate.py`
11. To run healtcheck operation execute `python healthcheck.py <status_check_max_count>` where <status_check_max_count> [int] (default 1)
    indicates iteration number to run status check in case of failure
12. To run scale operation execute `python scale.py <replica_count>` where <replica_count> [int] (default 1)
    indicates the number of desired replicas of Apache pods
13. Once test is done, CNF service instance can be deleted with `python delete.py` command
