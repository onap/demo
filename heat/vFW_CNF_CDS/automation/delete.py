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
from time import sleep

from onapsdk.aai.business import Customer
from onapsdk.so.so_element import OrchestrationRequest

from config import Config

logger = logging.getLogger("")
logger.setLevel(logging.DEBUG)
fh = logging.StreamHandler()
fh_formatter = logging.Formatter('%(asctime)s %(levelname)s %(lineno)d:%(filename)s(%(process)d) - %(message)s')
fh.setFormatter(fh_formatter)
logger.addHandler(fh)

logger.info("******** Get Customer *******")
customer = None
try:
    customer = Customer.get_by_global_customer_id(Config.GLOBAL_CUSTOMER_ID)
except:
    logger.error("Customer not found")
    exit(1)

logger.info("******** Check Service Subscription *******")
service_subscription = None
for service_sub in customer.service_subscriptions:
    if service_sub.service_type == Config.SERVICENAME:
        logger.info("Service %s subscribed", Config.SERVICENAME)
        service_subscription = service_sub
        break
if not service_subscription:
    logger.error("Service Subscription not found")
    exit(1)

logger.info("******** Get Service Instance details *******")
service_instance = None
for service in service_subscription.service_instances:
    if service.instance_name == Config.SERVICE_INSTANCE_NAME:
        service_instance = service
        break
if not service_instance:
    logger.error("Service Instance not found")
    exit(1)

logger.info("******** Delete Service %s *******", service_instance.instance_name)
service_deletion = service_instance.delete()
status = None
while not (status == OrchestrationRequest.StatusEnum.COMPLETED
           or status == OrchestrationRequest.StatusEnum.FAILED):
    sleep(10)
    status = service_deletion.status
    logger.info(f"Orchestration status is: {status.value}")
