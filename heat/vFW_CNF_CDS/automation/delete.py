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

from config import Config

from instantiate import get_customer, check_orchestration_status, get_service_model

logger = logging.getLogger("")
logger.setLevel(logging.DEBUG)
fh = logging.StreamHandler()
fh_formatter = logging.Formatter('%(asctime)s %(levelname)s %(lineno)d:%(filename)s(%(process)d) - %(message)s')
fh.setFormatter(fh_formatter)
logger.addHandler(fh)


def get_service_subscription(customer, service_name):
    try:
        service_subscription = next(
            service_sub for service_sub in customer.service_subscriptions if service_sub.service_type == service_name)
    except StopIteration:
        logger.error("Service Subscription not found")
        exit(1)
    return service_subscription


def get_service_instance(service_subscription, service_instance_name):
    try:
        service_instance = next(instance for instance in service_subscription.service_instances
                                if instance.instance_name == service_instance_name)
    except StopIteration:
        logger.error("Service Instance not found")
        exit(1)
    return service_instance


def delete_service_macro(service_instance):
    service_deletion = service_instance.delete(a_la_carte=False)
    check_orchestration_status(service_deletion)


def delete_service_alacarte(service_instance):
    for vnf in service_instance.vnf_instances:
        for vf_module in vnf.vf_modules:
            vf_module_deletion = vf_module.delete()
            check_orchestration_status(vf_module_deletion)
        vnf_deletion = vnf.delete()
        check_orchestration_status(vnf_deletion)
    service_deletion = service_instance.delete()
    check_orchestration_status(service_deletion)


def delete_service_alacarte2(service_instance, service_model):
    for vnf in service_instance.vnf_instances:
        for label in sorted(Config.VF_MODULE_PARAM_LIST, reverse=True):
            vf_module_model = next(vfmodule for vfmodule in service_model.vf_modules
                                   if vfmodule.properties["vf_module_label"] == label)
            vfModuleModelInvariantUUID = vf_module_model.metadata["vfModuleModelInvariantUUID"]
            vf_module = next(vfmodule for vfmodule in vnf.vf_modules if vfmodule.model_invariant_id == vfModuleModelInvariantUUID)
            vf_module_deletion = vf_module.delete()
            check_orchestration_status(vf_module_deletion)

        vnf_deletion = vnf.delete()
        check_orchestration_status(vnf_deletion)
    service_deletion = service_instance.delete()
    check_orchestration_status(service_deletion)


def main():
    logger.info("*******************************")
    logger.info("**** SERVICE DELETION ****")
    logger.info("*******************************")

    logger.info("******** GET Customer *******")
    customer = get_customer(Config.GLOBAL_CUSTOMER_ID)

    logger.info("******** Check Service Subscription *******")
    service_subscription = get_service_subscription(customer, Config.SERVICENAME)

    logger.info("******** Get Service Instance details *******")
    service_instance = get_service_instance(service_subscription, Config.SERVICE_INSTANCE_NAME)

    service_model = get_service_model(Config.SERVICENAME)

    logger.info("******** Delete Service %s *******", service_instance.instance_name)
    if Config.MACRO_INSTANTIATION:
        delete_service_macro(service_instance)
    else:
        delete_service_alacarte2(service_instance, service_model)


if __name__ == "__main__":
    main()
