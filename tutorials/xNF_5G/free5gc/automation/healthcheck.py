# ============LICENSE_START=======================================================
# Copyright (C) 2021 Samsung
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

from onapsdk.aai.business import Customer
from onapsdk.cds.blueprint import Workflow, Blueprint

from config import Config

#FIXME remove from global scope
logger = logging.getLogger("")
logger.setLevel(logging.INFO)
fh = logging.StreamHandler()
fh_formatter = logging.Formatter('%(asctime)s %(levelname)s %(lineno)d:%(filename)s(%(process)d) - %(message)s')
fh.setFormatter(fh_formatter)
logger.addHandler(fh)

def resolve_hc_inputs():
    logger.info("******** Check Customer *******")
    customer = None
    for found_customer in list(Customer.get_all()):
        logger.debug("Customer %s found", found_customer.subscriber_name)
        if found_customer.subscriber_name == Config.GLOBAL_CUSTOMER_ID:
            logger.info("Customer %s found", found_customer.subscriber_name)
            customer = found_customer
            break
    if customer is None:
        raise Exception("Customer %s wasn't found in ONAP" % Config.GLOBAL_CUSTOMER_ID)
    logger.info("******** Check Service Subscription *******")
    service_subscription = None
    for service_sub in customer.service_subscriptions:
        logger.debug("Service subscription %s is found", service_sub.service_type)
        if service_sub.service_type == Config.SERVICENAME:
            logger.info("Service %s subscribed", Config.SERVICENAME)
            service_subscription = service_sub
            break
    logger.info("******** Retrieve Service Metadata *******")
    service_instance = None
    for single_service in service_subscription.service_instances:
        if single_service.instance_name == Config.SERVICE_INSTANCE_NAME:
            service_instance = single_service
            break
    service_id = service_instance.instance_id
    vnfs = list(service_instance.vnf_instances)
    if len(vnfs) > 1:
        raise NotImplementedError("Service %s is composed of more than one vnf!" % service_id)
    if not vnfs:
        raise Exception("Service %s doesn't contain any vnfs" % service_id)
    vnf_id = vnfs[0].vnf_id
    return service_id, vnf_id

def main():
    blueprint = None
    with zipfile.ZipFile(Config.VSPFILE, 'r') as package:
        with package.open("CBA.zip", 'r') as cba:
            blueprint = Blueprint(cba.read())

    healthcheck = Workflow('health-check', None, blueprint)
    serv_id, vnf_id = resolve_hc_inputs()
    cds_input = {"health-check-properties":
        {
            "service-instance-id": serv_id,
            "vnf-id": vnf_id
        }
    }
    logger.info("Requesting Healthcheck for CBA %s:%s with inputs:\n%s",
            blueprint.metadata.template_name,
            blueprint.metadata.template_version,
            cds_input)
    result = healthcheck.execute(cds_input)
    logger.info("Healthcheck process completed with result: %s", result)
    logger.info("Please check cds-blueprints-processor logs to see exact status")

if __name__ == "__main__":
    main()
