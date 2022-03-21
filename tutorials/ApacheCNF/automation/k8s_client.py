# ============LICENSE_START=======================================================
# Copyright (C) 2021 Orange
# ================================================================================
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ============LICENSE_END=========================================================

import os
from pprint import pprint

import oyaml as yaml
from kubernetes import config, client
from kubernetes.client import OpenApiException


class K8sClient:
    def __init__(self, kubeconfig_path):
        self.mypath = os.path.dirname(os.path.realpath(__file__))
        config.load_kube_config(config_file=os.path.join(self.mypath, kubeconfig_path))
        self.api_instance = client.CustomObjectsApi()

    def read_custom_object_file(self, file_path):
        with open(file_path) as crd_file:
            crd_body = yaml.load(crd_file, Loader=yaml.FullLoader)
        return crd_body

    def get_custom_object_details(self, crd_body):
        group = crd_body["apiVersion"].split("/")[0]
        version = crd_body["apiVersion"].split("/")[1]
        plural = crd_body["kind"].lower() + "s"
        #name = crd_body["metadata"]["name"]

        return group, version, plural #, name

    def create_custom_object(self, file_path):
        crd_body = self.read_custom_object_file(file_path)
        #group, version, plural, name = self.get_custom_object_details(crd_body)
        group, version, plural = self.get_custom_object_details(crd_body)
        api_response = None
        try:
            api_response = self.api_instance.create_cluster_custom_object(group=group,
                                                                          version=version,
                                                                          plural=plural,
                                                                          body=crd_body,
                                                                          pretty="true")
        except OpenApiException as error:
            print(str(error.status) + " " + error.reason)
            pprint(error.body)
        return api_response
