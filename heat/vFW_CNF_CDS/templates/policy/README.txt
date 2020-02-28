`policy.sh` script allows to create and push naming policy for CNF usecase. Execution of script can be checked by calling `./policy.sh -h`. As pdp doesn't expose it's service externally, this folder needs to be copied to some ONAP pod with bash and curl available (for example, mariadb-galera) and executed within.
Scope of changes and reasoning behind is described in related ticket: https://jira.onap.org/browse/SDNC-1109.

Example execution flow:
```
cd ..
kubectl cp policy onap-mariadb-galera-mariadb-galera-0:/tmp/policy
kubectl exec -it onap-mariadb-galera-mariadb-galera-0 bash
cd /tmp/policy
./policy.sh get #See that CNF naming policy is not uploaded yet
./policy.sh create #Create and push CNF naming policy
./policy.sh get #Verify that now it's visible
```
