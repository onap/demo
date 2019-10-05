# Istio virtualservice for Grafana to be exposed on a specific host

The following values has to be updated depending on the Cluster's installation in values.yaml

1. allowedhosts - URI of Service on which Grafana is accessed
2. gateways - Name of the Istio gateway to which this virtualservice will be connected to
3. destinationhost - Grafana service's name

# Installation
1. helm install grafana-istio-virtualservice -n edge1

NOTE - The namespace (edge1) is used an example. This has to be changed to be namespace where Grafana is deployed.
