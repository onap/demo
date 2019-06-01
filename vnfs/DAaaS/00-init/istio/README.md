# Steps for Instaling Istio with Istio- Operator

# Step 1 - Add the helm repo
helm repo add banzaicloud-stable https://kubernetes-charts.banzaicloud.com/

# Step 2 - Create the template for deployment of Istio with SDS configuration from the helm chart
helm template istio-instance --name istio --namespace <Namespace> > istio-sds.yaml

# Step 3 - Apply the configuration to required namespace
kubectl apply -f istio-sds.yaml -n <namespace>
