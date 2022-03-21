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

from config import Config, VariablesDict
from instantiate import get_customer, check_orchestration_status

from onapsdk.exceptions import ResourceNotFound, APIError
from onapsdk.aai.business import ServiceInstance

logger = logging.getLogger()
logger.setLevel(logging.DEBUG)


def get_service_subscription(customer, service_type):
    try:
        service_subscription = customer.get_service_subscription_by_service_type(
            service_type=service_type)
        return service_subscription
    except ResourceNotFound:
        logger.error("Service Subscription not found")
        exit(1)


def get_service_instance(service_subscription, service_instance_name):
    try:
        service_instance = service_subscription.get_service_instance_by_name(
            service_instance_name=service_instance_name)
        return service_instance
    except ResourceNotFound:
        logger.error("Service Instance not found")
        exit(1)


def delete_service_macro(service_instance: ServiceInstance, service_instance_info):
    vnf_infos_list = []
    for index, vnf_info in enumerate(service_instance_info["vnfs"]):
        if not vnf_info.get("vnf_name_suffix"):
            vnf_info["vnf_name_suffix"] = str(index)
        vnf_info["instance_name"] = f"Instance_{vnf_info['model_name']}_{vnf_info['vnf_name_suffix']}"
        vnf_infos_list.append(vnf_info)

    ordered_vnf_instances = sorted(vnf_infos_list,
                                   key=lambda _vnf: _vnf.get("processing_priority", 100),
                                   reverse=True)

    delete_vnfs_instances(service_instance, ordered_vnf_instances)

    return check_orchestration_status(service_instance.delete(a_la_carte=False))


def delete_vnfs_instances(service_instance: ServiceInstance, ordered_vnf_instances):
    for vnf in ordered_vnf_instances:
        vnf_name = vnf.get("instance_name")
        vnf_instance = next((vnf_instance
                             for vnf_instance in service_instance.vnf_instances
                             if vnf_instance.vnf_name == vnf_name), None)
        if not vnf_instance:
            continue

        try:
            vnf_deletion = vnf_instance.delete(a_la_carte=False)
        except APIError:
            logger.error("Operation not supported, whole service instance will be deleted with random order")
            break
        check_orchestration_status(vnf_deletion)
    return


def delete_service_alacarte(service_instance):
    for vnf in service_instance.vnf_instances:
        for vf_module in vnf.vf_modules:
            vf_module_deletion = vf_module.delete()
            check_orchestration_status(vf_module_deletion)
        vnf_deletion = vnf.delete()
        check_orchestration_status(vnf_deletion)
    service_deletion = service_instance.delete(a_la_carte=True)
    check_orchestration_status(service_deletion)


def main():
    logger.info("*******************************")
    logger.info("**** SERVICE DELETION ****")
    logger.info("*******************************")

    config = Config(env_dict=VariablesDict.env_variable)
    logger.info("******** GET Customer *******")
    customer = get_customer(config.service_instance["customer_id"])

    logger.info("******** Check Service Subscription *******")
    service_subscription = get_service_subscription(customer, config.service_instance["model_name"])

    logger.info("******** Get Service Instance details *******")
    service_instance = get_service_instance(service_subscription, config.service_instance["instance_name"])

    logger.info("******** Delete Service %s *******", service_instance.instance_name)
    if config.service_model["macro_orchestration"]:
        # if config.service_instance.get("deletion_policy") in ('InstantiationOrder', 'ReverseInstantiationOrder'):
        #     delete_service_macro_delete_policy(service_instance, config.service_instance)
        # else:
        delete_service_macro(service_instance, config.service_instance)
    else:
        logger.error("A_la_carte orchestration method not updated")
        if config.service_model["pnfs"] is not None:
            raise NotImplementedError
        else:
            delete_service_alacarte(service_instance)


if __name__ == "__main__":
    sh = logging.StreamHandler()
    sh_formatter = logging.Formatter('%(asctime)s %(levelname)s %(lineno)d:%(filename)s(%(process)d) - %(message)s')
    sh.setFormatter(sh_formatter)
    logger.addHandler(sh)

    main()
