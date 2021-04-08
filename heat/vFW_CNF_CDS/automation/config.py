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

class Config:
    SCENARIO = 2
    # 1 - default configuration values like set below
    # 2 - extra ssh service that comes from the profile
    # 3 - extra ssh service that comes from config + verification of the CNF status

    #### REGION DETAILS ####
    COMPLEX_ID = "complex"
    CLOUD_OWNER = "k8sCloudOwner"
    CLOUD_REGION = "kud-1"
    AVAILABILITY_ZONE_NAME = "k8s-availability-zone"
    HYPERVISOR_TYPE = "k8s"
    TENANT_NAME = "kud-1"
    K8S_NAMESPACE = "vfirewall"
    CUSTOMER_RESOURCE_DEFINITIONS = []
# Uncomment, if you want to run on non KUD k8s cluster
#    CUSTOMER_RESOURCE_DEFINITIONS = ["crds/crd1",
#                                     "crds/crd2"]

    CLUSTER_KUBECONFIG_PATH = "artifacts/cluster_kubeconfig"

    #### SERVICE DETAILS ####
    NATIVE = True
    SKIP_POST_INSTANTIATION = True
    GLOBAL_CUSTOMER_ID = "customer_cnf"
    VSPFILE = "vsp/vfw_k8s_demo.zip"
    if NATIVE:
        VSPFILE = "vsp/native_vfw_k8s_demo.zip"

    PROFILE_NAME = "vfw-cnf-cds-base-profile"
    PROFILE_SOURCE = PROFILE_NAME
    RELEASE_NAME = "vfw-1"

    VENDOR = "vendor_cnf"
    SERVICENAME = "vfw_k8s_demo_CNF"
    VSPNAME = "VSP_" + SERVICENAME
    VFNAME = "VF_" + SERVICENAME
    SERVICE_INSTANCE_NAME = "INSTANCE_" + SERVICENAME + "_" + str(SCENARIO)
    SDNC_ARTIFACT_NAME = "vnf"

    # INSERT PARAMS FOR VNF HERE AS "name" : "value" PAIR
    VNF_PARAM_LIST = {
        "k8s-rb-profile-namespace": K8S_NAMESPACE
    }

    VF_MODULE_PREFIX = ""
    if NATIVE:
        VF_MODULE_PREFIX = "helm_"

    VF_MODULE_PARAM_LIST = {
        VF_MODULE_PREFIX + "base_template": {
            "k8s-rb-profile-name": PROFILE_NAME,
            "k8s-rb-profile-source": PROFILE_SOURCE,
            "k8s-rb-instance-release-name": RELEASE_NAME + "-base"
        },
        VF_MODULE_PREFIX + "vfw": {
            "k8s-rb-profile-name": PROFILE_NAME,
            "k8s-rb-profile-source": PROFILE_SOURCE,
            "k8s-rb-instance-release-name": RELEASE_NAME + "-vfw"
        },
        VF_MODULE_PREFIX + "vpkg": {
            "k8s-rb-profile-name": PROFILE_NAME,
            "k8s-rb-profile-source": PROFILE_SOURCE,
            "k8s-rb-instance-release-name": RELEASE_NAME + "-vpkg"
        },
        VF_MODULE_PREFIX + "vsn": {
            "k8s-rb-profile-name": PROFILE_NAME,
            "k8s-rb-profile-source": PROFILE_SOURCE,
            "k8s-rb-instance-release-name": RELEASE_NAME + "-vsn"
        }
    }

    ######## DEFAULT VALUES ########
    OWNING_ENTITY = "OE-Demonstration"
    PROJECT = "Project-Demonstration"
    PLATFORM = "test"
    LINE_OF_BUSINESS = "LOB-Demonstration"

    ######## SCENARIOS #############

    ########     1    #############
    if SCENARIO == 1:
        SKIP_POST_INSTANTIATION = True
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["k8s-rb-profile-name"] = PROFILE_NAME
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["k8s-rb-profile-source"] = PROFILE_SOURCE
    ########     2    #############
    elif SCENARIO == 2:
        SKIP_POST_INSTANTIATION = True
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["k8s-rb-profile-name"] = "vfw-cnf-cds-vpkg-profile"
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["k8s-rb-profile-source"] = "vfw-cnf-cds-vpkg-profile"
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["vpg-management-port"] = "31922"
    ########     3    #############
    elif SCENARIO == 3:
        SKIP_POST_INSTANTIATION = False
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["k8s-rb-profile-name"] = PROFILE_NAME
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["k8s-rb-profile-source"] = PROFILE_SOURCE
    else:
        raise Exception("Not Implemented Scenario")
