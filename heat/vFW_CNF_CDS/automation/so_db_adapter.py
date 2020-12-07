# ============LICENSE_START=======================================================
# Copyright (C) 2020 Orange
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

import base64
import os

from kubernetes import config, client
from kubernetes.stream import stream


class SoDBAdapter:

    def __init__(self, cloud_region_id, complex_id, onap_kubeconfig_path):
        self.CLOUD_REGION_ID = cloud_region_id
        self.COMPLEX_ID = complex_id
        self.ONAP_KUBECONFIG_PATH = onap_kubeconfig_path
        self.MYPATH = os.path.dirname(os.path.realpath(__file__))

        config.load_kube_config(config_file=os.path.join(self.MYPATH, self.ONAP_KUBECONFIG_PATH))
        self.api_instance = client.CoreV1Api()
        self.pod_name = self.get_mariadb_pod_name()
        self.password = self.get_mariadb_root_username_password()

    def get_mariadb_pod_name(self):
        pods = self.api_instance.list_namespaced_pod(namespace="onap")
        for pod in pods.items:
            if pod.metadata.name.find("mariadb-galera-0") != -1:
                return pod.metadata.name

    def get_mariadb_root_username_password(self):
        secrets = self.api_instance.list_namespaced_secret(namespace="onap")
        for secret in secrets.items:
            if secret.metadata.name.find("mariadb-galera-db-root-password") != -1:
                base64_password = secret.data["password"]
                base64_bytes = base64_password.encode('ascii')
                password_bytes = base64.b64decode(base64_bytes)

                return password_bytes.decode('ascii')

    def run_exec_request(self, exec_command):
        response = stream(self.api_instance.connect_get_namespaced_pod_exec,
                          name=self.pod_name,
                          # container="container-name",
                          namespace="onap",
                          command=exec_command,
                          stdin=False,
                          tty=False,
                          stderr=True,
                          stdout=True)
        return response

    def check_region_in_db(self):
        exec_command = [
            "/bin/sh",
            "-c",
            f"mysql -uroot -p{self.password} catalogdb -e 'SELECT * FROM cloud_sites;'"]
        response = self.run_exec_request(exec_command)

        is_region_found = False
        for line in response.split("\n"):
            if line.split("\t")[0] == self.CLOUD_REGION_ID:
                print(line)
                is_region_found = True
                return is_region_found
        return is_region_found

    def add_region_to_so_db(self):
        exec_command = [
            "/bin/sh",
            "-c",
            f"mysql -uroot -p{self.password} catalogdb -e "
            f"'insert into cloud_sites(ID, REGION_ID, IDENTITY_SERVICE_ID, CLOUD_VERSION, CLLI, ORCHESTRATOR ) "
            f"values (\"{self.CLOUD_REGION_ID}\", \"{self.CLOUD_REGION_ID}\", \"DEFAULT_KEYSTONE\", \"2.5\", "
            f"\"{self.COMPLEX_ID}\", \"multicloud\");'"]

        response = self.run_exec_request(exec_command)

        return response
