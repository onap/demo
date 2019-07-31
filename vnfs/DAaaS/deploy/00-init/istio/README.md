
# Instructions to Install Istio ServiceMesh

# a. Install Istio Operator's helm chart
# NOTE - Istio Operator is useful for maintainence and Upgrade to Istio versions

helm install --name=istio-operator --namespace=istio-system istio-operator
