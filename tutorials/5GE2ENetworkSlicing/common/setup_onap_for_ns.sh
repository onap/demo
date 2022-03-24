#!/bin/bash
# This scripts preloads ONAP with some relevant entries required to orchestrate NS
# Some steps may fail if script is ran non-first time on environment so strict error checking is turned off
set +e -x
aai_curl() {
  curl -ksSL -H "X-TransactionId: $RANDOM" -H "X-FromAppId: Jenkins" -H "Content-Type: application/json" -H "Accept: application/json" \
    -H "Authorization: Basic QUFJOkFBSQ==" "$@"
}
MASTER_IP="${1:?Missing mandatory positional parameter - Master IP}"
TENANT_ID=${2:?Missing mandatory positional parameter - Tenant ID}
TENANT_NAME=${3:?Missing mandatory positional parameter - Tenant Name}

aai_curl -X PUT -d '{ "global-customer-id":"5GCustomer", "subscriber-name":"5GCustomer", "subscriber-type":"INFRA" }' "https://${MASTER_IP}:30233/aai/v23/business/customers/customer/5GCustomer"

aai_curl -X PUT "https://${MASTER_IP}:30233/aai/v23/business/customers/customer/5GCustomer/service-subscriptions/service-subscription/5G"

aai_curl -X PUT "https://${MASTER_IP}:30233/aai/v23/cloud-infrastructure/complexes/complex/clli2" \
-d '{
    "physical-location-id": "clli2",
    "physical-location-type": "office",
    "street1": "Dummy 1",
    "city": "Kraków",
    "postal-code": "30-000",
    "country": "Poland",
    "region": "Smaller Poland"
}'

echo "Handling AAI Entries"
aai_curl -X PUT "https://${MASTER_IP}:30233/aai/v23/cloud-infrastructure/cloud-regions/cloud-region/k8scloudowner4/k8sregionfour" \
  --data '{
      "cloud-owner": "k8scloudowner4",
      "cloud-region-id": "k8sregionfour",
      "cloud-type": "k8s",
      "owner-defined-type": "t1",
      "cloud-region-version": "1.0",
      "complex-name": "clli2",
      "cloud-zone": "CloudZone",
      "sriov-automation": false
  }'
aai_curl -X PUT "https://${MASTER_IP}:30233/aai/v23/cloud-infrastructure/cloud-regions/cloud-region/k8scloudowner4/k8sregionfour/vip-ipv4-address-list/${MASTER_IP}" \
  --data "{
      \"vip-ipv4-address\": \"${MASTER_IP}\"
  }"
aai_curl -X PUT "https://${MASTER_IP}:30233/aai/v23/cloud-infrastructure/cloud-regions/cloud-region/k8scloudowner4/k8sregionfour/relationship-list/relationship" \
  --data '{
      "related-to": "complex",
      "related-link": "/aai/v23/cloud-infrastructure/complexes/complex/clli2",
      "relationship-data": [
          {
            "relationship-key": "complex.physical-location-id",
            "relationship-value": "clli2"
          }
      ]
  }'
aai_curl -X PUT "https://${MASTER_IP}:30233/aai/v23/cloud-infrastructure/cloud-regions/cloud-region/k8scloudowner4/k8sregionfour/availability-zones/availability-zone/k8savz" \
  --data '{
      "availability-zone-name": "k8savz",
      "hypervisor-type": "k8s"
  }'
aai_curl -X PUT "https://${MASTER_IP}:30233/aai/v23/cloud-infrastructure/cloud-regions/cloud-region/k8scloudowner4/k8sregionfour/tenants/tenant/${TENANT_ID}" \
  --data '{
      "tenant-id": "'$TENANT_ID'",
      "tenant-name": "'$TENANT_NAME'",
      "relationship-list": {
          "relationship": [
          {
              "related-to": "service-subscription",
              "relationship-label": "org.onap.relationships.inventory.Uses",
              "related-link": "/aai/v23/business/customers/customer/5GCustomer/service-subscriptions/service-subscription/5G",
              "relationship-data": [
                  {
                      "relationship-key": "customer.global-customer-id",
                      "relationship-value": "5GCustomer"
                  },
                  {
                      "relationship-key": "service-subscription.service-type",
                      "relationship-value": "5G"
                  }
              ]
        }
        ]
      }
  }'

echo "Configuring k8splugin"
curl -ksSL -X POST "https://${MASTER_IP}:30283/api/multicloud-k8s/v1/v1/connectivity-info" \
  --header "Content-Type: multipart/form-data" \
  --form "file=@${HOME}/.kube/config" \
  --form metadata='{
    "cloud-region": "k8sregionfour",
    "cloud-owner": "k8scloudowner4"
  }'

echo "Configuring SO"
pass=$(kubectl get "$(kubectl get secrets -o name | grep mariadb-galera-db-root-password)" \
  -o jsonpath="{.data.password}" | base64 --decode)
kubectl -n onap exec onap-mariadb-galera-0 -- \
  mysql -uroot -p"${pass}" -D catalogdb -e \
  'INSERT IGNORE INTO
    cloud_sites(ID, REGION_ID, IDENTITY_SERVICE_ID, CLOUD_VERSION, CLLI, ORCHESTRATOR)
    values("k8sregionfour", "k8sregionfour", "DEFAULT_KEYSTONE", "2.5", "clli2", "multicloud");'
