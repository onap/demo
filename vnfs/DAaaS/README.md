# Distributed Analytics Framework


## Pre-requisites
| Required   | Version |
|------------|---------|
| Kubernetes | 1.12.3+ |
| Docker CE  | 18.09+  |
| Helm       | >=2.12.1 and <=2.13.1 |
## Download Framework
```bash
git clone https://github.com/onap/demo.git
DA_WORKING_DIR=$PWD/demo/vnfs/DAaaS/deploy
```

## Install Istio Service Mesh

## Istio is installed in two Steps
```bash
1. Istio-Operator
2. Istio-config
```

## Download the Istio Installation repo

```bash
cd $DA_WORKING_DIR/00-init
helm install --name=istio-operator istio-operator --namespace=istio-system
cd $DA_WORKING_DIR/00-init/istio
helm install --name istio istio-instance --namespace istio-system
```

## Install Metallb to act as a Loadbalancer
```bash
cd  $DA_WORKING_DIR/00-init
NOTE: Update the IP Address Ranges before you Install Metallb
NOTE: If you are using a single IP, use <IP>/32 format
helm install --name metallb metallb --namespace metallb-system
```

## Install Rook-Ceph for Persistent Storage
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
otc@otconap7 /var/lib/rook $  kubectl get crds | grep rook
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
kubectl delete -f ~/delete.yaml
```

After this, delete the below directory in all the nodes.
```
sudo rm -rf /var/lib/rook/
```
Now, again attempt : 
```
helm install -n rook . -f values.yaml --namespace=rook-ceph-system
```

## Install Operator package
### Build docker images
#### collectd-operator
```bash
cd $DA_WORKING_DIR/../microservices

## Note: The image tag and respository in the Collectd-operator helm charts needs to match the IMAGE_NAME
IMAGE_NAME=dcr.cluster.local:32644/collectd-operator:latest
./build_image.sh collectd-operator $IMAGE_NAME
```
#### visualization-operator
```bash
cd $DA_WORKING_DIR/../microservices

## Note: The image tag and respository in the Visualization-operator helm charts needs to match the IMAGE_NAME
IMAGE_NAME=dcr.cluster.local:32644/visualization-operator:latest
./build_image.sh visualization-operator $IMAGE_NAME
```

### Install the Operator Package
```bash
cd $DA_WORKING_DIR/deploy/operator
helm install -n operator . -f values.yaml --namespace=operator
```
Check for the status of the pods in operator namespace. Check if Prometheus operator pods are in Ready state.
```bash
kubectl get pods -n operator
NAME                                                      READY   STATUS    RESTARTS
m3db-operator-0                                           1/1     Running   0
op-etcd-operator-etcd-backup-operator-6cdc577f7d-ltgsr    1/1     Running   0
op-etcd-operator-etcd-operator-79fd99f8b7-fdc7p           1/1     Running   0
op-etcd-operator-etcd-restore-operator-855f7478bf-r7qxp   1/1     Running   0
op-prometheus-operator-operator-5c9b87965b-wjtw5          1/1     Running   1
op-sparkoperator-6cb4db884c-75rcd                         1/1     Running   0
strimzi-cluster-operator-5bffdd7b85-rlrvj                 1/1     Running   0
```
#### Troubleshooting Operator installation
Sometimes deleting the previously installed Operator package will fail to remove all operator pods. To troubleshoot this ensure these following steps.

1. Make sure that all the other deployments or helm release is deleted (purged). Operator package is a baseline package for the applications, so if the applications are still running while trying to delete the operator package might result in unwarrented state. 

2. Delete all the resources and CRDs associated with operator package.
```bash
#NOTE: Use the same release name and namespace as in installation of operator package in the previous section
cd $DA_WORKING_DIR/operator
helm template -n operator . -f values.yaml --namespace=operator > ../delete_operator.yaml
cd ../
kubectl delete -f delete_operator.yaml
```
## Install Collection package
Note: Collectd.conf is avaliable in $DA_WORKING_DIR/collection/charts/collectd/resources/config directory. Any valid collectd.conf can be placed here.
```bash
Default (For custom collectd skip this section)
=======
cd $DA_WORKING_DIR/deploy/collection
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
   $DA_WORKING_DIR/collection/charts/collectd/resources

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
#### Configure Collectd Plugins
1. Using the sample [collectdglobal.yaml](microservices/collectd-operator/examples/collectd/collectdglobal.yaml), Configure the CollectdGlobal CR
2. If there are additional Types.db files to update, Copy the additional types.db files to resources folder. 
3. Create a ConfigMap to load the types.db and update the configMap with name of the ConfigMap created.
4. Create and configure the required CollectdPlugin CRs. Use these samples as a reference [cpu_collectdplugin_cr.yaml](microservices/collectd-operator/examples/collectd/cpu_collectdplugin_cr.yaml), [prometheus_collectdplugin_cr.yaml](microservices/collectd-operator/examples/collectd/prometheus_collectdplugin_cr.yaml).
4. Use the same namespace where the collection package was installed.
5. Assuming it is edge1, create the config resources that are applicable. Apply the following commands in the same order.
```yaml
# Note: 
## 1. Create Configmap is optional and required only if additional types.db file needs to be mounted.
## 2. Add/Remove --from-file accordingly. Use the correct file name based on the context.
kubectl create configmap typesdb-configmap --from-file ./resource/[FILE_NAME1] --from-file ./resource/[FILE_NAME2]
kubectl create -f edge1 collectdglobal.yaml
kubectl create -f edge1 [PLUGIN_NAME1]_collectdplugin_cr.yaml
kubectl create -f edge1 [PLUGIN_NAME2]_collectdplugin_cr.yaml
kubectl create -f edge1 [PLUGIN_NAME3]_collectdplugin_cr.yaml
...
```

