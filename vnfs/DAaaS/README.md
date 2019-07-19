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
DA_WORKING_DIR=$PWD/demo/vnfs/DAaaS/deploy
```

#### Install Rook-Ceph for Persistent Storage
Note: This is unusual but Flex volume path can be different than the default value. values.yaml has the most common flexvolume path configured. In case of errors related to flexvolume please refer to the https://rook.io/docs/rook/v0.9/flexvolume.html#configuring-the-flexvolume-path to find the appropriate flexvolume-path and set it in values.yaml
```bash
cd $DA_WORKING_DIR/00-init/rook-ceph
helm install -n rook . -f values.yaml --namespace=rook-ceph-system
```
Check for the status of the pods in rook-ceph namespace. Once all pods are in Ready state move on to the next section.

```bash
$ kubectl get pods -n rook-ceph-system
NAME                                 READY   STATUS    RESTARTS   AGE
rook-ceph-agent-9wszf                1/1     Running   0          121s
rook-ceph-agent-xnbt8                1/1     Running   0          121s
rook-ceph-operator-bc77d6d75-ltwww   1/1     Running   0          158s
rook-discover-bvj65                  1/1     Running   0          133s
rook-discover-nbfrp                  1/1     Running   0          133s
```
```bash
$ kubectl -n rook-ceph get pod
NAME                                   READY   STATUS      RESTARTS   AGE
rook-ceph-mgr-a-d9dcf5748-5s9ft        1/1     Running     0          77s
rook-ceph-mon-a-7d8f675889-nw5pl       1/1     Running     0          105s
rook-ceph-mon-b-856fdd5cb9-5h2qk       1/1     Running     0          94s
rook-ceph-mon-c-57545897fc-j576h       1/1     Running     0          85s
rook-ceph-osd-0-7cbbbf749f-j8fsd       1/1     Running     0          25s
rook-ceph-osd-1-7f67f9646d-44p7v       1/1     Running     0          25s
rook-ceph-osd-2-6cd4b776ff-v4d68       1/1     Running     0          25s
rook-ceph-osd-prepare-vx2rz            0/2     Completed   0          60s
rook-ceph-tools-5bd5cdb949-j68kk       1/1     Running     0          53s
```

#### Troubleshooting Rook-Ceph installation

In case your machine had rook previously installed successfully or unsuccessfully
and you are attempting a fresh installation of rook operator, you may face some issues.
Lets help you with that.

* First check if there are some rook CRDs existing :
```
kubectl get crds | grep rook
```
If this return results like :
```
otc@otconap7 /var/lib/rook $  kc get crds | grep rook
cephblockpools.ceph.rook.io         2019-07-19T18:19:05Z
cephclusters.ceph.rook.io           2019-07-19T18:19:05Z
cephfilesystems.ceph.rook.io        2019-07-19T18:19:05Z
cephobjectstores.ceph.rook.io       2019-07-19T18:19:05Z
cephobjectstoreusers.ceph.rook.io   2019-07-19T18:19:05Z
volumes.rook.io                     2019-07-19T18:19:05Z
```
then you should delete these previously existing rook based CRDs by generating a delete 
manifest file by these commands and then deleting those files:
```
helm template -n rook . -f values.yaml > ~/delete.yaml
kc delete -f ~/delete.yaml
```

After this, delete the below directory in all the nodes.
```
sudo rm -rf /var/lib/rook/
```
Now, again attempt : 
```
helm install -n rook . -f values.yaml --namespace=rook-ceph-system
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
1. Build the custom collectd image
2. Set COLLECTD_IMAGE_NAME with appropriate image_repository:tag
3. Push the image to docker registry using the command
4. docker push ${COLLECTD_IMAGE_NAME}
5. Edit the values.yaml and change the image repository and tag using 
   COLLECTD_IMAGE_NAME appropriately.
6. Place the collectd.conf in 
   $DA_WORKING_DIR/collection/charts/collectd/resources/config 

7. cd $DA_WORKING_DIR/collection
8. helm install -n cp . -f values.yaml --namespace=edge1
```

#### Verify Collection package
* Check if all pods are up in edge1 namespace
* Check the prometheus UI using port-forwarding port 9090 (default for prometheus service)
```
$ kubectl get pods -n edge1
NAME                                      READY   STATUS    RESTARTS   AGE
cp-cadvisor-8rk2b                       1/1     Running   0          15s
cp-cadvisor-nsjr6                       1/1     Running   0          15s
cp-collectd-h5krd                       1/1     Running   0          23s
cp-collectd-jc9m2                       1/1     Running   0          23s
cp-prometheus-node-exporter-blc6p       1/1     Running   0          17s
cp-prometheus-node-exporter-qbvdx       1/1     Running   0          17s
prometheus-cp-prometheus-prometheus-0   4/4     Running   1          33s

$ kubectl get svc -n edge1
NAME                            TYPE        CLUSTER-IP     EXTERNAL-IP   PORT(S)  
cadvisor                        NodePort    10.43.53.122   <none>        80:30091/TCP
collectd                        ClusterIP   10.43.222.34   <none>        9103/TCP
cp13-prometheus-node-exporter   ClusterIP   10.43.17.242   <none>        9100/TCP
cp13-prometheus-prometheus      NodePort    10.43.26.155   <none>        9090:30090/TCP
prometheus-operated             ClusterIP   None           <none>        9090/TCP
```

#### Install Minio Model repository
* Prerequisite: Dynamic storage provisioner needs to be enabled. Either rook-ceph ($DA_WORKING_DIR/00-init) or another alternate provisioner needs to be enabled.
```bash
cd $DA_WORKING_DIR/minio

Edit the values.yaml to set the credentials to access the minio UI.
Default values are
accessKey: "onapdaas"
secretKey: "onapsecretdaas"

helm install -n minio . -f values.yaml --namespace=edge1
```

#### Onboard an Inference Application
```
TODO
```