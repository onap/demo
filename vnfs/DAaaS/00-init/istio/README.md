Istio Installation

1. Download the Source code
curl -L https://git.io/getLatestIstio | ISTIO_VERSION=1.1.7 sh -

2. Add the ISTIO helm chart repository. “helm repo add istio.io https://storage.googleapis.com/istio-release/releases/1.1.7/charts/”

    NOTE : Make sure the helm client and helm server (tiller) is installed

    Create a namespace istio-system where all the istio components are installed “kubectl create namespace istio-system”

3. Install all the Istio Custom Resource Definitions (CRDs) using kubectl apply


   “helm template istio-init --name istio-init --namespace istio-system | kubectl apply -f -”.

4. Verify that all 53 Istio CRDs were committed to the Kubernetes api-server using the following command:

    “kubectl get crds | grep 'istio.io\|certmanager.k8s.io' | wc -l”

5. Install istio with the sds as the configuration profile.

   “helm template istio --name istio --namespace istio-system --values istio/values-istio-sds-auth.yaml | kubectl apply -f -”

6.  Verify the Installation

    “kubectl get svc -n istio-system” && “kubectl get pods -n istio-system”

   Reference -
1. https://istio.io/docs/setup/kubernetes/install/helm/
2. https://istio.io/docs/tasks/security/auth-sds/
