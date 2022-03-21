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

import logging
import os
from uuid import uuid4

from onapsdk.so.so_db_adapter import SoDbAdapter, IdentityService

from config import Config, VariablesDict
from onapsdk.exceptions import ResourceNotFound, APIError
from onapsdk.aai.cloud_infrastructure import Complex, CloudRegion
from onapsdk.msb.k8s import ConnectivityInfo

logger = logging.getLogger()
logger.setLevel(logging.DEBUG)


def create_complex(complex_id):
    logger.info("******** Complex *******")
    try:
        region_complex = next(Complex.get_all(physical_location_id=complex_id))
        logger.info("Complex exists")
        return region_complex
    except ResourceNotFound:
        logger.info("Complex does not exist")
        region_complex = Complex.create(physical_location_id=complex_id,
                                        name=complex_id,
                                        physical_location_type="office",
                                        street1="DummyStreet 1",
                                        city="DummyCity",
                                        postal_code="00-000",
                                        country="DummyCountry",
                                        region="DummyRegion")
        logger.info("Complex created")
        return region_complex


def create_cloud_region(cloud_region):
    logger.info("******** Cloud Region *******")
    region_id = cloud_region["name"]
    cloud_owner = cloud_region["cloud_owner"]
    cloud_type = cloud_region["cloud_type"]
    complex_id = cloud_region["complex_id"]
    cloud_region_version = "1.0" if cloud_type == "k8s" else "v2.5"
    try:
        region = next(CloudRegion.get_all(cloud_owner=cloud_owner, cloud_region_id=region_id))
        logger.info("Cloud region exists")
        return region
    except ResourceNotFound:
        logger.info("Cloud region does not exist")
        region = CloudRegion.create(cloud_owner=cloud_owner,
                                    cloud_region_id=region_id,
                                    cloud_type=cloud_type,
                                    owner_defined_type="t1",
                                    cloud_region_version=cloud_region_version,
                                    complex_name=complex_id,
                                    cloud_zone="CloudZone",
                                    sriov_automation="false",
                                    orchestration_disabled=False,
                                    in_maint=False)
        logger.info("Cloud region created")
        return region


def link_region_to_complex(cloud_region, complx):
    logger.info("******** Cloud region <-> Complex *******")
    cloud_region.link_to_complex(complex_object=complx)


def add_tenant(cloud_region, tenant_id, tenant_name):
    logger.info("Tenant does not exist")
    cloud_region.add_tenant(tenant_id=tenant_id,
                            tenant_name=tenant_name)
    logger.info(f"Tenant {tenant_name} added to region")


def add_tenants(cloud_region, k8s_region, tenants):
    logger.info("******** Tenants *******")
    for tenant in tenants:
        tenant_name = tenant["name"]
        if k8s_region:
            try:
                next(_tenant for _tenant in cloud_region.tenants if _tenant.name == tenant_name)
                logger.info("Tenant exists")
            except (StopIteration, ResourceNotFound):
                tenant_id = str(uuid4())
                add_tenant(cloud_region=cloud_region, tenant_id=tenant_id, tenant_name=tenant_name)
        else:
            tenant_id = tenant["id"]
            try:
                cloud_region.get_tenant(tenant_id)
                logger.info("Tenant exists")
            except ResourceNotFound:
                add_tenant(cloud_region=cloud_region, tenant_id=tenant_id, tenant_name=tenant_name)


def update_connectivity_info(region):
    logger.info("******** Connectivity Info *******")
    kubeconfig_path = region["kubeconfig_file"]
    cloud_owner = region["cloud_owner"]
    region_id = region["name"]
    try:
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)), kubeconfig_path), 'rb') as kubeconfig_file:
            kubeconfig = kubeconfig_file.read()

        connectivity_info = ConnectivityInfo.get_connectivity_info_by_region_id(cloud_region_id=region_id)
        logger.info("Connectivity Info exists ")
        logger.info("Delete Connectivity Info ")
        connectivity_info.delete()
        ConnectivityInfo.create(cloud_region_id=region_id,
                                cloud_owner=cloud_owner,
                                kubeconfig=kubeconfig)
        logger.info("Connectivity Info created ")
    except (APIError, ResourceNotFound):
        logger.info("Connectivity Info does not exists ")
        ConnectivityInfo.create(cloud_region_id=region_id,
                                cloud_owner=cloud_owner,
                                kubeconfig=kubeconfig)
        logger.info("Connectivity Info created ")
    except FileNotFoundError:
        logger.error("Error - File Not Found")
        logger.info("Please check if kubeconfig file exists")
        exit(1)


def add_region_to_so_db(region):
    logger.info("******** SO Database *******")
    if is_k8s_region(region):
        identity_service = IdentityService(identity_id="Keystone_K8s",
                                           url="http://test:5000/v3",
                                           mso_id="onapsdk_user",
                                           mso_pass="mso_pass_onapsdk",
                                           project_domain_name="NULL",
                                           user_domain_name="NULL",
                                           identity_server_type="KEYSTONE")

        SoDbAdapter.add_cloud_site(cloud_region_id=region["name"],
                                   complex_id=region["complex_id"],
                                   identity_service=identity_service,
                                   orchestrator="multicloud")
    else:
        identity_url = region["identity_url"]
        mso_id = region["mso_id"]
        mso_pass = region["mso_pass"]
        identity_server_type = region["identity_server_type"]
        identity_service = IdentityService(identity_id=region["name"] + "_KEYSTONE",
                                           url=identity_url,
                                           mso_id=mso_id,
                                           mso_pass=mso_pass,
                                           project_domain_name="Default",
                                           user_domain_name="Default",
                                           identity_server_type=identity_server_type)

        SoDbAdapter.add_cloud_site(cloud_region_id=region["name"],
                                   complex_id=region["complex_id"],
                                   identity_service=identity_service,
                                   orchestrator="NULL")


def is_k8s_region(region):
    is_k8s = False
    if region["cloud_type"] == "k8s":
        is_k8s = True
    return is_k8s


########################################################################################################################
def main():
    config = Config(env_dict=VariablesDict.env_variable)

    for region in config.cloud_regions:
        complx = create_complex(region["complex_id"])
        cloud_region = create_cloud_region(region)
        link_region_to_complex(cloud_region, complx)
        add_tenants(cloud_region, is_k8s_region(region), region.get("tenants"))
        if is_k8s_region(region):
            update_connectivity_info(region)
        add_region_to_so_db(region)


if __name__ == "__main__":
    sh = logging.StreamHandler()
    sh_formatter = logging.Formatter('%(asctime)s %(levelname)s %(lineno)d:%(filename)s(%(process)d) - %(message)s')
    sh.setFormatter(sh_formatter)
    logger.addHandler(sh)

    main()
