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
import zipfile
from io import BytesIO
from time import sleep
from uuid import uuid4

import oyaml as yaml

from config import Config
from onapsdk.aai.cloud_infrastructure import (
    CloudRegion,
)
from onapsdk.aai.business import (
    Customer,
    OwningEntity as AaiOwningEntity
)
from onapsdk.msb.k8s import Definition

from onapsdk.so.instantiation import (
    ServiceInstantiation,
    InstantiationParameter, VnfParameters, VfmoduleParameters)
from onapsdk.sdc.service import Service
from onapsdk.vid import LineOfBusiness, OwningEntity, Platform, Project
from onapsdk.so.so_element import OrchestrationRequest

logger = logging.getLogger("")
logger.setLevel(logging.DEBUG)
fh = logging.StreamHandler()
fh_formatter = logging.Formatter('%(asctime)s %(levelname)s %(lineno)d:%(filename)s(%(process)d) - %(message)s')
fh.setFormatter(fh_formatter)
logger.addHandler(fh)

logger.info("*******************************")
logger.info("**** SERVICE INSTANTIATION ****")
logger.info("*******************************")

logger.info("******** Create Customer *******")
customer = None
for found_customer in list(Customer.get_all()):
    logger.debug("Customer %s found", found_customer.subscriber_name)
    if found_customer.subscriber_name == Config.GLOBAL_CUSTOMER_ID:
        logger.info("Customer %s found", found_customer.subscriber_name)
        customer = found_customer
        break
if not customer:
    customer = Customer.create(Config.GLOBAL_CUSTOMER_ID, Config.GLOBAL_CUSTOMER_ID, "INFRA")

logger.info("******** Find Service in SDC *******")
service = None
services = Service.get_all()
for found_service in services:
    logger.debug("Service %s is found, distribution %s", found_service.name, found_service.distribution_status)
    if found_service.name == Config.SERVICENAME:
        logger.info("Found Service %s in SDC", found_service.name)
        service = found_service
        break

if not service:
    logger.error("Service %s not found in SDC", Config.SERVICENAME)
    exit(1)

logger.info("******** Check Service Subscription *******")
service_subscription = None
for service_sub in customer.service_subscriptions:
    logger.debug("Service subscription %s is found", service_sub.service_type)
    if service_sub.service_type == Config.SERVICENAME:
        logger.info("Service %s subscribed", Config.SERVICENAME)
        service_subscription = service_sub
        break

if not service_subscription:
    logger.info("******** Subscribe Service *******")
    customer.subscribe_service(service)

logger.info("******** Get Tenant *******")
cloud_region = CloudRegion(cloud_owner=Config.CLOUD_OWNER, cloud_region_id=Config.CLOUD_REGION,
                           orchestration_disabled=True, in_maint=False)
tenant = None
for found_tenant in cloud_region.tenants:
    logger.debug("Tenant %s found in %s_%s", found_tenant.name, cloud_region.cloud_owner, cloud_region.cloud_region_id)
    if found_tenant.name == Config.TENANT_NAME:
        logger.info("Found my Tenant %s", found_tenant.name)
        tenant = found_tenant
        break

if not tenant:
    logger.error("tenant %s not found", Config.TENANT_NAME)
    exit(1)

logger.info("******** Connect Service to Tenant *******")
service_subscription = None
for service_sub in customer.service_subscriptions:
    logger.debug("Service subscription %s is found", service_sub.service_type)
    if service_sub.service_type == Config.SERVICENAME:
        logger.info("Service %s subscribed", Config.SERVICENAME)
        service_subscription = service_sub
        break

if not service_subscription:
    logger.error("Service subscription %s is not found", Config.SERVICENAME)
    exit(1)

service_subscription.link_to_cloud_region_and_tenant(cloud_region, tenant)

logger.info("******** Add Business Objects (OE, P, Pl, LoB) in VID *******")
vid_owning_entity = OwningEntity.create(Config.OWNING_ENTITY)
vid_project = Project.create(Config.PROJECT)
vid_platform = Platform.create(Config.PLATFORM)
vid_line_of_business = LineOfBusiness.create(Config.LINE_OF_BUSINESS)

logger.info("******** Add Owning Entity in AAI *******")
owning_entity = None
for oe in AaiOwningEntity.get_all():
    if oe.name == vid_owning_entity.name:
        owning_entity = oe
        break
if not owning_entity:
    logger.info("******** Owning Entity not existing: create *******")
    owning_entity = AaiOwningEntity.create(vid_owning_entity.name, str(uuid4()))