#Install visualization package
```bash
Default (For custom Grafana dashboards skip this section)
=======
cd $DA_WORKING_DIR/visualization
helm install -n viz . -f values.yaml -f grafana-values.yaml

Custom Grafana dashboards
=========================
1. Place the custom dashboard definition into the folder $DA_WORKING_DIR/visualization/charts/grafana/dashboards
    Example dashboard definition can be found at $DA_WORKING_DIR/visualization/charts/grafana/dashboards/dashboard1.json
2. Create a configmap.yaml that imports above created dashboard.json file as config and copy that configmap.yaml to $DA_WORKING_DIR/visualization/charts/grafana/templates/
    Example configmap can be found at $DA_WORKING_DIR/visualization/charts/grafana/templates/configmap-add-dashboard.yaml
3. Add custom dashboard configuration to values.yaml or an overriding values.yaml. 
    Example configuration can be found in the "dashboardProviders" section of grafana-values.yaml

4. cd $DA_WORKING_DIR/visualization
5. For a fresh install of visualization package, do "helm install"
    e.g., helm install -n viz . -f values.yaml -f grafana-values.yaml
   If the custom dashboard is being added to an already running Grafana, do "helm upgrade"
    e.g., helm upgrade -n viz . -f values.yaml -f grafana-values.yaml -f ......
```

#### Verify Visualization package
Check if the visualization pod is up
```
$ kubectl get pods
    NAME                          READY   STATUS    RESTARTS   AGE
    viz-grafana-78dcffd75-sxnjv   1/1     Running   0          52m
```

### Login to Grafana
```
1. Get your 'admin' user password by running:
    kubectl get secret --namespace default viz-grafana -o jsonpath="{.data.admin-password}" | base64 --decode ; echo

2. Get the Grafana URL to visit by running these commands in the same shell:
    export POD_NAME=$(kubectl get pods --namespace default -l "app=grafana,release=viz" -o jsonpath="{.items[0].metadata.name}")
    kubectl --namespace default port-forward $POD_NAME 3000

3. Visit the URL : http://localhost:3000 and login with the password from step 1 and the username: admin
```

#### Configure Grafana Datasources
Using the sample [prometheus_grafanadatasource_cr.yaml](microservices/visualization-operator/examples/grafana/prometheus_grafanadatasource_cr.yaml), Configure the GrafanaDataSource CR by running the command below
```yaml
kubectl create -f [DATASOURCE_NAME1]_grafanadatasource_cr.yaml
kubectl create -f [DATASOURCE_NAME2]_grafanadatasource_cr.yaml
...
```

## Install Minio Model repository
* Prerequisite: Dynamic storage provisioner needs to be enabled. Either rook-ceph ($DA_WORKING_DIR/00-init) or another alternate provisioner needs to be enabled.
```bash
cd $DA_WORKING_DIR/minio

Edit the values.yaml to set the credentials to access the minio UI.
Default values are
accessKey: "onapdaas"
secretKey: "onapsecretdaas"

helm install -n minio . -f values.yaml --namespace=edge1
```

## Install Messaging platform

We have currently support strimzi based kafka operator.
Navigate to ```$DA_WORKING_DIR/deploy/messaging/charts/strimzi-kafka-operator``` directory.
Use the below command :
```
helm install . -f values.yaml  --name sko --namespace=test
```

NOTE: Make changes in the values.yaml if required.

Once the strimzi operator ready, you shall get a pod like :

```
strimzi-cluster-operator-5cf7648b8c-zgxv7       1/1     Running   0          53m
```

