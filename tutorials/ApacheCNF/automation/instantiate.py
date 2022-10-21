# ============LICENSE_START=======================================================
# Copyright (C) 2021 Orange
# Modification Copyright (C) 2022 Deutsche Telekom AG
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
from time import sleep

from config import Config, VariablesDict
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
    InstantiationParameter, SoService, SoServiceVnf)
from onapsdk.sdc.service import Service
from onapsdk.so.so_element import OrchestrationRequest
from onapsdk.aai.service_design_and_creation import Service as AaiService

logger = logging.getLogger()
logger.setLevel(logging.DEBUG)


def get_customer(global_customer_id: str = "customer_cnf"):
    logger.info("******** Customer *******")
    try:
        customer = Customer.get_by_global_customer_id(global_customer_id)
        logger.info("Customer exists")
    except ResourceNotFound:
        logger.info("Customer does not exist")
        customer = Customer.create(global_customer_id, global_customer_id, "INFRA")
        logger.info("Customer created")
    return customer


def get_service_model(model_name):
    try:
        service_model = next(model for model in Service.get_all() if model.name == model_name)
        logger.info(
            f"Found Service {service_model.name} in SDC, distribution status: {service_model.distribution_status}")
        return service_model
    except StopIteration:
        logger.error(f"Service model {model_name} not found in SDC")
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
    try:
        cloud_region_object = next(cr for cr in CloudRegion.get_all() if
                                   cr.cloud_region_id == cloud_region and cr.cloud_owner == cloud_owner)
        return cloud_region_object
    except (StopIteration, ResourceNotFound):
        logging.error("Cloud region not found!!!")
        exit(1)


def get_tenant(cloud_region, tenant_name):
    try:
        tenant = next(tenant for tenant in cloud_region.tenants if tenant.name == tenant_name)
        return tenant
    except (StopIteration, ResourceNotFound):
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


def delete_old_profiles(service, service_config):
    for vnf in service.vnfs:
        vnf_config_details = next(
            (_vnf for _vnf in service_config["vnfs"] if _vnf["model_name"] == vnf.model_name), None)
        if not vnf_config_details:
            continue
        for vf_module in vnf.vf_modules:
            vf_module_label = next(vfm_prop.value for vfm_prop in vf_module.properties if
                                   vfm_prop.name == "vf_module_label")
            if vf_module_label == "base_template_dummy_ignore":
                continue
            vf_module_config_details = next((_vf_module for _vf_module in vnf_config_details["vf_modules"] if
                                            _vf_module["model_name"] == vf_module_label), None)
            if not vf_module_config_details:
                continue
            if "k8s-rb-profile-name" not in vf_module_config_details["parameters"]:
                continue
            try:
                definition = Definition.get_definition_by_name_version(
                    rb_name=vf_module.model_invariant_uuid,
                    rb_version=vf_module.model_customization_id)
            except APIError:
                definition = Definition.get_definition_by_name_version(
                    rb_name=vf_module.model_invariant_uuid,
                    rb_version=vf_module.model_version_id)
            profile_name = vf_module_config_details["parameters"]["k8s-rb-profile-name"]
            try:
                profile = definition.get_profile_by_name(profile_name)
                namespace = None
                if "k8s-rb-profile-namespace" in vnf_config_details["parameters"]:
                    namespace = vnf_config_details["parameters"]["k8s-rb-profile-namespace"]
                if "k8s-rb-profile-namespace" in vf_module_config_details["parameters"]:
                    namespace = vf_module_config_details["parameters"]["k8s-rb-profile-namespace"]
                if namespace is not None and profile.namespace != namespace:
                    profile.delete()
                    logger.info("Profile: " + profile_name + " for " + vf_module.name + " deleted")
                else:
                    logger.info("No need to delete Profile " + profile_name +
                                " for " + vf_module.name + ". Namespace is fine")
            except ResourceNotFound:
                logger.info("Profile: " + profile_name + " for " + vf_module.name + " not found")


def check_service_instance_exists(service_subscription, service_instance_name):
    try:
        service_instance = next((instance for instance in service_subscription.service_instances
                                 if instance.instance_name == service_instance_name), None)
        return service_instance
    except ResourceNotFound:
        return None


def get_instantiation_parameters(properties, vnf_vf_module_config):
    instantiation_parameters = []
    for property_name, property_value in properties:
        instantiation_parameters.append(InstantiationParameter(name=property_name, value=property_value))

    for instantiation_parameter_key, instantiation_parameter_value in vnf_vf_module_config["parameters"]:
        instantiation_parameters.append(InstantiationParameter(name=instantiation_parameter_key,
                                                               value=instantiation_parameter_value))

    return instantiation_parameters


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


