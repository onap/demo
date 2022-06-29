# ============LICENSE_START=======================================================
# Copyright (C) 2021 Orange
# Copyright (C) 2022 Deutsche Telekom AG
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
import os

import oyaml as yaml

from config import Config
from onapsdk.aai.cloud_infrastructure import (
    CloudRegion
)
from onapsdk.aai.business import (
    Customer,
    OwningEntity as AaiOwningEntity
)
from onapsdk.exceptions import ResourceNotFound, APIError
from onapsdk.msb.k8s import Definition

from onapsdk.so.instantiation import (
    ServiceInstantiation,
    InstantiationParameter, VnfParameters, VfmoduleParameters)
from onapsdk.sdc.service import Service
from onapsdk.so.so_element import OrchestrationRequest
from onapsdk.aai.service_design_and_creation import Service as AaiService

logger = logging.getLogger("")
logger.setLevel(logging.DEBUG)
fh = logging.StreamHandler()
fh_formatter = logging.Formatter('%(asctime)s %(levelname)s %(lineno)d:%(filename)s(%(process)d) - %(message)s')
fh.setFormatter(fh_formatter)
logger.addHandler(fh)


def get_customer(global_customer_id):
    try:
        customer = Customer.get_by_global_customer_id(global_customer_id)
        logger.info(f"Customer {customer.subscriber_name} found")
        return customer
    except ResourceNotFound:
        logger.error("Customer not exists. Check if region creation was successfully finished")
        exit(1)


def get_service_model(service_type):
    try:
        service_model = next(model for model in Service.get_all() if model.name == service_type)
        logger.info(
            f"Found Service {service_model.name} in SDC, distribution status: {service_model.distribution_status}")
        return service_model
    except StopIteration:
        logger.error(f"Service model {service_type} not found in SDC")
        exit(1)


def check_service_customer_subscription(customer, service):
    try:
        customer.get_service_subscription_by_service_type(
            service_type=service.name)
    except ResourceNotFound:
        return False

    logger.info(f"Customer {customer.subscriber_name} subscribed for {service.name}")
    return True


def subscribe_service_customer(customer, service):
    if not check_service_customer_subscription(customer, service):
        logger.info("******** Subscribe Service *******")
        customer.subscribe_service(service.name)


def get_cloud_region(cloud_owner, cloud_region):
    return CloudRegion(cloud_owner=cloud_owner, cloud_region_id=cloud_region,
                       orchestration_disabled=True, in_maint=False)


def get_tenant(cloud_region, tenant_name):
    try:
        tenant = next(tenant for tenant in cloud_region.tenants if tenant.name == tenant_name)
        return tenant
    except StopIteration:
        logger.error(f"Tenant {tenant_name} not found")
        exit(1)


def add_owning_entity(owning_entity):
    logger.info("******** Add Owning Entity to AAI *******")
    try:
        aai_owning_entity = AaiOwningEntity.get_by_owning_entity_name(owning_entity)
    except ResourceNotFound:
        logger.info("******** Owning Entity not existing: create *******")
        aai_owning_entity = AaiOwningEntity.create(owning_entity)

    return aai_owning_entity


def delete_old_profiles(service):
    for vnf in service.vnfs:
        for vf_module in vnf.vf_modules:
            vf_module_label = next(vfm_prop.value for vfm_prop in vf_module.properties if
                                   vfm_prop.name == "vf_module_label")
            if vf_module_label == "base_template_dummy_ignore":
                continue
            if "k8s-rb-profile-name" not in Config.VF_MODULE_PARAM_LIST[vf_module_label]["instantiation_parameters"]:
                continue
            try:
                definition = Definition.get_definition_by_name_version(
                    rb_name=vf_module.model_invariant_uuid,
                    rb_version=vf_module.model_customization_id)
            except APIError:
                definition = Definition.get_definition_by_name_version(
                    rb_name=vf_module.model_invariant_uuid,
                    rb_version=vf_module.model_version_id)
            profile_name = Config.VF_MODULE_PARAM_LIST[vf_module_label]["instantiation_parameters"][
                "k8s-rb-profile-name"]
            try:
                profile = definition.get_profile_by_name(profile_name)
                namespace = None
                if "k8s-rb-profile-namespace" in Config.VNF_PARAM_LIST:
                    namespace = Config.VNF_PARAM_LIST["k8s-rb-profile-namespace"]
                if "k8s-rb-profile-namespace" in Config.VF_MODULE_PARAM_LIST[vf_module_label]["instantiation_parameters"]:
                    namespace = Config.VF_MODULE_PARAM_LIST[vf_module_label]["instantiation_parameters"]["k8s-rb-profile-namespace"]
                if namespace != None and profile.namespace != namespace:
                    profile.delete()
                    logger.info("Profile: " + profile_name + " for " + vf_module.name + " deleted")
                else:
                    logger.info("No need to delete Profile " + profile_name +
                                " for " + vf_module.name + ". Namespace is fine")
            except ResourceNotFound:
                logger.info("Profile: " + profile_name + " for " + vf_module.name + " not found")


