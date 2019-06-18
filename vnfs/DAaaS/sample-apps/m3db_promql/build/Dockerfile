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


FROM ubuntu:18.04

RUN apt update -yqq 

# Install all the essentials
RUN apt-get update --fix-missing && \
    apt-get install -y --no-install-recommends wget curl ca-certificates libglib2.0-0 libxext6 libsm6 libxrender1 \
                       git build-essential openssh-server openssh-client && \
    mkdir -p /var/run/sshd && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

ENV PATH /opt/conda/bin:$PATH

# Install miniconda
RUN wget --quiet https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda.sh && \
    /bin/bash ~/miniconda.sh -b -p /opt/conda && \
    rm ~/miniconda.sh && \
    ln -s /opt/conda/etc/profile.d/conda.sh /etc/profile.d/conda.sh && \
    echo ". /opt/conda/etc/profile.d/conda.sh" >> ~/.bashrc && \
    echo "conda activate base" >> ~/.bashrc

# Install python libraries using conda in the virtual_environment:ml_env
SHELL ["/bin/bash", "-c"]
RUN conda update -n base -c defaults conda && \
    conda create -n ml_env
RUN conda install -n ml_env -y -c anaconda pip 
RUN pwd
RUN conda install -n ml_env -y -c anaconda h5py

RUN conda install -n ml_env -y -c pytorch pytorch-cpu
RUN conda install -n ml_env -y -c conda-forge nlopt

RUN echo "conda activate ml_env" >> ~/.bashrc
RUN source ~/.bashrc
RUN /opt/conda/envs/ml_env/bin/pip install --no-cache-dir minio

# Install tini
RUN apt-get install -y --no-install-recommends curl grep sed dpkg && \
    TINI_VERSION=`curl https://github.com/krallin/tini/releases/latest | grep -o "/v.*\"" | sed 's:^..\(.*\).$:\1:'` && echo ${TINI_VERSION} && \
    curl -L "https://github.com/krallin/tini/releases/download/v${TINI_VERSION}/tini_${TINI_VERSION}.deb" > tini.deb && \
    dpkg -i tini.deb && \
    rm tini.deb && \
    apt clean

# This is needed to match the original entrypoint.sh file.
RUN cp /usr/bin/tini /sbin
RUN echo "export PATH=/opt/conda/envs/ml_env/bin:$PATH" >> ~/.bashrc

COPY . /app
WORKDIR /app

RUN mkdir promql_api
RUN wget https://raw.githubusercontent.com/onap/demo/master/vnfs/DAaaS/lib/promql_api/prom_ql_api.py
RUN mv prom_ql_api.py ./promql_api
RUN touch ./promql_api/__init__.py
RUN source ~/.bashrc
RUN conda install -n ml_env -y --file requirements.txt

ENTRYPOINT [ "/app/entrypoint.sh" ]
