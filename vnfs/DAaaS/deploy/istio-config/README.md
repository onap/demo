# Install Istio configurations for Service connectivity

## Prometheus

```bash
NOTE: Prometheus is Usually consumed by multiple services, 
Thus we need to expose the Prometheus to all of them.

1. Update "allowedhosts" in values.yaml in prometheus-config-virtualservice
with all the servies consuming Prometheus
2. gateways - Name of the Istio gateway to which this
virtualservice will be connected to
3. destinationhost - Prometheus service's name
4. port - port of the Prometheus service

After updating all the required details as per the
specification of deployment run the below command

cd ~/demo/vnfs/DAaaS/deploy/istio-config/prometheus-config

helm install prometheus-istio-connectivity -n edge1
``` 
## Grafana

```bash
NOTE: Grafana can be accessed on multiple hosts,
Update the "allowedhosts" on which the Grafana is
accessed

1. Update "allowedhosts" in values.yaml in grafana-config-virtualservice
with all the host/service names on which Grafana is accessed
2. gateways - Name of the Istio gateway to which this
virtualservice will be connected to
3. destinationhost - Grafana service's name
4. port - Port of the Grafana Service

After updating all the required details as per the
specification of deployment run the below command

cd ~/demo/vnfs/DAaaS/deploy/istio-config/grafana-config

helm install grafana-istio-connectivity -n edge1
```

## Minio
```bash
NOTE: Minio is a Headless Service. From Istio's perspective
Headless service can be accessed only by accessing each pod 
of the service.

To access each pod we need a virtualservice and service entry 
which has pod level entries

1. Update "allowedhosts" in values.yaml in minio-istio-connectivity
with all the host/Service names on which minio is accessed
2. gateways - Name of the Istio gateway to which this
virtualservice will be connected to
3. destinationhost - pod name of minio which needs to be accessed
Template - <POD_NAME>.<SERVICE_NAME>.<NAMESPACE>.<SVC><CLUSTER>.<LOCAL>
4. destinationhostPort - Port of the minio pod

After updating all the required details as per the
specification of deployment run the below command

cd ~/demo/vnfs/DAaaS/deploy/istio-config/minio-config

helm install minio-istio-connectivity -n edge1
```