def read_sdnc_model_details(file):
    mypath = os.path.dirname(os.path.realpath(__file__))
    file_path = os.path.join(mypath, file)
    with zipfile.ZipFile(file_path, 'r') as package:
        cba_io = BytesIO(package.read("CBA.zip"))
        with zipfile.ZipFile(cba_io) as cba:
            with cba.open('TOSCA-Metadata/TOSCA.meta') as meta_file:
                tosca_meta = yaml.load(meta_file, Loader=yaml.FullLoader)
                sdnc_model_name = tosca_meta.get("Template-Name")
                sdnc_model_version = tosca_meta.get("Template-Version")
    return sdnc_model_name, sdnc_model_version


def check_service_instance_exists(service_subscription, service_instance_name):
    try:
        service_instance = next((instance for instance in service_subscription.service_instances
                                 if instance.instance_name == service_instance_name), None)
        return service_instance
    except ResourceNotFound:
        return None


def get_vfmodule_parameters(vf_module, vf_module_instantiation_parameters, sdnc_model_name, sdnc_model_version):
    base_parameters = [
        InstantiationParameter(name="sdnc_model_name", value=sdnc_model_name),
        InstantiationParameter(name="sdnc_model_version", value=sdnc_model_version),
        InstantiationParameter(name="vf_module_label", value=vf_module)]

    for instantiation_parameter_key, instantiation_parameter_value in vf_module_instantiation_parameters:
        base_parameters.append(InstantiationParameter(name=instantiation_parameter_key,
                                                      value=instantiation_parameter_value))

    return VfmoduleParameters(vf_module, base_parameters)


def get_vnf_parameters(sdnc_model_name, sdnc_model_version, sdnc_artifact_name, vnf_param_list):
    vnf_parameters = [
        InstantiationParameter(name="sdnc_model_name", value=sdnc_model_name),
        InstantiationParameter(name="sdnc_model_version", value=sdnc_model_version),
        InstantiationParameter(name="sdnc_artifact_name", value=sdnc_artifact_name)]

    for vnf_param_name, vnf_param_value in vnf_param_list.items():
        vnf_parameters.append(
            InstantiationParameter(name=vnf_param_name, value=vnf_param_value)
        )

    return vnf_parameters


def check_orchestration_status(instantiation):
    status = None
    while not (status == OrchestrationRequest.StatusEnum.COMPLETED
               or status == OrchestrationRequest.StatusEnum.FAILED):
        sleep(10)
        status = instantiation.status
        logger.info(f"Orchestration status is: {status.value}")

    if status == OrchestrationRequest.StatusEnum.FAILED:
        logger.error("Orchestration was failed!")
        exit(1)
    else:
        logger.info("Orchestration was succeed")
        return


def get_aai_service(service_type):
    logging.info("******** Retrieve product family for service *******")
    try:
        aai_service = next(service for service in AaiService.get_all() if service.service_id == service_type)
    except (ResourceNotFound, StopIteration):
        logging.info("******** Service design and creation in AAI not existing: create *******")
        AaiService.create(service_id=service_type, service_description=service_type)
        aai_service = next(service for service in AaiService.get_all() if service.service_id == service_type)

    return aai_service


