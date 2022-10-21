# ============LICENSE_START=======================================================
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
import os
import sys
import zipfile
from io import BytesIO

from onapsdk.aai.business import Customer
from onapsdk.cds.blueprint import Workflow, Blueprint
from config import Config, VariablesDict

#FIXME remove from global scope
logger = logging.getLogger("")
logger.setLevel(logging.INFO)
fh = logging.StreamHandler()
fh_formatter = logging.Formatter('%(asctime)s %(levelname)s %(lineno)d:%(filename)s(%(process)d) - %(message)s')
fh.setFormatter(fh_formatter)
logger.addHandler(fh)


def resolve_inputs(config: Config):
    logger.info("******** Check Customer *******")
    customer_id = config.service_instance["customer_id"]
    customer = Customer.get_by_global_customer_id(customer_id)
    if customer is None:
        raise Exception("Customer %s wasn't found in ONAP" % customer_id)
    logger.info("******** Check Service Subscription *******")
    service_subscription = None
    for service_sub in customer.service_subscriptions:
        logger.debug("Service subscription %s is found", service_sub.service_type)
        if service_sub.service_type == config.service_model["model_name"]:
            logger.info("Service %s subscribed", config.service_model["model_name"])
            service_subscription = service_sub
            break
    logger.info("******** Retrieve Service Metadata *******")
    service_instance = None
    for single_service in service_subscription.service_instances:
        if single_service.instance_name == config.service_instance["instance_name"]:
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

def main(replica_count):
    mypath = os.path.dirname(os.path.realpath(__file__))
    config = Config(env_dict=VariablesDict.env_variable)
    for vnf in config.service_model["vnfs"]:
        file = vnf["vsp"]["vsp_file"]
        file_path = os.path.join(mypath, file)
        with zipfile.ZipFile(file_path, 'r') as package:
            cba_io = BytesIO(package.read("CBA.zip"))
            cba_io.seek(0)
            blueprint = Blueprint(cba_io.read())

            healthcheck: Workflow = blueprint.get_workflow_by_name('scale')
            serv_id, vnf_id = resolve_inputs(config)
            cds_input = {"scale-properties":
                {
                    "service-instance-id": serv_id,
                    "vnf-id": vnf_id,
                    "status-check-max-count": 20,
                    "replica-count": int(replica_count)
                }
            }

            logger.info("Requesting Scale for CBA %s:%s with inputs:\n%s",
                    blueprint.metadata.template_name,
                    blueprint.metadata.template_version,
                    cds_input)
            result = healthcheck.execute(cds_input)
            logger.info("Scale process completed with result: %s", result)
            logger.info("Please check cds-blueprints-processor logs to see exact status")

if __name__ == "__main__":
    replica_count = 1
    if len(sys.argv) > 1:
        replica_count = sys.argv[1]
    print(f"Replica Count: %s" % replica_count)
    main(replica_count)
