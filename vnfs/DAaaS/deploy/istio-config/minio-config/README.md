# Istio virtualservice for a specifc minio pod to be exposed to any service

The following values has to be updated depending on the Cluster's installation in values.yaml

1. allowedhosts - URI of Service on which minio is accessed
2. gateways - Name of the Istio gateway to which this virtualservice will be connected to
3. destinationhost - Specific pod name of the minio, Since it is a headless service

# Installation
1. helm install minio-istio-virtualservice -n edge1

NOTE - The namespace (edge1) is used an example. This has to be changed to be namespace where minio is deployed.

# Istio serviceentry needs to be Added for a Headless service like Minio

# Installation
1. helm install minio-istio-serviceentry -n edge1
