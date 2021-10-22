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
    SCENARIO = 1
    # 1 - default configuration values like set below
    # 2 - extra ssh service that comes from the profile
    # 3 - extra ssh service that comes from config + verification of the CNF status
    # change requires new onboarding

    K8S_NAMESPACE = "vfirewall"
    K8S_VERSION = "1.18.9"
    K8S_REGION = "kud"

    #### SERVICE DETAILS ####
    NATIVE = True # False for old dummy-heat based orchestration path
    SKIP_POST_INSTANTIATION = True
    MACRO_INSTANTIATION = True  # A-la-carte instantiation if False
    GLOBAL_CUSTOMER_ID = "customer_cnf"
    VSPFILE = "vsp/vfw_k8s_demo.zip"
    if NATIVE:
        VSPFILE = "vsp/native_vfw_k8s_demo.zip"

    PROFILE_NAME = "vfw-cnf-cds-base-profile"
    PROFILE_SOURCE = PROFILE_NAME
    RELEASE_NAME = "vfw-1"

    VENDOR = "vendor_cnf"
    SERVICENAME = "vfw_k8s_demo_CNF" + "_" + str(SCENARIO)
    VSPNAME = "VSP_" + SERVICENAME
    VFNAME = "VF_" + SERVICENAME
    SERVICE_INSTANCE_NAME = "INSTANCE_" + SERVICENAME
    SDNC_ARTIFACT_NAME = "vnf"

    # INSERT PARAMS FOR VNF HERE AS "name" : "value" PAIR
    VNF_PARAM_LIST = {
        "k8s-rb-profile-namespace": K8S_NAMESPACE,
        "k8s-rb-profile-k8s-version": K8S_VERSION
    }

    VF_MODULE_PREFIX = ""
    if NATIVE:
        VF_MODULE_PREFIX = "helm_"

    VF_MODULE_PARAM_LIST = {
        VF_MODULE_PREFIX + "base_template": {
            "instantiation_parameters": {
                "k8s-rb-profile-name": PROFILE_NAME,
                "k8s-rb-profile-source": PROFILE_SOURCE,
                "k8s-rb-instance-release-name": RELEASE_NAME + "-base",
                "k8s-rb-profile-namespace": K8S_NAMESPACE
            },
            "cloud_configuration": K8S_REGION
        },
        VF_MODULE_PREFIX + "vfw": {
            "instantiation_parameters": {
                "k8s-rb-profile-name": PROFILE_NAME,
                "k8s-rb-profile-source": PROFILE_SOURCE,
                "k8s-rb-instance-release-name": RELEASE_NAME + "-vfw",
                "k8s-rb-profile-namespace": K8S_NAMESPACE
            },
            "cloud_configuration": K8S_REGION
        },
        VF_MODULE_PREFIX + "vpkg": {
            "instantiation_parameters": {
                "k8s-rb-profile-name": PROFILE_NAME,
                "k8s-rb-profile-source": PROFILE_SOURCE,
                "k8s-rb-instance-release-name": RELEASE_NAME + "-vpkg",
                "k8s-rb-profile-namespace": K8S_NAMESPACE
            },
            "cloud_configuration": K8S_REGION
        },
        VF_MODULE_PREFIX + "vsn": {
            "instantiation_parameters": {
                "k8s-rb-profile-name": PROFILE_NAME,
                "k8s-rb-profile-source": PROFILE_SOURCE,
                "k8s-rb-instance-release-name": RELEASE_NAME + "-vsn",
                "k8s-rb-profile-namespace": K8S_NAMESPACE
            },
            "cloud_configuration": K8S_REGION
        }
    }
    ######## PNF DETAILS ########
    ADD_PNF = False
    if ADD_PNF:
        PNF_VSP_FILE = "vsp/pnf_package.csar"
        PNF_NAME = "PNF_example"
        PNF_VSP_NAME = "VSP_" + PNF_NAME

    ######## DEFAULT VALUES ########
    OWNING_ENTITY = "OE-Demonstration"
    PROJECT = "Project-Demonstration"
    PLATFORM = "test"
    LINE_OF_BUSINESS = "LOB-Demonstration"

    #### REGION DETAILS ####
    CLOUD_REGIONS = {
        K8S_REGION: {
            "complex_id": "k8s-complex1",
            "cloud_owner": "K8sCloudOwner",
            "cloud_type": "k8s",
            "availability_zone": "k8s-availability-zone",
            "tenant": {
                "name": K8S_REGION + "-tenant"
            },
            "customer_resource_definitions": [
                # Uncomment lines below, if you want to run on non KUD k8s cluster
                # "crds/crd1",
                # "crds/crd2"
            ],
            "cluster_kubeconfig_file": "artifacts/kud_kubeconfig"
        # },
        # "openstack-region-test-1": {
        #     "complex_id": "complex1",
        #     "cloud_owner": "CloudOwner",
        #     "cloud_type": "openstack",
        #     "availability_zone": "Main",
        #     "identity_url": "http://test:5000/v4",
        #     "mso_id": "test_use",
        #     "mso_pass": "test_password",
        #     "identity_server_type": "KEYSTONE_V3",
        #     "tenant": {
        #         "id": "5117085204e84027a8d1a0cf34abb0ba",
        #         "name": "onap-dev"
        #     }
        }
    }
    ######## SCENARIOS #############

    ########     1    #############
    if SCENARIO == 1:
        SKIP_POST_INSTANTIATION = True
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["instantiation_parameters"]["k8s-rb-profile-name"] = PROFILE_NAME
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["instantiation_parameters"]["k8s-rb-profile-source"] = PROFILE_SOURCE
    ########     2    #############
    elif SCENARIO == 2:
        SKIP_POST_INSTANTIATION = True
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["instantiation_parameters"]["k8s-rb-profile-name"] = "vfw-cnf-cds-vpkg-profile"
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["instantiation_parameters"]["k8s-rb-profile-source"] = "vfw-cnf-cds-vpkg-profile"
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["instantiation_parameters"]["vpg-management-port"] = "31922"
    ########     3    #############
    elif SCENARIO == 3:
        SKIP_POST_INSTANTIATION = False
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["instantiation_parameters"]["k8s-rb-profile-name"] = PROFILE_NAME
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["instantiation_parameters"]["k8s-rb-profile-source"] = PROFILE_SOURCE
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["instantiation_parameters"]["k8s-rb-config-template-name"] = "ssh-service-config"
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["instantiation_parameters"]["k8s-rb-config-template-source"] = "ssh-service-config"
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["instantiation_parameters"]["k8s-rb-config-name"] = "ssh-service-config"
        VF_MODULE_PARAM_LIST[VF_MODULE_PREFIX + "vpkg"]["instantiation_parameters"]["k8s-rb-config-value-source"] = "ssh-service-config"
    else:
        raise Exception("Not Implemented Scenario")