def instantiate_service_macro(service_instance_name,
                              sdnc_model_name, sdnc_model_version, sdnc_artifact_name, vf_name, vnf_param_list,
                              vf_module_list, service, cloud_region, tenant, customer, owning_entity,
                              vid_project, vid_line_of_business, vid_platform, service_subscription):
    # TODO: support for multiple vnf should be added BEGINING of the loop
    vnf_parameters = get_vnf_parameters(sdnc_model_name, sdnc_model_version, sdnc_artifact_name, vnf_param_list)

    vfmodule_parameters = []
    for vfmodule in vf_module_list:
        vf_module_instantiation_parameters = vf_module_list[vfmodule]["instantiation_parameters"].items()
        vfmodule_parameters.append(get_vfmodule_parameters(vfmodule, vf_module_instantiation_parameters,
                                                           sdnc_model_name, sdnc_model_version))

    vnf_instantiation_parameters = VnfParameters(name=vf_name,
                                                 vnf_parameters=vnf_parameters,
                                                 vfmodule_parameters=vfmodule_parameters)
    # END of the loop
    aai_service = get_aai_service(service.name)
    service_instantiation = ServiceInstantiation.instantiate_macro(
        sdc_service=service,
        cloud_region=cloud_region,
        tenant=tenant,
        customer=customer,
        owning_entity=owning_entity,
        project=vid_project,
        line_of_business=vid_line_of_business,
        platform=vid_platform,
        service_instance_name=service_instance_name,
        vnf_parameters=[vnf_instantiation_parameters],
        aai_service=aai_service,
        service_subscription=service_subscription
    )
    check_orchestration_status(service_instantiation)


def get_base_vf_module(vf_modules):
    base_vf_module = next(vf_module for vf_module in vf_modules if vf_module.properties["isBase"])
    return base_vf_module


def is_base(vf_module):
    if vf_module.properties["isBase"]:
        return True
    return False


def instantiate_vf_module(vf_module, vf_module_param_list, vnf_instance, sdnc_model_name, sdnc_model_version):
    vf_module_label = vf_module.properties["vf_module_label"]
    region_id = vf_module_param_list[vf_module_label]["cloud_configuration"]
    cloud_region = get_cloud_region(
        Config.CLOUD_REGIONS[region_id]["cloud_owner"],
        region_id)
    tenant = get_tenant(cloud_region, Config.CLOUD_REGIONS[region_id]["tenant"]["name"])

    vfmodule_instantiation_parameters = vf_module_param_list[vf_module_label]["instantiation_parameters"].items()

    base_parameters = [
        InstantiationParameter(name="sdnc_model_name", value=sdnc_model_name),
        InstantiationParameter(name="sdnc_model_version", value=sdnc_model_version),
        InstantiationParameter(name="vf_module_label", value=vf_module_label)]

    for instantiation_parameter_key, instantiation_parameter_value in vfmodule_instantiation_parameters:
        base_parameters.append(InstantiationParameter(name=instantiation_parameter_key,
                                                      value=instantiation_parameter_value))

    vf_module_instantiation = vnf_instance.add_vf_module(
        vf_module=vf_module,
        cloud_region=cloud_region,
        tenant=tenant,
        vnf_parameters=base_parameters,
        use_preload=False
    )
    check_orchestration_status(vf_module_instantiation)


def check_vf_module_list_correct(vf_modules, vf_module_param_list):
    labels = set()
    for vf_module in vf_modules:
        labels.add(vf_module.properties["vf_module_label"])
    if vf_module_param_list.keys() == labels:
        return True
    else:
        return False


