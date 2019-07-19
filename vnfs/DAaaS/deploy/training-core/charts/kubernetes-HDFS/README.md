---
layout: global
title: HDFS on Kubernetes
---
# HDFS on Kubernetes
Repository holding helm charts for running Hadoop Distributed File System (HDFS)
on Kubernetes.

See [charts/README.md](charts/README.md) for how to run the charts.

See [tests/README.md](tests/README.md) for how to run integration tests for
HDFS on Kubernetes.


# Troubleshooting

In case some pods are in pending state, check by using kubectl describe command.
If describe shows :
```
  Type     Reason            Age                From               Message
  ----     ------            ----               ----               -------
  Warning  FailedScheduling  7s (x20 over 66s)  default-scheduler  pod has unbound immediate PersistentVolumeClaims (repeated 3 times)
```

Then make sure you have the storage provisioner up and running.
In our case, its rook that we support.
So, rook should be up and be the default storage proviosner.

```
NAME                        PROVISIONER          AGE
rook-ceph-block (default)   ceph.rook.io/block   132m
```

Delete all the previous unbound PVCs like below :
```
NAME                           STATUS    VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS   AGE
data-hdfs1-zookeeper-0         Pending                                                     108m
editdir-hdfs1-journalnode-0    Pending                                                     108m
metadatadir-hdfs1-namenode-0   Pending                                                     108m
```

```
kubectl delete pvc/data-hdfs1-zookeeper-0
kubectl delete pvc/editdir-hdfs1-journalnode-0
kubectl delete pvc/metadatadir-hdfs1-namenode-0 
```

#### If the dataNode restarts with the error:
```
19/07/19 21:22:55 FATAL datanode.DataNode: Initialization failed for Block pool <registering> (Datanode Uuid unassigned) service to hdfs1-namenode-1.hdfs1-namenode.hdfs1.svc.cluster.local/XXX.YY.ZZ.KK:8020. Exiting. 
java.io.IOException: All specified directories are failed to load.
        at org.apache.hadoop.hdfs.server.datanode.DataStorage.recoverTransitionRead(DataStorage.java:478)
        at org.apache.hadoop.hdfs.server.datanode.DataNode.initStorage(DataNode.java:1358)
        at org.apache.hadoop.hdfs.server.datanode.DataNode.initBlockPool(DataNode.java:1323)
        at org.apache.hadoop.hdfs.server.datanode.BPOfferService.verifyAndSetNamespaceInfo(BPOfferService.java:317)
        at org.apache.hadoop.hdfs.server.datanode.BPServiceActor.connectToNNAndHandshake(BPServiceActor.java:223)
        at org.apache.hadoop.hdfs.server.datanode.BPServiceActor.run(BPServiceActor.java:802)
```

* SOLUTION:  Make sure that whatever host path you set for the dataNode is deleted and doesnt exist before you run the hdfs helm chart.
```
    - name: hdfs-data-0
      hostPath:
        path: /hdfs-data
```
In case you are reinstalling the HDFS, delete the host path : /hdfs-data 
before you proceed or else the above error shall come.