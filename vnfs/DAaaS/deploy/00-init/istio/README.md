# Steps for Instaling Istio with Istio- Operator

# Step 1 - Add the helm repo
helm template istio-instance --name istio --namespace <Namespace> > istio-sds.yaml

# Step 2 - Apply the configuration to required namespace
kubectl apply -f istio-sds.yaml -n <namespace>