def instantiate_service_alacarte(service_subscription, service_instance_name,
                                 sdnc_model_name, sdnc_model_version, sdnc_artifact_name, vnf_param_list,
                                 vf_module_param_list, service, cloud_region, tenant, customer, owning_entity,
                                 vid_project, vid_line_of_business, vid_platform):
    # Tworzenie Serwisu
    service_instantiation = ServiceInstantiation.instantiate_ala_carte(
        sdc_service=service,
        cloud_region=cloud_region,
        tenant=tenant,
        customer=customer,
        owning_entity=owning_entity,
        project=vid_project,
        service_instance_name=service_instance_name
    )
    check_orchestration_status(service_instantiation)
    # Koniec tworzenia Serwisu

    service_instance = service_subscription.get_service_instance_by_name(service_instance_name)
    # Dodawanie VNFs (VF) do Serwisu
    for vnf in service.vnfs:
        vnf_parameters = get_vnf_parameters(sdnc_model_name, sdnc_model_version, sdnc_artifact_name, vnf_param_list)
        vnf_instantiation = service_instance.add_vnf(
            # vnf_instance_name=vf_name, TODO: support for multiple vnf in config file
            vnf=vnf,
            line_of_business=vid_line_of_business,
            platform=vid_platform,
            vnf_parameters=vnf_parameters
        )
        check_orchestration_status(vnf_instantiation)

        # Dodawanie VF Modulow
        vnf_type = service.name + "/" + vnf.name
        vnf_instance = next((vnf for vnf in service_instance.vnf_instances if vnf.vnf_type == vnf_type), None)

        if check_vf_module_list_correct(vnf.vf_modules, vf_module_param_list):
            for vf_module_label in vf_module_param_list:
                vf_module = next(
                    vf_module for vf_module in vnf.vf_modules if
                    vf_module.properties["vf_module_label"] == vf_module_label)
                instantiate_vf_module(vf_module, vf_module_param_list, vnf_instance, sdnc_model_name,
                                      sdnc_model_version)
        else:
            logger.error("VF_MODULE_PARAM_LIST error. ")
        # Koniec dodawania VF Modulow
    # Koniec dodawania VNFs

    # # Dodanie base
    # base_vf_module = get_base_vf_module(vnf.vf_modules)
    # instantiate_vf_module(base_vf_module, vf_module_param_list, vnf_instance, sdnc_model_name, sdnc_model_version)
    # # Dodanie reszty
    # for vf_module in vnf.vf_modules:
    #     if not is_base(vf_module):
    #         instantiate_vf_module(vf_module, vf_module_param_list, vnf_instance, sdnc_model_name,
    #                               sdnc_model_version)


def main():
    logger.info("*******************************")
    logger.info("**** SERVICE INSTANTIATION ****")
    logger.info("*******************************")

    logger.info("******** GET Customer *******")
    customer = get_customer(Config.GLOBAL_CUSTOMER_ID)

    logger.info("******** GET Service Model from SDC *******")
    service = get_service_model(Config.SERVICENAME)

    logger.info("******** Subscribe Customer for Service *******")
    subscribe_service_customer(customer, service)

    logger.info("******** Get Tenant *******")
    basic_cloud_region_name = next(iter(Config.CLOUD_REGIONS.keys()))
    cloud_region = get_cloud_region(Config.CLOUD_REGIONS[basic_cloud_region_name]["cloud_owner"],
                                    basic_cloud_region_name)
    tenant = get_tenant(cloud_region,
                        Config.CLOUD_REGIONS[basic_cloud_region_name]["tenant"]["name"])

    ######
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
    ####

    logger.info("******** Business Objects (OE, P, Pl, LoB) *******")
    project = Config.PROJECT
    platform = Config.PLATFORM
    line_of_business = Config.LINE_OF_BUSINESS
    owning_entity = add_owning_entity(Config.OWNING_ENTITY)

    logger.info("******** Delete old profiles ********")
    delete_old_profiles(service)

    logger.info("******** Read SDNC MODEL NAME and VERSION from CBA.zip ********")
    sdnc_model_name, sdnc_model_version = read_sdnc_model_details(Config.VSPFILE)

    logger.info("******** Instantiate Service *******")
    service_instance = check_service_instance_exists(service_subscription, Config.SERVICE_INSTANCE_NAME)
    if service_instance:
        logger.info("******** Service Instance exists, do not instantiate *******")
    else:
        logger.info("******** Service Instance not existing: Instantiate *******")
        if Config.MACRO_INSTANTIATION:
            instantiate_service_macro(Config.SERVICE_INSTANCE_NAME, sdnc_model_name,
                                      sdnc_model_version, Config.SDNC_ARTIFACT_NAME, Config.VFNAME,
                                      Config.VNF_PARAM_LIST,
                                      Config.VF_MODULE_PARAM_LIST,
                                      service, cloud_region, tenant, customer, owning_entity, project,
                                      line_of_business, platform, service_subscription)
        else:
            instantiate_service_alacarte(service_subscription, Config.SERVICE_INSTANCE_NAME, sdnc_model_name,
                                         sdnc_model_version, Config.SDNC_ARTIFACT_NAME, Config.VNF_PARAM_LIST,
                                         Config.VF_MODULE_PARAM_LIST,
                                         service, cloud_region, tenant, customer, owning_entity, project,
                                         line_of_business, platform)


if __name__ == "__main__":
    main()
