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
import os
import time
import zipfile
from io import BytesIO

import yaml

from config import Config, VariablesDict
import onapsdk.constants as const

from onapsdk.sdc.vendor import Vendor
from onapsdk.sdc.vsp import Vsp
from onapsdk.sdc.vf import Vf
from onapsdk.sdc.pnf import Pnf
from onapsdk.sdc.service import Service, ServiceInstantiationType
from onapsdk.exceptions import ResourceNotFound

logger = logging.getLogger()
logger.setLevel(logging.DEBUG)


def retrieve_service(service_name: str):
    logger.info("Retrieve service from SDC before onboarding")
    services = Service.get_all()

    for found_service in services:
        if found_service.name == service_name:
            logging.info(f"Service {found_service.name} found in SDC, onboarding will not be executed")
            exit(0)
    return


def onboard_vendor(vendor_name: str = "demo_vendor"):
    logger.info("******** Onboard Vendor *******")
    try:
        vendor = next(vendor for vendor in Vendor.get_all() if vendor.name.lower() == vendor_name.lower())
    except (StopIteration, ResourceNotFound):
        vendor = Vendor(vendor_name)
        vendor.onboard()
    return vendor


def onboard_vsp(vsp_name, vsp_file, vendor):
    logger.info(f"******** Onboard VSP - {vsp_name} *******")
    mypath = os.path.dirname(os.path.realpath(__file__))
    vsp_path = os.path.join(mypath, vsp_file)
    vsp = None
    try:
        vsp = Vsp(name=vsp_name, vendor=vendor, package=open(vsp_path, 'rb'))
    except FileNotFoundError:
        logger.error(f"No vsp file was found for {vsp_name}!")
        exit(1)
    vsp.onboard()
    return vsp


def onboard_pnf(pnf_name, vsp_name, vsp_file, vendor_name):
    logger.info(f"******** Onboard PNF - {pnf_name} *******")
    vendor = onboard_vendor(vendor_name=vendor_name)
    try:
        pnf = next(_pnf for _pnf in Pnf.get_all() if _pnf.name == pnf_name)
        logger.info("PNF with provided name exists in ONAP SDC, onboarding will not be executed")
    except (StopIteration, ResourceNotFound):
        pnf_vsp = onboard_vsp(vsp_name=vsp_name, vsp_file=vsp_file, vendor=vendor)
        pnf = Pnf(name=pnf_name, vsp=pnf_vsp)
        pnf.onboard()
    return pnf


def onboard_vnf(vnf_name, vsp_name, vsp_file, vendor_name):
    logger.info(f"******** Onboard VNF - {vnf_name} *******")
    vendor = onboard_vendor(vendor_name=vendor_name)
    try:
        vnf = next(_vf for _vf in Vf.get_all() if _vf.name == vnf_name)
        if vnf.status != "Certified":
            logger.error("Selected VNF is not certified. Try Onboard VF with new name.")
            exit(1)
        logger.info("VNF with provided name exists in ONAP SDC, onboarding will not  be executed")
    except (StopIteration, ResourceNotFound):
        vnf_vsp = onboard_vsp(vsp_name=vsp_name, vsp_file=vsp_file, vendor=vendor)
        vnf = Vf(name=vnf_name, vsp=vnf_vsp)
        vnf.create()
        vnf.onboard()
    return vnf


def create_service(service_name, is_macro: bool = True):
    logger.info("******** Create Service *******")
    if is_macro:
        service = Service(name=service_name,
                          instantiation_type=ServiceInstantiationType.MACRO)
    else:
        service = Service(name=service_name,
                          instantiation_type=ServiceInstantiationType.A_LA_CARTE)
    service.create()
    return service


def read_sdnc_model_details(file):
    mypath = os.path.dirname(os.path.realpath(__file__))
    file_path = os.path.join(mypath, file)
    try:
        with zipfile.ZipFile(file_path, 'r') as package:
            try:
                cba_io = BytesIO(package.read("CBA.zip"))
                with zipfile.ZipFile(cba_io) as cba:
                    with cba.open('TOSCA-Metadata/TOSCA.meta') as meta_file:
                        tosca_meta = yaml.load(meta_file, Loader=yaml.SafeLoader)
                        sdnc_model_name = tosca_meta.get("Template-Name")
                        sdnc_model_version = tosca_meta.get("Template-Version")
                        return sdnc_model_name, sdnc_model_version
            except KeyError:
                logger.info("No CBA file was found")
                return None, None
    except FileNotFoundError:
        logger.error("No vsp file was found!")
        exit(1)


def set_properties(service, xnf, vsp_details):
    sdnc_model_name, sdnc_model_version = read_sdnc_model_details(vsp_details["vsp_file"])
    if sdnc_model_name and sdnc_model_version:
        if service.status == const.DRAFT:
            logger.info("******** Set SDNC properties for VF ********")
            component = service.get_component(xnf)
            prop = component.get_property("sdnc_model_name")
            prop.value = sdnc_model_name
            prop = component.get_property("sdnc_model_version")
            prop.value = sdnc_model_version
            prop = component.get_property("controller_actor")
            prop.value = "CDS"
            prop = component.get_property("sdnc_artifact_name")
            prop.value = vsp_details["sdnc_artifact_name"]
            prop = component.get_property("skip_post_instantiation_configuration")
            prop.value = vsp_details["skip_post_instantiation_configuration"]


def check_distribution_status(service):
    logger.info("******** Check Service Distribution *******")
    distribution_completed = False
    nb_try = 0
    nb_try_max = 10
    while distribution_completed is False and nb_try < nb_try_max:
        distribution_completed = service.distributed
        if distribution_completed is True:
            logger.info(f"Service Distribution for {service.name} is successfully finished")
            break
        logger.info(f"Service Distribution for {service.name} ongoing, Wait for 60 s")
        time.sleep(60)
        nb_try += 1

    if distribution_completed is False:
        logger.error(f"Service Distribution for {service.name} failed !!", )
        exit(1)


def main():
    config = Config(env_dict=VariablesDict.env_variable)
    retrieve_service(service_name=config.service_model["model_name"])

    logger.info("******** SERVICE DESIGN *******")
    service = create_service(service_name=config.service_model["model_name"],
                             is_macro=config.service_model["macro_orchestration"])
    vnfs = config.service_model.get("vnfs")
    if vnfs:
        for vnf in vnfs:
            new_vnf = onboard_vnf(vnf_name=vnf["model_name"],
                                  vsp_name="VSP" + "_" + vnf["model_name"],
                                  vsp_file=vnf["vsp"]["vsp_file"],
                                  vendor_name=vnf["vsp"]["vendor"])
            service.add_resource(new_vnf)
            set_properties(service=service, xnf=new_vnf, vsp_details=vnf["vsp"])

    pnfs = config.service_model.get("pnfs")
    if pnfs:
        for pnf in pnfs:
            new_pnf = onboard_pnf(pnf_name=pnf["model_name"],
                                  vsp_name="VSP" + "_" + pnf["model_name"],
                                  vsp_file=pnf["vsp"]["vsp_file"],
                                  vendor_name=pnf["vsp"]["vendor"])
            service.add_resource(new_pnf)
            set_properties(service=service, xnf=new_pnf, vsp_details=pnf["vsp"])

    service.checkin()
    service.onboard()
    check_distribution_status(service)


if __name__ == "__main__":
    sh = logging.StreamHandler()
    sh_formatter = logging.Formatter('%(asctime)s %(levelname)s %(lineno)d:%(filename)s(%(process)d) - %(message)s')
    sh.setFormatter(sh_formatter)
    logger.addHandler(sh)

    main()
