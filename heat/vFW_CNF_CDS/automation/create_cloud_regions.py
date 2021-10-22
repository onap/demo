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

from config import Config
from k8s_client import K8sClient
from onapsdk.exceptions import ResourceNotFound, APIError
from onapsdk.aai.business import Customer
from onapsdk.aai.cloud_infrastructure import Complex, CloudRegion
from onapsdk.msb.k8s import ConnectivityInfo

logger = logging.getLogger("")
logger.setLevel(logging.DEBUG)
fh = logging.StreamHandler()
fh_formatter = logging.Formatter('%(asctime)s %(levelname)s %(lineno)d:%(filename)s(%(process)d) - %(message)s')
fh.setFormatter(fh_formatter)
logger.addHandler(fh)

MYPATH = os.path.dirname(os.path.realpath(__file__))


def create_complex(region_id):
    #### Create complex if not exists ####
    logger.info("******** Complex *******")
    complex_id = Config.CLOUD_REGIONS[region_id]["complex_id"]
    try:
        region_complex = next(Complex.get_all(physical_location_id=complex_id))
        logger.info("Complex exists")
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
    return region_complex


def create_cloud_region(region_id):
    #### Create cloud region if not exists ####
    logger.info("******** Cloud Region *******")
    cloud_owner = Config.CLOUD_REGIONS[region_id]["cloud_owner"]
    cloud_type = Config.CLOUD_REGIONS[region_id]["cloud_type"]
    complex_id = Config.CLOUD_REGIONS[region_id]["complex_id"]
    cloud_region_version = "1.0" if cloud_type == "k8s" else "v2.5"
    try:
        region = next(CloudRegion.get_all(cloud_owner=cloud_owner, cloud_region_id=region_id))
        logger.info("Cloud region exists")
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
    return region


def link_region_to_complex(cloud_region, complx):
    logger.info("******** Cloud region <-> Complex *******")
    cloud_region.link_to_complex(complex_object=complx)


def add_availability_zone(cloud_region):
    logger.info("******** Availability zone *******")
    region_id = cloud_region.cloud_region_id
    availability_zone_name = Config.CLOUD_REGIONS[region_id]["availability_zone"]
    cloud_type = Config.CLOUD_REGIONS[region_id]["cloud_type"]
    try:
        cloud_region.get_availability_zone_by_name(availability_zone_name)
        logger.info("Availability zone exists")
    except ResourceNotFound:
        logger.info("Availability zone does not exist")
        cloud_region.add_availability_zone(availability_zone_name=availability_zone_name,
                                           availability_zone_hypervisor_type=cloud_type)
        logger.info("Availability zone added to region")


def add_tenant(cloud_region):
    logger.info("******** Tenant *******")
    region_id = cloud_region.cloud_region_id
    is_k8s = is_k8s_region(region_id)
    tenant_name = Config.CLOUD_REGIONS[region_id]["tenant"]["name"]

    if is_k8s:
        try:
            next(_tenant for _tenant in cloud_region.tenants if _tenant.name == tenant_name)
            logger.info("Tenant exists")
        except (StopIteration, ResourceNotFound):
            tenant_id = str(uuid4())
            logger.info("Tenant does not exist")
            cloud_region.add_tenant(tenant_id=tenant_id,
                                    tenant_name=tenant_name)
            logger.info(f"Tenant {tenant_name} added to region")
    else:
        tenant_id = Config.CLOUD_REGIONS[region_id]["tenant"]["id"]
        try:
            cloud_region.get_tenant(tenant_id)
            logger.info("Tenant exists")
        except ResourceNotFound:
            logger.info("Tenant does not exist")
            cloud_region.add_tenant(tenant_id=tenant_id,
                                    tenant_name=tenant_name)
            logger.info(f"Tenant {tenant_name} added to region")


def create_customer():
    #### Create customer if not exists ####
    logger.info("******** Customer *******")
    try:
        Customer.get_by_global_customer_id(Config.GLOBAL_CUSTOMER_ID)
        logger.info("Customer exists")
    except ResourceNotFound:
        logger.info("Customer does not exist")
        Customer.create(Config.GLOBAL_CUSTOMER_ID, Config.GLOBAL_CUSTOMER_ID, "INFRA")
        logger.info("Customer created")


def update_connectivity_info(region_id):
    #### Update or create connectivity info ####
    logger.info("******** Connectivity Info *******")
    cluster_kubeconfig_path = Config.CLOUD_REGIONS[region_id]["cluster_kubeconfig_file"]
    cloud_owner = Config.CLOUD_REGIONS[region_id]["cloud_owner"]
    with open(os.path.join(MYPATH, cluster_kubeconfig_path), 'rb') as kubeconfig_file:
        kubeconfig = kubeconfig_file.read()
    try:
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


def add_custom_resource_definitions(region_id):
    #### Add Custom Resource Definitions ####
    logger.info("******** Custom Resource Definitions *******")
    cluster_kubeconfig_path = Config.CLOUD_REGIONS[region_id]["cluster_kubeconfig_file"]
    k8s_client = K8sClient(kubeconfig_path=cluster_kubeconfig_path)
    for crd in Config.CLOUD_REGIONS[region_id]["customer_resource_definitions"]:
        k8s_client.create_custom_object(crd)


def add_region_to_so_db(region):
    #### Add region to SO db ####
    logger.info("******** SO Database *******")
    if is_k8s_region(region.cloud_region_id):
        identity_service = IdentityService(identity_id="Keystone_K8s",
                                           url="http://test:5000/v3",
                                           mso_id="onapsdk_user",
                                           mso_pass="mso_pass_onapsdk",
                                           project_domain_name="NULL",
                                           user_domain_name="NULL",
                                           identity_server_type="KEYSTONE")

        SoDbAdapter.add_cloud_site(cloud_region_id=region.cloud_region_id,
                                   complex_id=region.complex_name,
                                   identity_service=identity_service,
                                   orchestrator="multicloud")
    else:
        identity_url = Config.CLOUD_REGIONS[region.cloud_region_id]["identity_url"]
        mso_id = Config.CLOUD_REGIONS[region.cloud_region_id]["mso_id"]
        mso_pass = Config.CLOUD_REGIONS[region.cloud_region_id]["mso_pass"]
        identity_server_type = Config.CLOUD_REGIONS[region.cloud_region_id]["identity_server_type"]
        identity_service = IdentityService(identity_id=region.cloud_region_id + "_KEYSTONE",
                                           url=identity_url,
                                           mso_id=mso_id,
                                           mso_pass=mso_pass,
                                           project_domain_name="Default",
                                           user_domain_name="Default",
                                           identity_server_type=identity_server_type)

        SoDbAdapter.add_cloud_site(cloud_region_id=region.cloud_region_id,
                                   complex_id=region.complex_name,
                                   identity_service=identity_service,
                                   orchestrator="NULL")


def is_k8s_region(region_id):
    is_k8s = False
    if Config.CLOUD_REGIONS[region_id]["cloud_type"] == "k8s":
        is_k8s = True
    return is_k8s


########################################################################################################################
def main():
    create_customer()

    for cloud_region_id in Config.CLOUD_REGIONS:

        complx = create_complex(cloud_region_id)
        cloud_region = create_cloud_region(cloud_region_id)
        link_region_to_complex(cloud_region, complx)
        add_availability_zone(cloud_region)
        add_tenant(cloud_region)

        if is_k8s_region(cloud_region_id):
            update_connectivity_info(cloud_region_id)
            add_custom_resource_definitions(cloud_region_id)

        add_region_to_so_db(cloud_region)


if __name__ == "__main__":
    main()
