# vFW_CNF_CDS use case automation

1. Install required packages with pipenv `pipenv install`
2. Run virtual environment `pipenv shell --fancy`
3. Add kubeconfig files, one for ONAP cluster, and one for k8s cluster that will host vFW:
   - `artifacts/cluster_kubeconfig`
   - `artifacts/onap_kubeconfig`
4. Prepare onboarding packages `cd ../templates/ && make && cd ../automation/`
5. Modify `config.py`:
   - NATIVE - enables native helm orchestration path in SO
   - CLOUD_REGION
   - GLOBAL_CUSTOMER_ID
   - VENDOR
   - SERVICENAME
   - CUSTOMER_RESOURCE_DEFINITIONS - add list of CRDs to be installed on non KUD k8s cluster
6. Run script `python create_k8s_region.py` in order to create **k8s cloud region**
7. Onboard **vFW** `python onboard.py`
8. Instantiate **vFW** `python instantiate.py`
9. Once test is done, **vFW** service instance can be deleted with `python delete.py` command