def instantiate_service_macro(config, service, cloud_region, tenant, customer, owning_entity,
                              project, line_of_business, platform):
    service_instance_name = config.service_instance["instance_name"]
    so_input = config.so_input
    for vnf in so_input["vnfs"]:
        _vnf = next(nf for nf in service.vnfs if nf.model_name == vnf["model_name"])
        sdnc_model_name = next(prop.value for prop in _vnf.properties if prop.name == "sdnc_model_name")
        sdnc_model_version = next(prop.value for prop in _vnf.properties if prop.name == "sdnc_model_version")
        sdnc_artifact_name = next(prop.value for prop in _vnf.properties if prop.name == "sdnc_artifact_name")

        vnf["parameters"]["sdnc_model_name"] = sdnc_model_name
        vnf["parameters"]["sdnc_model_version"] = sdnc_model_version
        vnf["parameters"]["sdnc_artifact_name"] = sdnc_artifact_name
        for vf_module in vnf["vf_modules"]:
            vf_module_label = vf_module["model_name"]
            vf_module["parameters"]["sdnc_model_name"] = sdnc_model_name
            vf_module["parameters"]["sdnc_model_version"] = sdnc_model_version
            vf_module["parameters"]["vf_module_label"] = vf_module_label

    # TODO: PNF support in so_input -> first ONAPSDK

    so_service = SoService.load(so_input)

    aai_service = get_aai_service(service.name)
    service_instantiation = ServiceInstantiation.instantiate_macro(
        sdc_service=service,
        cloud_region=cloud_region,
        tenant=tenant,
        customer=customer,
        owning_entity=owning_entity,
        project=project,
        line_of_business=line_of_business,
        platform=platform,
        service_instance_name=service_instance_name,
        aai_service=aai_service,
        so_service=so_service
    )
    check_orchestration_status(service_instantiation)


def instantiate_vnfs_macro(config, service_subscription, line_of_business, platform):

    service_instance = check_service_instance_exists(service_subscription, config.service_instance["instance_name"])

    so_input_vnfs_locations = config.create_so_input(other_cluster=True)

    if not so_input_vnfs_locations["vnfs"]:
        return

    so_input_vnfs_by_priority = sorted(so_input_vnfs_locations["vnfs"],
                                       key=lambda _vnf: _vnf.get("processing_priority", 100))

    for vnf in so_input_vnfs_by_priority:

        cloud_owner_name = next(
            region["cloud_owner"] for region in config.cloud_regions if region["name"] == vnf["cloud_region"])
        cloud_region = get_cloud_region(cloud_owner=cloud_owner_name, cloud_region=vnf["cloud_region"])

        vnf_model = next(nf for nf in service_instance.sdc_service.vnfs
                         if nf.model_name == vnf["model_name"])

        sdnc_model_name = next(prop.value for prop in vnf_model.properties if prop.name == "sdnc_model_name")
        sdnc_model_version = next(prop.value for prop in vnf_model.properties if prop.name == "sdnc_model_version")
        sdnc_artifact_name = next(prop.value for prop in vnf_model.properties if prop.name == "sdnc_artifact_name")

        vnf["parameters"]["sdnc_model_name"] = sdnc_model_name
        vnf["parameters"]["sdnc_model_version"] = sdnc_model_version
        vnf["parameters"]["sdnc_artifact_name"] = sdnc_artifact_name
        for vf_module in vnf["vf_modules"]:
            vf_module_label = vf_module["model_name"]
            vf_module["parameters"]["sdnc_model_name"] = sdnc_model_name
            vf_module["parameters"]["sdnc_model_version"] = sdnc_model_version
            vf_module["parameters"]["vf_module_label"] = vf_module_label

        so_vnf = SoServiceVnf.load(vnf)

        vnf_instantiation = service_instance.add_vnf(
            vnf=vnf_model,
            line_of_business=line_of_business,
            platform=platform,
            cloud_region=cloud_region,
            tenant=get_tenant(cloud_region=cloud_region, tenant_name=vnf["tenant_name"]),
            vnf_instance_name=vnf["instance_name"],
            so_vnf=so_vnf,
            a_la_carte=False
        )

        check_orchestration_status(vnf_instantiation)


def instantiate_vf_module(vf_module, vf_module_param_list, vnf_instance, sdnc_model_name, sdnc_model_version):
    vf_module_label = next(vfm_prop.value for vfm_prop in vf_module.properties if vfm_prop.name == "vf_module_label")
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


def check_vf_module_list_correct(vf_modules, vf_modules_config_list):
    model_labels = set()
    config_labels = set()
    for vf_module in vf_modules:
        _model_label = next(_prop.value for _prop in vf_module.properties if _prop.name == "vf_module_label")
        model_labels.add(_model_label)
    for vf_module in vf_modules_config_list:
        config_labels.add(vf_module.model_name)
    if model_labels == config_labels:
        return True
    else:
        return False


def get_properties(vnf):
    properties = dict()

    properties["sdnc_model_name"] = next(prop.value for prop in vnf.properties if prop.name == "sdnc_model_name")
    properties["sdnc_model_version"] = next(prop.value for prop in vnf.properties if prop.name == "sdnc_model_version")
    properties["sdnc_artifact_name"] = next(prop.value for prop in vnf.properties if prop.name == "sdnc_artifact_name")

    return properties


