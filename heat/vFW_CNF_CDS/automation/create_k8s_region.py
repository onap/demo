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

import logging
import os
from uuid import uuid4

from config import Config
from k8s_client import K8sClient
from so_db_adapter import SoDBUpdate
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

#### Create complex if not exists ####
logger.info("******** Complex *******")
try:
    complex = list(Complex.get_all(physical_location_id=Config.COMPLEX_ID))[0]
    logger.info("Complex exists")
except IndexError:
    logger.info("Complex does not exists")
    complex = Complex.create(physical_location_id=Config.COMPLEX_ID,
                             name=Config.COMPLEX_ID,
                             physical_location_type="office",
                             street1="DummyStreet 1",
                             city="DummyCity",
                             postal_code="00-000",
                             country="DummyCountry",
                             region="DummyRegion")
    logger.info("Complex created")

#### Create cloud region if not exists ####
logger.info("******** Cloud Region *******")
try:
    cloud_region = list(CloudRegion.get_all(cloud_owner=Config.CLOUD_OWNER, cloud_region_id=Config.CLOUD_REGION))[0]
    logger.info("Cloud region exists")
except IndexError:
    logger.info("Cloud region does not exists")
    cloud_region = CloudRegion.create(cloud_owner=Config.CLOUD_OWNER,
                                      cloud_region_id=Config.CLOUD_REGION,
                                      cloud_type="k8s",
                                      owner_defined_type="t1",
                                      cloud_region_version="1.0",
                                      complex_name=complex.physical_location_id,
                                      cloud_zone="CloudZone",
                                      sriov_automation="false",
                                      orchestration_disabled=False,
                                      in_maint=False)
    logger.info("Cloud region created")

logger.info("******** Cloud regiongion <-> Complex *******")
cloud_region.link_to_complex(complex)

logger.info("******** Availability zone *******")
cloud_region.add_availability_zone(availability_zone_name=Config.AVAILABILITY_ZONE_NAME,
                                   availability_zone_hypervisor_type=Config.HYPERVISOR_TYPE)

logger.info("******** Tenant *******")
cloud_region.add_tenant(str(uuid4()), Config.TENANT_NAME)

#### Update or create connectivity info ####
logger.info("******** Connectivity Info *******")
with open(os.path.join(MYPATH, Config.CLUSTER_KUBECONFIG_PATH), 'rb') as kubeconfig_file:
    kubeconfig = kubeconfig_file.read()
try:
    connectivity_info = ConnectivityInfo.get_connectivity_info_by_region_id(cloud_region_id=Config.CLOUD_REGION)
    logger.info("Connectivity Info exists ")
    logger.info("Delete Connectivity Info ")
    connectivity_info.delete()
    connectivity_info = ConnectivityInfo.create(cloud_region_id=Config.CLOUD_REGION,
                                                cloud_owner=Config.CLOUD_OWNER,
                                                kubeconfig=kubeconfig)
    logger.info("Connectivity Info created ")
except:
    logger.info("Connectivity Info does not exists ")
    connectivity_info = ConnectivityInfo.create(cloud_region_id=Config.CLOUD_REGION,
                                                cloud_owner=Config.CLOUD_OWNER,
                                                kubeconfig=kubeconfig)
    logger.info("Connectivity Info created ")

#### Add Custom Resource Definitions ####
k8s_client = K8sClient(kubeconfig_path=Config.CLUSTER_KUBECONFIG_PATH)
for crd in Config.CUSTOMER_RESOURCE_DEFINITIONS:
    k8s_client.create_custom_object(crd)

#### Create customer if not exists ####
logger.info("******** Customer *******")
try:
    customer = Customer.get_by_global_customer_id(Config.GLOBAL_CUSTOMER_ID)
    logger.info("Customer exists")
except:
    logger.info("Customer exists")
    customer = Customer.create(Config.GLOBAL_CUSTOMER_ID, Config.GLOBAL_CUSTOMER_ID, "INFRA")
    logger.info("Customer created")

#### Add region to SO db ####
logger.info("******** SO Database *******")
result = SoDBUpdate.add_region_to_so_db(cloud_region_id=Config.CLOUD_REGION,
                                        complex_id=Config.COMPLEX_ID)
if result.status_code == 201:
    logger.info("Region in SO db created successfully")
else:
    logger.error("Creating region in SO db failed")