logger.info("******** Delete old profiles ********")
for vnf in service.vnfs:
    for vf_module in vnf.vf_modules:
        definition = Definition.get_definition_by_name_version(vf_module.metadata["vfModuleModelInvariantUUID"],
                                                               vf_module.metadata["vfModuleModelUUID"])
        vf_module_label = vf_module.properties["vf_module_label"]
        if vf_module_label == "base_template_dummy_ignore":
            continue
        profile_name = Config.VF_MODULE_PARAM_LIST[vf_module_label]["k8s-rb-profile-name"]
        try:
            profile = definition.get_profile_by_name(profile_name)
            namespace = None
            if "k8s-rb-profile-namespace" in Config.VNF_PARAM_LIST:
                namespace = Config.VNF_PARAM_LIST["k8s-rb-profile-namespace"]
            if "k8s-rb-profile-namespace" in Config.VF_MODULE_PARAM_LIST[vf_module_label]:
                namespace = Config.VF_MODULE_PARAM_LIST[vf_module_label]["k8s-rb-profile-namespace"]
            if namespace != None and profile.namespace != namespace:
                profile.delete()
                logger.info("Profile: " + profile_name + " for " + vf_module.name + " deleted")
            else:
                logger.info("No need to delete Profile " + profile_name +
                            " for " + vf_module.name + ". Namespace is fine")
        except ValueError:
            logger.info("Profile: " + profile_name + " for " + vf_module.name + " not found")


# Read SDNC MODEL NAME and VERSION from CBA.zip
logger.info("*******************************")
logger.info("Retrieving SDNC MODEL NAME and VERSION")
logger.info("*******************************")
with zipfile.ZipFile(Config.VSPFILE, 'r') as package:
    cba_io = BytesIO(package.read("CBA.zip"))
    with zipfile.ZipFile(cba_io) as cba:
        with cba.open('TOSCA-Metadata/TOSCA.meta') as meta_file:
            tosca_meta = yaml.load(meta_file, Loader=yaml.FullLoader)
            SDNC_MODEL_NAME = tosca_meta.get("Template-Name")
            SDNC_MODEL_VERSION = tosca_meta.get("Template-Version")

logger.info("******** Instantiate Service *******")
service_instance = None
service_instantiation = None
for se in service_subscription.service_instances:
    if se.instance_name == Config.SERVICE_INSTANCE_NAME:
        service_instance = se
        break
if not service_instance:
    logger.info("******** Service Instance not existing: Instantiate *******")
    # Instantiate service
    vfmodules_list = Config.VF_MODULE_PARAM_LIST
    vnf_param_list = Config.VNF_PARAM_LIST

    vnf_param = [
        InstantiationParameter(name="sdnc_model_name", value=SDNC_MODEL_NAME),
        InstantiationParameter(name="sdnc_model_version", value=SDNC_MODEL_VERSION),
        InstantiationParameter(name="sdnc_artifact_name", value=Config.SDNC_ARTIFACT_NAME)]

    for vnf_param_name, vnf_param_value in vnf_param_list.items():
        vnf_param.append(
            InstantiationParameter(name=vnf_param_name, value=vnf_param_value)
        )

    vfmodules_param = []
    for vfmodule in vfmodules_list:
        params = [
            InstantiationParameter(name="sdnc_model_name", value=SDNC_MODEL_NAME),
            InstantiationParameter(name="sdnc_model_version", value=SDNC_MODEL_VERSION),
            InstantiationParameter(name="vf_module_label", value=vfmodule)]

        for vfmodule_param_name, vfmodule_param_value in vfmodules_list[vfmodule].items():
            params.append(
                InstantiationParameter(name=vfmodule_param_name, value=vfmodule_param_value)
            )

        vfmodules_param.append(VfmoduleParameters(vfmodule, params))

    vnf_params = VnfParameters(name=Config.VFNAME, vnf_parameters=vnf_param, vfmodule_parameters=vfmodules_param)

    service_instantiation = ServiceInstantiation.instantiate_macro(
        sdc_service=service,
        cloud_region=cloud_region,
        tenant=tenant,
        customer=customer,
        owning_entity=owning_entity,
        project=vid_project,
        line_of_business=vid_line_of_business,
        platform=vid_platform,
        service_instance_name=Config.SERVICE_INSTANCE_NAME,
        vnf_parameters=[vnf_params]
    )
    logger.info("Instantiation request ID: %s", service_instantiation.request_id)
    logger.info("Service Instance ID: %s", service_instantiation.instance_id)
    status = None
    while not (status == OrchestrationRequest.StatusEnum.COMPLETED
               or status == OrchestrationRequest.StatusEnum.FAILED):
        sleep(10)
        status = service_instantiation.status
        logger.info(f"Orchestration status is: {status.value}")