def instantiate_service_alacarte(config, service_subscription, service_model, cloud_region, tenant, customer,
                                 owning_entity,
                                 project, line_of_business, platform):
    raise NotImplementedError("Not supported since 2022")

    service_instance_name = config.service_instance["instance_name"]
    # Service creation
    service_instantiation = ServiceInstantiation.instantiate_ala_carte(
        sdc_service=service_model,
        cloud_region=cloud_region,
        tenant=tenant,
        customer=customer,
        owning_entity=owning_entity,
        project=project,
        service_instance_name=service_instance_name
    )
    check_orchestration_status(service_instantiation)
    # End of service creation

    service_instance = service_subscription.get_service_instance_by_name(service_instance_name)
    # Add VNFs
    for vnf in service_model.vnfs:
        # TODO: priority
        properties = get_properties(vnf)
        vnf_config = next(_vnf for _vnf in config.service_instance["vnfs"]
                          if config.service_instance["vnfs"]["model_name"] == vnf.name)
        vnf_parameters = get_instantiation_parameters(properties, vnf_config)
        #  TODO: instance name
        vnf_instantiation = service_instance.add_vnf(
            vnf=vnf,
            line_of_business=line_of_business,
            platform=platform,
            vnf_parameters=vnf_parameters
        )
        check_orchestration_status(vnf_instantiation)

        # Add vf_modules
        vnf_type = service_model.name + "/" + vnf.name
        vnf_instance = next((vnf for vnf in service_instance.vnf_instances if vnf.vnf_type == vnf_type), None)

        if check_vf_module_list_correct(vnf.vf_modules, vnf_config["vf_modules"]):
            for vf_module in vnf.vf_modules:
                vf_module_config = next(_vf for _vf in vnf_config["vf_modules"]
                                        if _vf["model_name"] == vf_module.properties["vf_module_label"])
                vf_module_parameters = get_instantiation_parameters(properties, vf_module_config)
                vf_module_instantiation = vnf_instance.add_vf_module(
                    vf_module=vf_module,
                    cloud_region=cloud_region,
                    tenant=tenant,
                    vnf_parameters=vf_module_parameters,
                    use_preload=False
                )
                check_orchestration_status(vf_module_instantiation)
        else:
            logger.error("VF_MODULE_PARAM_LIST error. ")
        # End of vf_modules
    # End of VNFs


def main():
    logger.info("*******************************")
    logger.info("**** SERVICE INSTANTIATION ****")
    logger.info("*******************************")

    config = Config(env_dict=VariablesDict.env_variable)

    logger.info("******** GET Customer *******")
    customer = get_customer(config.service_instance["customer_id"])

    logger.info("******** GET Service Model from SDC *******")
    service = get_service_model(config.service_instance["model_name"])

    logger.info("******** Subscribe Customer for Service *******")
    subscribe_service_customer(customer, service)

    logger.info("******** Get Tenant *******")
    region_details = next(
        region for region in config.cloud_regions if region["name"] == config.service_instance["cloud_region"])
    cloud_region = get_cloud_region(region_details["cloud_owner"], region_details["name"])
    tenant = get_tenant(cloud_region,
                        config.service_instance["tenant_name"])

    ######
    logger.info("******** Connect Service to Tenant *******")
    service_subscription = None
    try:
        service_subscription = customer.get_service_subscription_by_service_type(
            service_type=config.service_instance["model_name"])
    except ResourceNotFound:
        logger.error("Service subscription %s is not found", config.service_instance["model_name"])
        exit(1)

    service_subscription.link_to_cloud_region_and_tenant(cloud_region, tenant)
    ####

    logger.info("******** Business Objects (OE, P, Pl, LoB) *******")
    project = "Project-Demonstration"
    platform = "Platform-test"
    line_of_business = config.user_params["company_name"] + "-LOB"
    owning_entity = add_owning_entity(config.user_params["company_name"])

    logger.info("******** Delete old profiles ********")
    delete_old_profiles(service, config.service_instance)

    logger.info("******** Instantiate Service *******")
    service_instance = check_service_instance_exists(service_subscription, config.service_instance["instance_name"])
    if service_instance:
        logger.info("******** Service Instance exists, do not instantiate *******")
    else:
        logger.info("******** Service Instance not existing: Instantiate *******")
        if config.service_model["macro_orchestration"]:
            instantiate_service_macro(config, service, cloud_region, tenant, customer, owning_entity,
                                      project, line_of_business, platform)
            instantiate_vnfs_macro(config=config, service_subscription=service_subscription,
                                   line_of_business=line_of_business, platform=platform)
        else:
            instantiate_service_alacarte(config, service_subscription, service, cloud_region, tenant, customer,
                                         owning_entity, project, line_of_business, platform)


if __name__ == "__main__":
    sh = logging.StreamHandler()
    sh_formatter = logging.Formatter('%(asctime)s %(levelname)s %(lineno)d:%(filename)s(%(process)d) - %(message)s')
    sh.setFormatter(sh_formatter)
    logger.addHandler(sh)

    main()