Once this done, install the kafka package like any other helm charts you have.
Navigate to dir : ```$DA_WORKING_DIRdeploy/messaging``` and use command:
```
helm install --name kafka-cluster charts/kafka/
```

Once this done, you should have the following pods up and running.

```
kafka-cluster-entity-operator-b6557fc6c-hlnkm   3/3     Running   0          47m
kafka-cluster-kafka-0                           2/2     Running   0          48m
kafka-cluster-kafka-1                           2/2     Running   0          48m
kafka-cluster-kafka-2                           2/2     Running   0          48m
kafka-cluster-zookeeper-0                       2/2     Running   0          49m
kafka-cluster-zookeeper-1                       2/2     Running   0          49m
kafka-cluster-zookeeper-2                       2/2     Running   0          49m
```

You should have the following services when do a ```kubectl get svc```

```
kafka-cluster-kafka-bootstrap    ClusterIP   10.XX.YY.ZZ   <none>        9091/TCP,9092/TCP,9093/TCP   53m
kafka-cluster-kafka-brokers      ClusterIP   None           <none>        9091/TCP,9092/TCP,9093/TCP   53m
kafka-cluster-zookeeper-client   ClusterIP   10.XX.YY.ZZ   <none>        2181/TCP                     55m
kafka-cluster-zookeeper-nodes    ClusterIP   None           <none>        2181/TCP,2888/TCP,3888/TCP   55m
```
#### Testing messaging 

You can test your kafka brokers by creating a simple producer and consumer.

Producer : 
```
kubectl run kafka-producer -ti --image=strimzi/kafka:0.12.2-kafka-2.2.1 --rm=true --restart=Never -- bin/kafka-console-producer.sh --broker-list kafka-cluster-kafka-bootstrap:9092 --topic my-topic
 ```
 Consumer :
 ```

kubectl run kafka-consumer -ti --image=strimzi/kafka:0.12.2-kafka-2.2.1 --rm=true --restart=Never -- bin/kafka-console-consumer.sh --bootstrap-server kafka-cluster-kafka-bootstrap:9092 --topic my-topic --from-beginning
```

## Install Training Package

#### Install M3DB (Time series Data lake)
##### Pre-requisites
1.  kubernetes cluster with atleast 3 nodes
2.  Etcd operator, M3DB operator
3.  Node labelled with zone and region.

```bash
## Defult region is us-west1, Default labels are us-west1-a, us-west1-b, us-west1-c
## If this is changed then isolationGroups in training-core/charts/m3db/values.yaml needs to be updated.
NODES=($(kubectl get nodes --output=jsonpath={.items..metadata.name}))

kubectl label node/${NODES[0]} failure-domain.beta.kubernetes.io/region=us-west1
kubectl label node/${NODES[1]} failure-domain.beta.kubernetes.io/region=us-west1
kubectl label node/${NODES[2]} failure-domain.beta.kubernetes.io/region=us-west1

kubectl label node/${NODES[0]} failure-domain.beta.kubernetes.io/zone=us-west1-a --overwrite=true
kubectl label node/${NODES[1]} failure-domain.beta.kubernetes.io/zone=us-west1-b --overwrite=true
kubectl label node/${NODES[2]} failure-domain.beta.kubernetes.io/zone=us-west1-c --overwrite=true
```
```bash
cd $DA_WORKING_DIR/training-core/charts/m3db
helm install -n m3db . -f values.yaml --namespace training
```
```
$ kubectl get pods -n training
NAME                   READY   STATUS    RESTARTS   AGE
m3db-cluster-rep0-0    1/1     Running   0          103s
m3db-cluster-rep1-0    1/1     Running   0          83s
m3db-cluster-rep1-0    1/1     Running   0          62s
m3db-etcd-sjhgl4xfgc   1/1     Running   0          83s
m3db-etcd-lfs96hngz6   1/1     Running   0          67s
m3db-etcd-rmgdkkx4bq   1/1     Running   0          51s
```

##### Configure remote write from Prometheus to M3DB
```bash
cd $DA_WORKING_DIR/day2_configs/prometheus/
```
```yaml
cat << EOF > add_m3db_remote.yaml
spec:
  remoteWrite:
  - url: "http://m3coordinator-m3db.training.svc.cluster.local:7201/api/v1/prom/remote/write"
    writeRelabelConfigs:
      - targetLabel: metrics_storage
        replacement: m3db_remote
EOF
```
```bash
kubectl patch --namespace=edge1 prometheus cp-prometheus-prometheus -p "$(cat add_m3db_remote.yaml)" --type=merge
```
Verify the prometheus GUI to see if the m3db remote write is enabled.
