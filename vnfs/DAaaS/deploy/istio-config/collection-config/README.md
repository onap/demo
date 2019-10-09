# Istio virtualservice for Collectd, CAdvisor, Node Exporter

The following values has to be updated depending on the Cluster's installation in values.yaml

1. allowedhosts - hostname or FQDN for collection service (used by Prometheus to discover scraping target)
2. gateways - Name of the Istio gateway to which this virtualservice will be connected to
3. destinationhost - collection service name
4. port - port of the collection service

# Installation
1. helm install collectd-istio-connectivity -n edge1
2. helm install cadvisor-istio-connectivity -n edge1
3. helm install node-exporter-istio-connectivity -n edge1

NOTE - The namespace (edge1) is used an example. This has to be changed to be namespace where collectd is deployed.
