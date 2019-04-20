# Horovod

[Horovod](https://eng.uber.com/horovod/) is a distributed training framework for TensorFlow, and it's provided by UBER. The goal of Horovod is to make distributed Deep Learning fast and easy to use. And it provides [Horovod in Docker](https://github.com/uber/horovod/blob/master/docs/docker.md) to streamline the installation process.

## Introduction

This chart bootstraps Horovod which is a Distributed TensorFlow Framework on a Kubernetes cluster using the Helm Package Manager. It deploys Horovod workers as statefulsets, and the Horovod master as a job, then discover the host list automatically.

## Prerequisites

- Kubernetes cluster v1.8+

## Build Docker Image

You can use the dockerfile image provided along with this package. The benefit of this dockerfile is it contains many additional packages that the data science engineers usually require like spark, tensorflow, pytorch, matplotlib, nltk, 
keras, h5py, pyarrow. 

Before building the docker image, first build and make a Spark distribution following the instructions in http://spark.apache.org/docs/latest/building-spark.html
If this docker file is being used in the context of building your images from a Spark distribution, the docker build command should be invoked from the top level directory of the Spark distribution. E.g.:

```
docker build -t spark:latest -f kubernetes/dockerfiles/spark/ubuntu18.04/Dockerfile .
```

Once you build the spark image, go inside the spark package and place the file "keras_mnist_advanced_modified.py" in the dirctory: examples/src/main/python/tensorflow/. Create the 'tensorflow' directory if it doesnt exists.
We do this because we the file keras_mnist_advanced_modified.py is optimized for CPU running and we want this file to be automatically present in the final docker image that we build.

```
docker build -t spark-tf-keras-horovod-pytorch:latest -f kubernetes/dockerfiles/spark/ubuntu18.04/Dockerfile .
```

## Prepare ssh keys

```
# Setup ssh key
export SSH_KEY_DIR=`mktemp -d`
cd $SSH_KEY_DIR
yes | ssh-keygen -N "" -f id_rsa
```

## Create the values.yaml

To run Horovod with GPU, you can create `values.yaml` like below

```
# cat << EOF > ~/values.yaml
---
ssh:
  useSecrets: true
  hostKey: |-
$(cat $SSH_KEY_DIR/id_rsa | sed 's/^/    /g')

  hostKeyPub: |-
$(cat $SSH_KEY_DIR/id_rsa.pub | sed 's/^/    /g')

worker:
  number: 2
  image:
    repository: uber/horovod
    tag: 0.12.1-tf1.8.0-py3.5
master:
  image:
    repository: uber/horovod
    tag: 0.12.1-tf1.8.0-py3.5
  args:
    - "mpirun -np 3 --hostfile /horovod/generated/hostfile --mca orte_keep_fqdn_hostnames t --allow-run-as-root --display-map --tag-output --timestamp-output sh -c '/opt/conda/envs/tf_env/bin/python /opt/spark/examples/src/main/python/tensorflow/keras_mnist_advanced_modified.py'"
EOF
```

For most cases, the overlay network impacts the Horovod performance greatly, so we should apply `Host Network` solution. To run Horovod with Host Network and GPU, you can create `values.yaml` like below


```
# cat << EOF > ~/values.yaml
---
useHostNetwork: true

ssh:
  useSecrets: true
  port: 32222
  hostKey: |-
$(cat $SSH_KEY_DIR/id_rsa | sed 's/^/    /g')

  hostKeyPub: |-
$(cat $SSH_KEY_DIR/id_rsa.pub | sed 's/^/    /g')


worker:
  number: 2
  image:
    repository: uber/horovod
    tag: 0.12.1-tf1.8.0-py3.5
master:
  image:
    repository: uber/horovod
    tag: 0.12.1-tf1.8.0-py3.5
  args:
    - "mpirun -np 3 --hostfile /horovod/generated/hostfile --mca orte_keep_fqdn_hostnames t --allow-run-as-root --display-map --tag-output --timestamp-output sh -c '/opt/conda/envs/tf_env/bin/python /opt/spark/examples/src/main/python/tensorflow/keras_mnist_advanced_modified.py'"
EOF
```

```
NOTE: A sample values.yaml is provided for reference. After adding the above changes, we should have a values.yml similar to that.
```

> notice: the difference is that you should set `useHostNetwork` as true, then set another ssh port rather than `22`

## Installing the Chart

To install the chart with the release name `mnist`:

```bash
$ helm install --values ~/values.yaml --name mnist stable/horovod
```

## Uninstalling the Chart

To uninstall/delete the `mnist` deployment:

```bash
$ helm delete mnist
```

The command removes all the Kubernetes components associated with the chart and
deletes the release.

## Upgrading an existing Release to a new major version
A major chart version change (like v1.2.3 -> v2.0.0) indicates that there is an
incompatible breaking change needing manual actions.

### 1.0.0
This version removes the `chart` label from the `spec.selector.matchLabels`
which is immutable since `StatefulSet apps/v1beta2`. It has been inadvertently
added, causing any subsequent upgrade to fail. See https://github.com/helm/charts/issues/7726.

In order to upgrade, delete the Horovod StatefulSet before upgrading, supposing your Release is named `my-release`:

```bash
$ kubectl delete statefulsets.apps --cascade=false my-release
```

## Configuration

The following table lists the configurable parameters of the Horovod
chart and their default values.

| Parameter | Description | Default |
|-----------|-------------|---------|
| `useHostNetwork`  | Host network    | `false` |
| `ssh.port` | The ssh port | `22` |
| `ssh.useSecrets` | Determine if using the secrets for ssh | `false` |
| `worker.number`|  The worker's number | `5` |
| `worker.image.repository` | horovod worker image | `uber/horovod` |
| `worker.image.pullPolicy` | `pullPolicy` for the worker | `IfNotPresent` |
| `worker.image.tag` | `tag` for the worker | `0.12.1-tf1.8.0-py3.5` |
| `resources`| pod resource requests & limits| `{}`|
| `worker.env` | worker's environment variables | `{}` |
| `master.image.repository` | horovod master image | `uber/horovod` |
| `master.image.tag` | `tag` for the master | `0.12.1-tf1.8.0-py3.5` |
| `master.image.pullPolicy` | image pullPolicy for the master image| `IfNotPresent` |
| `master.args` | master's args | `{}` |
| `master.env` | master's environment variables | `{}` |
