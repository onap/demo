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

import time
import zipfile
from io import BytesIO

import oyaml as yaml

from config import Config
import onapsdk.constants as const

from onapsdk.sdc.vendor import Vendor
from onapsdk.sdc.vsp import Vsp
from onapsdk.sdc.vf import Vf
from onapsdk.sdc.service import Service, ServiceInstantiationType

import os

logger = logging.getLogger("")
logger.setLevel(logging.DEBUG)
fh = logging.StreamHandler()
fh_formatter = logging.Formatter('%(asctime)s %(levelname)s %(lineno)d:%(filename)s(%(process)d) - %(message)s')
fh.setFormatter(fh_formatter)
logger.addHandler(fh)

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

logger.info("*******************************")
logger.info("******** SERVICE DESIGN *******")
logger.info("*******************************")

logger.info("******** Onboard Vendor *******")
vendor = Vendor(name=Config.VENDOR)
vendor.onboard()

logger.info("******** Onboard VSP *******")
mypath = os.path.dirname(os.path.realpath(__file__))
myvspfile = os.path.join(mypath, Config.VSPFILE)
vsp = Vsp(name=Config.VSPNAME, vendor=vendor, package=open(myvspfile, 'rb'))
vsp.onboard()

logger.info("******** Onboard VF *******")
vf = Vf(name=Config.VFNAME)
vf.vsp = vsp
vf.create()
vf.onboard()

logger.info("******** Onboard Service *******")
svc = Service(name=Config.SERVICENAME,
              instantiation_type=ServiceInstantiationType.MACRO)
svc.create()

if svc.status == const.DRAFT:
    svc.add_resource(vf)

    logger.info("******** Set SDNC properties for VF ********")
    component = svc.get_component(vf)
    prop = component.get_property("sdnc_model_version")
    prop.value = SDNC_MODEL_VERSION
    prop = component.get_property("sdnc_artifact_name")
    prop.value = Config.SDNC_ARTIFACT_NAME
    prop = component.get_property("sdnc_model_name")
    prop.value = SDNC_MODEL_NAME
    prop = component.get_property("controller_actor")
    prop.value = "CDS"
    prop = component.get_property("skip_post_instantiation_configuration")
    prop.value = Config.SKIP_POST_INSTANTIATION

    logger.info("******** Onboard Service *******")
    svc.checkin()
    svc.onboard()

logger.info("******** Check Service Distribution *******")
distribution_completed = False
nb_try = 0
nb_try_max = 10
while distribution_completed is False and nb_try < nb_try_max:
    distribution_completed = svc.distributed
    if distribution_completed is True:
        logger.info("Service Distribution for %s is successfully finished", svc.name)
        break
    logger.info("Service Distribution for %s ongoing, Wait for 60 s", svc.name)
    time.sleep(60)
    nb_try += 1

if distribution_completed is False:
    logger.error("Service Distribution for %s failed !!", svc.name)
    exit(1)
