1. Install required packages with pipenv `pipenv install`
2. Run virtual environment `pipenv shell`
3. Add kubeconfig files, one for ONAP cluster, and one for k8s cluster that will host vFW:
   - `artifacts/cluster_kubeconfig`
   - `artifacts/onap_kubeconfig`
4. Modify `config.py`:
   - CLOUD_REGION
   - GLOBAL_CUSTOMER_ID
   - VENDOR
   - SERVICENAME
5. Run script `python create_k8s_region.py` in order to create **k8s cloud region**   
6. Onboard **vFW** `python onboard.py`
7. Instantiate **vFW** `python instantiate.py`
