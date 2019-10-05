# Istio virtualservice for Prometheus to connect to any service

The following values has to be updated depending on the Cluster's installation in values.yaml

1. allowedhosts - URI of Service consuming Prometheus (or Any service which consumes Prometheus. Eg- Grafana)
2. gateways - Name of the Istio gateway to which this virtualservice will be connected to
3. destinationhost - Prometheus service name
4. port - port of the Prometheus service

# Installation
1. helm install prometheus-istio-connectivity -n edge1

NOTE - The namespace (edge1) is used an example. This has to be changed to be namespace where Prometheus is deployed.
