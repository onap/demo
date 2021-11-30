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

from instantiate import get_customer, check_orchestration_status

logger = logging.getLogger()
logger.setLevel(logging.DEBUG)


def get_service_subscription(customer, service_type):
    try:
        service_subscription = next(
            service_sub for service_sub in customer.service_subscriptions if service_sub.service_type == service_type)
        return service_subscription
    except StopIteration:
        logger.error("Service Subscription not found")
        exit(1)


def get_service_instance(service_subscription, service_instance_name):
    try:
        service_instance = next(instance for instance in service_subscription.service_instances
                                if instance.instance_name == service_instance_name)
        return service_instance
    except StopIteration:
        logger.error("Service Instance not found")
        exit(1)


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
    service_deletion = service_instance.delete(a_la_carte=True)
    check_orchestration_status(service_deletion)


def main():
    logger.info("*******************************")
    logger.info("**** SERVICE DELETION ****")
    logger.info("*******************************")

    config = Config()
    logger.info("******** GET Customer *******")
    customer = get_customer(config.service_instance["customer_id"])

    logger.info("******** Check Service Subscription *******")
    service_subscription = get_service_subscription(customer, config.service_instance["model_name"])

    logger.info("******** Get Service Instance details *******")
    service_instance = get_service_instance(service_subscription, config.service_instance["instance_name"])

    logger.info("******** Delete Service %s *******", service_instance.instance_name)
    if config.service_model["macro_orchestration"]:
        delete_service_macro(service_instance)
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
