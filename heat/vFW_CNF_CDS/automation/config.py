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

class Config:
    #### REGION DETAILS ####
    NATIVE = False
    COMPLEX_ID = "complex"
    CLOUD_OWNER = "k8sCloudOwner"
    CLOUD_REGION = "k8s-region-1"
    AVAILABILITY_ZONE_NAME = "k8s-availability-zone"
    HYPERVISOR_TYPE = "k8s"
    TENANT_NAME = "k8s-tenant-1"
    K8S_NAMESPACE = "vfirewall"
    CUSTOMER_RESOURCE_DEFINITIONS = []
# Uncomment, if you want to run on non KUD k8s cluster
#    CUSTOMER_RESOURCE_DEFINITIONS = ["crds/crd1",
#                                     "crds/crd2"]

    CLUSTER_KUBECONFIG_PATH = "artifacts/cluster_kubeconfig"
    ONAP_KUBECONFIG_PATH = "artifacts/onap_kubeconfig"

    #### SERVICE DETAILS ####
    GLOBAL_CUSTOMER_ID = "customer_cnf"
    VSPFILE = "vsp/vfw_k8s_demo.zip"
    if NATIVE:
        VSPFILE = "vsp/native_vfw_k8s_demo.zip"
    VENDOR = "vendor_cnf"
    SERVICENAME = "vfw_k8s_demo_CNF"
    VSPNAME = "VSP_" + SERVICENAME
    VFNAME = "VF_" + SERVICENAME
    SERVICE_INSTANCE_NAME = "INSTANCE_" + SERVICENAME
    SDNC_ARTIFACT_NAME = "vnf"
    VF_MODULE_PREFIX = ""
    if NATIVE:
        VF_MODULE_PREFIX = "helm_"

    VF_MODULE_LIST = {VF_MODULE_PREFIX + "base_template":
                          {"name": VF_MODULE_PREFIX + "base_template",
                           "k8s-rb-profile-name": "vfw-cnf-cds-base-profile",
                           "k8s-rb-profile-namespace": K8S_NAMESPACE},
                      VF_MODULE_PREFIX + "vfw":
                          {"name": VF_MODULE_PREFIX + "vfw",
                           "k8s-rb-profile-name": "vfw-cnf-cds-base-profile",
                           "k8s-rb-profile-namespace": K8S_NAMESPACE},
                      VF_MODULE_PREFIX + "vpkg":
                          {"name": VF_MODULE_PREFIX + "vpkg",
                           "k8s-rb-profile-name": "vfw-cnf-cds-base-profile",
                           "k8s-rb-profile-namespace": K8S_NAMESPACE},
                      VF_MODULE_PREFIX + "vsn":
                          {"name": VF_MODULE_PREFIX + "vsn",
                           "k8s-rb-profile-name": "vfw-cnf-cds-base-profile",
                           "k8s-rb-profile-namespace": K8S_NAMESPACE}}

    ######## DEFAULT VALUES ########
    OWNING_ENTITY = "OE-Demonstration"
    PROJECT = "Project-Demonstration"
    PLATFORM = "test"
    LINE_OF_BUSINESS = "LOB-Demonstration"

