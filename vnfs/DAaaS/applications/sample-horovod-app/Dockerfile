# Copyright (c) 2019 Intel Corporation
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Ported kubernetes spark image to Ubuntu

FROM ubuntu:18.04

# Install jdk
RUN apt update -yqq 
RUN apt install -y locales openjdk-8-jdk && rm -rf /var/lib/apt/lists/* \
    && localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8

# Install all the essentials
RUN apt-get update --fix-missing && \
    apt-get install -y numactl wget curl bzip2 nmap vim ca-certificates libglib2.0-0 libxext6 libsm6 libxrender1 \
                       git mercurial subversion build-essential openssh-server openssh-client net-tools && \
    mkdir -p /var/run/sshd && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

ENV LANG en_US.utf8
ENV JAVA_HOME /usr/lib/jvm/java-8-openjdk-amd64
ENV PATH $JAVA_HOME/bin:$PATH
ENV PATH /opt/conda/bin:/opt/spark/bin:$PATH
ENV OPENMPI_VERSION 3.1

# Install openMPI
RUN mkdir /tmp/openmpi && \
    cd /tmp/openmpi && \
    wget --quiet https://www.open-mpi.org/software/ompi/v${OPENMPI_VERSION}/downloads/openmpi-${OPENMPI_VERSION}.2.tar.gz -O openmpi.tar.gz && \
    tar zxf openmpi.tar.gz && \
    cd openmpi-3.1.2 && \
    ./configure --enable-orterun-prefix-by-default && \
    make -j $(nproc) all && \
    make install && \
    ldconfig && \
    rm -rf /tmp/openmpi

# Install miniconda
RUN wget --quiet https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda.sh && \
    /bin/bash ~/miniconda.sh -b -p /opt/conda && \
    rm ~/miniconda.sh && \
    ln -s /opt/conda/etc/profile.d/conda.sh /etc/profile.d/conda.sh && \
    echo ". /opt/conda/etc/profile.d/conda.sh" >> ~/.bashrc && \
    echo "conda activate base" >> ~/.bashrc

# Install tf & keras using conda in the virtual_environment:tf_env
SHELL ["/bin/bash", "-c"]
RUN conda update -n base -c defaults conda && \
    conda create -n tf_env
RUN conda install -n tf_env -y -c anaconda \
    pip tensorflow keras nltk pyarrow
RUN conda install -n tf_env -y -c anaconda h5py

RUN conda install -n tf_env -y -c pytorch pytorch-cpu
RUN conda install -n tf_env -y -c conda-forge matplotlib

RUN echo "conda activate tf_env" >> ~/.bashrc && \
    conda install -n tf_env -y -c conda-forge clangdev

RUN source ~/.bashrc
RUN HOROVOD_WITH_TENSORFLOW=1 /opt/conda/envs/tf_env/bin/pip install --no-cache-dir horovod

# openMPI sane defaults:
RUN echo "hwloc_base_binding_policy = none" >> /usr/local/etc/openmpi-mca-params.conf && \
    echo "rmaps_base_mapping_policy = slot" >> /usr/local/etc/openmpi-mca-params.conf && \
    echo "btl_tcp_if_exclude = lo,docker0" >> /usr/local/etc/openmpi-mca-params.conf

# Allow OpenSSH to talk to containers without asking for confirmation
RUN cat /etc/ssh/ssh_config | grep -v StrictHostKeyChecking > /etc/ssh/ssh_config.new && \
    echo "    StrictHostKeyChecking no" >> /etc/ssh/ssh_config.new && \
    mv /etc/ssh/ssh_config.new /etc/ssh/ssh_config

# Install tini
RUN apt-get install -y curl grep sed dpkg && \
    TINI_VERSION=`curl https://github.com/krallin/tini/releases/latest | grep -o "/v.*\"" | sed 's:^..\(.*\).$:\1:'` && echo ${TINI_VERSION} && \
    curl -L "https://github.com/krallin/tini/releases/download/v${TINI_VERSION}/tini_${TINI_VERSION}.deb" > tini.deb && \
    dpkg -i tini.deb && \
    rm tini.deb && \
    apt clean

# This is needed to match the original entrypoint.sh file.
RUN cp /usr/bin/tini /sbin

# Begin: Installing spark
ARG spark_jars=jars
ARG img_path=kubernetes/dockerfiles
ARG k8s_tests=kubernetes/tests

# Before building the docker image, first build and make a Spark distribution following
# the instructions in http://spark.apache.org/docs/latest/building-spark.html.
# If this docker file is being used in the context of building your images from a Spark
# distribution, the docker build command should be invoked from the top level directory
# of the Spark distribution. E.g.:
# docker build -t spark:latest -f kubernetes/dockerfiles/spark/ubuntu18.04/Dockerfile .

RUN mkdir -p /opt/spark && \
    mkdir -p /opt/spark/work-dir && \
    touch /opt/spark/RELEASE && \
    rm /bin/sh && \
    ln -sv /bin/bash /bin/sh && \
    echo "auth required pam_wheel.so use_uid" >> /etc/pam.d/su && \
    chgrp root /etc/passwd && chmod ug+rw /etc/passwd


COPY ${spark_jars} /opt/spark/jars
COPY bin /opt/spark/bin
COPY sbin /opt/spark/sbin
COPY ${img_path}/spark/entrypoint.sh /opt/

COPY ${k8s_tests} /opt/spark/tests
COPY data /opt/spark/data
ENV SPARK_HOME /opt/spark

RUN mkdir /opt/spark/python
COPY python/pyspark /opt/spark/python/pyspark
COPY python/lib /opt/spark/python/lib
ENV PYTHONPATH /opt/spark/python/lib/pyspark.zip:/opt/spark/python/lib/py4j-*.zip
ENV PATH /opt/conda/envs/tf_env/bin:$PATH

RUN echo "export PATH=/opt/conda/envs/tf_env/bin:$PATH" >> ~/.bashrc
#    echo "activate tf_env\n" >> ~/.bashrc
RUN pip install petastorm
COPY examples /opt/spark/examples
WORKDIR /opt/spark/work-dir

ENTRYPOINT [ "/opt/entrypoint.sh" ]

# End: Installing spark
