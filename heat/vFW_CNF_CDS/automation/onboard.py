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

from instantiate import read_sdnc_model_details
from config import Config
import onapsdk.constants as const

from onapsdk.sdc.vendor import Vendor
from onapsdk.sdc.vsp import Vsp
from onapsdk.sdc.vf import Vf
from onapsdk.sdc.pnf import Pnf
from onapsdk.sdc.service import Service, ServiceInstantiationType

import os

logger = logging.getLogger("")
logger.setLevel(logging.DEBUG)
fh = logging.StreamHandler()
fh_formatter = logging.Formatter('%(asctime)s %(levelname)s %(lineno)d:%(filename)s(%(process)d) - %(message)s')
fh.setFormatter(fh_formatter)
logger.addHandler(fh)


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


def onboard_pnf(pnf_name, vsp_name, vsp_file, vendor):
    logger.info(f"******** Onboard PNF - {pnf_name} *******")
    pnf_vsp = onboard_vsp(vsp_name=vsp_name, vsp_file=vsp_file, vendor=vendor)
    pnf = Pnf(name=pnf_name, vsp=pnf_vsp)
    pnf.onboard()
    return pnf


def onboard_vnf(vnf_name, vsp_name, vsp_file, vendor):
    logger.info(f"******** Onboard VNF - {vnf_name} *******")
    vnf_vsp = onboard_vsp(vsp_name=vsp_name, vsp_file=vsp_file, vendor=vendor)
    vnf = Vf(name=vnf_name, vsp=vnf_vsp)
    vnf.create()
    vnf.onboard()
    return vnf


def create_service(service_name, is_macro: bool = True):
    logger.info("******** Create Service *******")
    if is_macro:
        svc = Service(name=service_name,
                      instantiation_type=ServiceInstantiationType.MACRO)
    else:
        svc = Service(name=service_name,
                      instantiation_type=ServiceInstantiationType.A_LA_CARTE)
    svc.create()
    return svc


def set_properties(service, xnf):
    sdnc_model_name, sdnc_model_version = read_sdnc_model_details(Config.VSPFILE)
    if service.status == const.DRAFT:
        logger.info("******** Set SDNC properties for VF ********")
        component = service.get_component(xnf)
        prop = component.get_property("sdnc_model_version")
        prop.value = sdnc_model_version
        prop = component.get_property("sdnc_artifact_name")
        prop.value = Config.SDNC_ARTIFACT_NAME
        prop = component.get_property("sdnc_model_name")
        prop.value = sdnc_model_name
        prop = component.get_property("controller_actor")
        prop.value = "CDS"
        prop = component.get_property("skip_post_instantiation_configuration")
        prop.value = Config.SKIP_POST_INSTANTIATION


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
    retrieve_service(service_name=Config.SERVICENAME)
    logger.info("******** SERVICE DESIGN *******")
    vendor = onboard_vendor(vendor_name=Config.VENDOR)
    service = create_service(Config.SERVICENAME, Config.MACRO_INSTANTIATION)
    if Config.ADD_PNF:
        pnf = onboard_pnf(pnf_name=Config.PNF_NAME,
                          vsp_name=Config.PNF_VSP_NAME,
                          vsp_file=Config.PNF_VSP_FILE,
                          vendor=vendor)
        service.add_resource(pnf)
    vnf = onboard_vnf(vnf_name=Config.VFNAME,
                      vsp_name=Config.VSPNAME,
                      vsp_file=Config.VSPFILE,
                      vendor=vendor)
    service.add_resource(vnf)
    set_properties(service, vnf)
    service.checkin()
    service.onboard()
    check_distribution_status(service)


if __name__ == "__main__":
    main()
