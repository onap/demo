# Istio virtualservice for M3DB to connect to any service

The following values has to be updated depending on the Cluster's installation in values.yaml

1. allowedhosts - URI of Service consuming M3DB (or Any service which consumes M3DB. Eg- Prometheus, Grafana etc)
2. gateways - Name of the Istio gateway to which this virtualservice will be connected to
3. destinationhost - m3coordinator service name
4. port - port of the m3coordinator service

# Installation
1. helm install m3db-istio-connectivity -n training

NOTE - The namespace (training) is used an example. This has to be changed to be namespace where m3db is deployed.
