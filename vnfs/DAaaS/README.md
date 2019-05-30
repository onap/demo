# Distributed Analytics Framework
## Install

#### Pre-requisites
| Required   | Version |
|------------|---------|
| Kubernetes | 1.12.3+ |
| Docker CE  | 18.09+  |
| Helm       | 2.12.1+ |
#### Download Framework
```bash
git clone https://github.com/onap/demo.git
DA_WORKING_DIR=$PWD/demo/vnfs/DAaaS
cd $DA_WORKING_DIR
```

#### Install Rook-Ceph for Persistent Storage
Note: This is unusual but Flex volume path can be different than the default value. values.yaml has the most common flexvolume path configured. In case of errors related to flexvolume please refer to the https://rook.io/docs/rook/v0.9/flexvolume.html#configuring-the-flexvolume-path to find the appropriate flexvolume-path and set it in values.yaml
```bash
cd 00-init/rook-ceph
helm install -n rook . -f values.yaml --namespace=rook-ceph-system
```
Check for the status of the pods in rook-ceph namespace. Once all pods are in Ready state move on to the next section.
```bash
$ kubectl -n rook-ceph get pod
NAME                                   READY   STATUS      RESTARTS   AGE
rook-ceph-agent-4zkg8                  1/1     Running     0          140s
rook-ceph-mgr-a-d9dcf5748-5s9ft        1/1     Running     0          77s
rook-ceph-mon-a-7d8f675889-nw5pl       1/1     Running     0          105s
rook-ceph-mon-b-856fdd5cb9-5h2qk       1/1     Running     0          94s
rook-ceph-mon-c-57545897fc-j576h       1/1     Running     0          85s
rook-ceph-osd-0-7cbbbf749f-j8fsd       1/1     Running     0          25s
rook-ceph-osd-1-7f67f9646d-44p7v       1/1     Running     0          25s
rook-ceph-osd-2-6cd4b776ff-v4d68       1/1     Running     0          25s
rook-ceph-osd-prepare-vx2rz            0/2     Completed   0          60s
rook-discover-dhkb8                    1/1     Running     0          140s
```

#### Install Operator package
```bash
cd $DA_WORKING_DIR/operator
helm install -n operator . -f values.yaml --namespace=operator
```
Check for the status of the pods in operator namespace. Check if Prometheus operator pods are in Ready state.
```bash
kubectl get pods -n operator
NAME                                                      READY   STATUS    RESTARTS
m3db-operator-0                                           1/1     Running   0       -etcd-operator-etcd-backup-operator-6cdc577f7d-ltgsr      1/1     Running   0
op-etcd-operator-etcd-operator-79fd99f8b7-fdc7p           1/1     Running   0
op-etcd-operator-etcd-restore-operator-855f7478bf-r7qxp   1/1     Running   0
op-prometheus-operator-operator-5c9b87965b-wjtw5          1/1     Running   1
op-sparkoperator-6cb4db884c-75rcd                         1/1     Running   0
strimzi-cluster-operator-5bffdd7b85-rlrvj                 1/1     Running   0
```

#### Install Collection package
Note: Collectd.conf is avaliable in $DA_WORKING_DIR/collection/charts/collectd/resources/config directory. Any valid collectd.conf can be placed here.
```bash
Default (For custom collectd skip this section)
=======
cd $DA_WORKING_DIR/collection
helm install -n cp . -f values.yaml --namespace=edge1

Custom Collectd
===============
Build the image and set the image name with tag to COLLECTD_IMAGE_NAME
Push the image to docker registry using the command
docker push dcr.default.svc.local:32000/${COLLECTD_IMAGE_NAME}
Edit the values.yaml and change the image name as the value of COLLECTD_IMAGE_NAME
place the collectd.conf in $DA_WORKING_DIR/collection/charts/collectd/resources/config directory

cd $DA_WORKING_DIR/collection
helm install -n cp . -f values.yaml --namespace=edge1
```

#### Verify Collection package
```
TODO
1. Check if all pods are up uin edge1 namespace
2. Check the prometheus UI using the port 30090
```
#### Onboard an Inference Application
```
TODO
```