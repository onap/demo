# vFW_CNF_CDS use case automation

0. Make sure you have python 3.8.x installed and default interpreter, what is required by onap-pythonsdk
1. Install required packages with pipenv `pipenv install`
2. Run virtual environment `pipenv shell`. In case of problems use `--fancy` option
   
   **In case of problems with pipenv:** `venv` can be used as well. In that case, please install all required python packages in correct version according the list in `Pipfile`
3. Add kubeconfig file for k8s cluster that will host vFW
   - `artifacts/cluster_kubeconfig`
4. Prepare onboarding packages `cd ../templates/ && make && cd ../automation/`
5. Modify `config.py`:
   - NATIVE - enables native helm orchestration path in SO
   - CLOUD_REGION
   - GLOBAL_CUSTOMER_ID
   - VENDOR
   - SERVICENAME
   - CUSTOMER_RESOURCE_DEFINITIONS - add list of CRDs to be installed on non KUD k8s cluster
   - VNF_PARAM_LIST - list of parameters to pass for VNF creation process
   - VF_MODULE_PARAM_LIST - list of parameters to pass for VF Module creation
6. __Important:__ Before running python scripts, some settings for `onapsdk` with information about ONAP endpoints (and socks) have to be exported. 
   All settings for ONAP instance are located in `automation/onap_settings.py` file. To export that settings please run command inside `pipenv` or `venv` shell
   ```shell
   (automation) ubuntu@onap:~/automation$ export ONAP_PYTHON_SDK_SETTINGS="onap_settings"
   ```
7. Run script `python create_k8s_region.py` in order to create **k8s cloud region**
8. Onboard **vFW** `python onboard.py`
9. Instantiate **vFW** `python instantiate.py`
10. Once test is done, **vFW** service instance can be deleted with `python delete.py` command

