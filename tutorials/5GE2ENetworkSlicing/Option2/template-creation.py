#!/usr/bin/env python3

# ============LICENSE_START===================================================
#  Copyright (C) 2021 Samsung Electronics
# ============================================================================
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
# SPDX-License-Identifier: Apache-2.0
# ============LICENSE_END=====================================================

from onapsdk.sdc.category_management import ServiceCategory
from onapsdk.sdc.service import Service
from onapsdk.sdc.properties import Property, ParameterError
from onapsdk.sdc.vf import Vf
from onapsdk.sdc.vsp import Vsp
from onapsdk.sdc.vfc import Vfc
from onapsdk.sdc.vendor import Vendor

from time import sleep
import onapsdk.constants as const
from onapsdk.exceptions import APIError, ResourceNotFound
import logging

vsp = Vsp("test1")
vsp.create()

logger = logging.getLogger("")
logger.setLevel(logging.DEBUG)
fh = logging.StreamHandler()
fh_formatter = logging.Formatter('%(asctime)s %(levelname)s %(lineno)d:%(filename)s(%(process)d) - %(message)s')
fh.setFormatter(fh_formatter)
logger.addHandler(fh)

SUFFIX = ''

def create_name(name, suffix=SUFFIX):
    """ helper function to create uniqe name by appending
        predefined suffix, may be helpful during testing
        as ONAP does not allow for easy deeltion of services"""
    return name + suffix


def create_service_category(category_names):
    for cn in category_names:
        logger.info('creating service category [%s]', cn)
        ServiceCategory.create(name=cn)


def create_vendor(vendor_name):
    vendor = Vendor(vendor_name)
    vendor.create()
    try:
        vendor.onboard()
    except APIError as e:
        logger.warn("Exception during vendor onboarding, ", e)
        raise e

    return vendor


def create_vsp(name, vendor, onboard=False):
    logger.info("creating vsp: [%s:%s]", name, vendor)
    retry = 0
    done = False

    vsp = Vsp(name=name, vendor=vendor)
    if onboard:
        while not done:
            try:
                vsp.create()
                vsp.onboard()
            except ResourceNotFound as e:
                logger.warn(f"Failed to onboard {name}", e)
                retry = retry + 1
                if retry >= 5:
                    raise e
            except APIError as e:
                logger.warn("Exception during vsp onboarding, ", e)
                raise e
            else:
                done = True

    return vsp


def create_vf(name, category, vendor, onboard=False):
    logger.info("create vf: [%s:%s]", name, category)

    vfc = Vfc('AllottedResource')  # seemd incorrect
    vf = Vf(name=name, category=category, vendor=vendor)
    vf.create()
    if vf.status == const.DRAFT:
        vf.add_resource(vfc)
        if onboard:
            onboard_vf(vf)
    return vf


def onboard_vf(vf):
    retry = 0
    done = False
    to = 2

    while not done:
        try:
            vf.onboard()
        except ResourceNotFound as e:
            retry += 1
            if retry > 5:
                raise e
            sleep(to)
            to = 2 * to + 1
        else:
            done = True
    logger.info("onboarded vf: [%s]", vf.name)


def create_service(name, category, vnfs=[], properties=[], role=None, service_type=None):
    logger.info("create service: [%s:%s]", name, category)
    retry = 0
    done = False

    srvc = Service(name=name, category=category, properties=properties, role=role, service_type=service_type)
    srvc.create()

    while not done:
        try:
            if srvc.status == const.DRAFT:
                for vnf in vnfs:
                    srvc.add_resource(vnf)

            if srvc.status != const.DISTRIBUTED:
                srvc.onboard()
        except ResourceNotFound as e:
            retry += 1
            if retry > 5:
                raise e
        else:
            done = True

    return srvc

def create_service_1(name, category, vnfs=[], properties=[], role=None, service_type=None):
    logger.info("create service: [%s:%s]", name, category)
    retry = 0
    done = False

    srvc = Service(name=name, category=category, properties=properties, role=role, service_type=service_type)
    srvc.create()

    while not done:
        try:
            if srvc.status == const.DRAFT:
                for vnf in vnfs:
                    srvc.add_resource(vnf)
                    for c in srvc.components:
                        if c.name == 'SLICE_AR 0':
                            cp = get_component_property(c, 'allottedresource0_providing_service_invariant_uuid')
                            if cp:
                                logger.info('setting value on property [%s]', cp)
                                cp.value = "{\\\"get_input\\\":\\\"slice_ar0_allottedresource0_providing_service_invariant_uuid\\\"}"
                            else:
                                raise ParameterError('no property providing_service_invariant_uuid found')

                            cp = get_component_property(c, 'allottedresource0_providing_service_uuid')
                            if cp:
                                cp.value = "{\\\"get_input\\\":\\\"slice_ar0_allottedresource0_providing_service_uuid\\\"}"
                            else:
                                raise ParameterError('no property providing_service_uuid found')

                    break
            if srvc.status != const.DISTRIBUTED:
                srvc.onboard()
        except ResourceNotFound as e:
            retry += 1
            if retry > 5:
                raise e
        else:
            done = True

    return srvc


def get_component_property(component, name):
    prop = None
    try:
        prop = list(filter(lambda x: x.name == name, component.properties))
        if prop:
            prop = prop[0]
        else:
            raise ParameterError('no property found')
    except ParameterError as e:
        logger.warn("component [%s] has no property [%s]", component.name, name)
        raise e

    logger.info("retrived property [%s] for component [%s]", prop.name if prop else 'null', component.name)

    return prop


vendor = create_vendor('aaaa')
vsp = create_vsp('test1', vendor)

# create custom categories
create_service_category(['CST', 'ServiceProfile', 'AN SliceProfile', 'AN NF Slice Profile',
                         'CN SliceProfile', 'TN SliceProfile', 'NST', 'TN BH NSST', 'Alloted Resource',
                         'TN MH NSST', 'TN FH NSST', 'TN Network Requirement', 'AN NF NSST', 'CN NSST', 'AN NSST'])

props = [Property('ConnectionLink', 'string', value='{\\"get_input\\":\\"ConnectionLink\\"}'),
         Property('jitter', 'string', value='10'),
         Property('latency', 'integer', value=10),
         Property('maxBandwith', 'integer', value=1000)]

srv = create_service(create_name('TN_Network_Requirement'), 'TN Network Requirement', properties=props)
vf = create_vf(create_name('TN_Network_Req_AR'), 'Alloted Resource', vendor)
for c in vf.components:
    if c.name == 'AllottedResource 0':
        c.get_property('providing_service_invariant_uuid').value = srv.unique_uuid
        c.get_property('providing_service_uuid').value = srv.identifier
        c.get_property('providing_service_name').value = srv.name
        break
onboard_vf(vf)

props = [Property('pLMNIdList', 'string', value='39-00'),
        Property('jitter', 'string', value='10'),
        Property('latency', 'integer', value=10),
        Property('maxBandwith', 'integer', value=1000)]

# 3
srv = create_service(create_name('Tn_ONAP_internal_BH'), 'TN BH NSST', vnfs=[vf], role='ONAP_internal',
                    properties=props)
# 6
vf_tn_bh_ar = create_vf(create_name('Tn_BH_AR'), 'Alloted Resource', vendor)
for c in vf_tn_bh_ar.components:
    if c.name == 'AllottedResource 0':
        c.get_property('providing_service_invariant_uuid').value = srv.unique_uuid
        c.get_property('providing_service_uuid').value = srv.identifier
        c.get_property('providing_service_name').value = srv.name
        break
onboard_vf(vf_tn_bh_ar)

# 4
props = [Property('anNSSCap', 'org.openecomp.datatypes.NSSCapabilities')]
srv = create_service(create_name('EmbbAn_NF'), 'AN NF NSST', role='huawei', service_type='embb', properties=props)
# 7
vf_embban_nf_ar = create_vf(create_name('EmbbAn_NF_AR'), 'Alloted Resource', vendor)
for c in vf_embban_nf_ar.components:
    if c.name == 'AllottedResource 0':
        c.get_property('providing_service_invariant_uuid').value = srv.unique_uuid
        c.get_property('providing_service_uuid').value = srv.identifier
        c.get_property('providing_service_name').value = srv.name
        break

onboard_vf(vf_embban_nf_ar)

# 5
props = [Property('activityFactor', 'org.openecomp.datatypes.NSSCapabilities', '30'),
        Property('areaTrafficCapDL', 'org.openecomp.datatypes.NSSCapabilities', '800'),
        Property('areaTrafficCapUL', 'org.openecomp.datatypes.NSSCapabilities', '800'),
        Property('expDataRateDL', 'org.openecomp.datatypes.NSSCapabilities', '1000'),
        Property('expDataReateUL', 'org.openecomp.datatypes.NSSCapabilities', '1000')]

srv = Service(name=create_name('EmbbCn_External'),
            category='CN NSST',
            role='huawei',
            service_type='embb',
            properties=[Property('aname', 'org.openecomp.datatypes.NSSCapabilities',
                                value="[{\\\"latency\\\":35,\\\"maxNumberofUEs\\\":1005,\\\"resourceSharingLevel\\\":\\\"Shared\\\",\\\"sST\\\":\\\"eMBB\\\",\\\"activityFactor\\\":30,\\\"areaTrafficCapDL\\\":800,\\\"areaTrafficCapUL\\\":1000}]")])

srv.create()

if srv.status == const.DRAFT:
    srv.add_deployment_artifact(artifact_type="WORKFLOW", artifact_name="eMBB.zip", artifact="./eMBB.zip",
                                artifact_label="abc")

if srv.status != const.DISTRIBUTED:
    done = False
    retry = 0
    to = 1
    while not done:
        try:
            srv.onboard()
        except ResourceNotFound as e:
            retry += 1
            if retry > 5:
                raise e
            to = 2 * to + 1
            sleep(to)
        else:
            done = True


# 8
vf_embbcn_external_ar = create_vf(create_name('EmbbCn_External_AR'), 'Alloted Resource', vendor)
for c in vf_embbcn_external_ar.components:
    if c.name == 'AllottedResource 0':
        c.get_property('providing_service_invariant_uuid').value = srv.unique_uuid
        c.get_property('providing_service_uuid').value = srv.identifier
        c.get_property('providing_service_name').value = srv.name
        break
onboard_vf(vf_embbcn_external_ar)

logger.info("create service EmbbNst_O2")
# 9
srv = create_service(create_name('EmbbNst_O2'),
                     'NST',
                     role='option2',
                     vnfs = [vf_embbcn_external_ar, vf_embban_nf_ar, vf_tn_bh_ar],
                     properties=[Property('aname', 'org.openecomp.datatypes.NSSCapabilities',
                                value="[{\\\"latency\\\":20,\
                                            \\\"maxNumberofUEs\\\":1000,\
                                            \\\"maxNumberofConns\\\":100000,\
                                            \\\"resourceSharingLevel\\\":\\\"Shared\\\",\
                                            \\\"sST\\\":\\\"eMBB\\\",\
                                            \\\"activityFactor\\\":60,\
                                            \\\"availability\\\":0.6,\
                                            \\\"dLThptPerSlice\\\":1000,\
                                            \\\"uLThptPerSlice\\\":1000,\
                                            \\\"jitter\\\":10,\
                                            \\\"survivalTime\\\":10,\
                                            \\\"ueMobilityLevel\\\":\\\"stationary\\\",\
                                            \\\"pLMNIdList\\\":\\\"39-00\\\",\
                                            \\\"reliability\\\":\\\"99%\\\"}]")])

# 10
logger.info("create Slice_AR")
vf = create_vf(create_name('Slice_AR'), 'Allotted Resource', vendor)
for c in vf.components:
    if c.name == 'AllottedResource 0':
        cp = get_component_property(c, 'providing_service_invariant_uuid')
        if cp:
            logger.info('setting value on property [%s]', cp)
            cp.value = "{\\\"get_input\\\":\\\"allottedresource0_providing_service_invariant_uuid\\\"}"
        else:
            raise ParameterError('no property providing_service_invariant_uuid found')

        cp = get_component_property(c, 'providing_service_uuid')
        if cp:
            cp.value = "{\\\"get_input\\\":\\\"allottedresource0_providing_service_uuid\\\"}"
        else:
            raise ParameterError('no property providing_service_uuid found')

        break
onboard_vf(vf)

# 11
slice_profile = '[{' \
                '\\"activityFactor\\":{\\"get_input\\":\\"anSP_activityFactor\\"},' \
                '\\"areaTrafficCapDL\\":{\\"get_input\\":\\"anSP_areaTrafficCapDL\\"},' \
                '\\"areaTrafficCapUL\\":{\\"get_input\\":\\"anSP_areaTrafficCapUL\\"},' \
                '\\"cSAvailabilityTarget\\":{\\"get_input\\":\\"anSP_cSAvailabilityTarget\\"},' \
                '\\"cSRealibilityMeanTime\\":{\\"get_input\\":\\"anSP_cSRealibilityMeanTime\\"}}]'
an_slice_profile = [Property('anSP', 'org.openecomp.datatypes.SliceProfile', slice_profile),
                    Property('ipAddress', 'string', '{\\"get_input\\":\\"ipAddress\\"}'),
                    Property('logicInterfaceId', 'string', '{\\"get_input\\":\\"logicInterfaceId\\"}'),
                    Property('nextHopInfo', 'string', '{\\"get_input\\":\\"nextHopInfo\\"}')]

logger.info("create service Slice Profile AN O2")
srv_slice_profile_an_o2 = create_service_1(create_name('SliceProfile_AN_O2'),
                                  'AN SliceProfile',
                                  properties = an_slice_profile,
                                  vnfs = [vf])

# 12
logger.info('create service SliceProfile_TN')
tn_slice_profile = [Property('jitter', 'string', '{\\"get_input\\":\\"jitter\\"}'),
                    Property('latency', 'integer', '{\\"get_input\\":\\"latency\\"}'),
                    Property('pLMNIdList', 'string', '{\\"get_input\\":\\"pLMNIdList\\"}'),
                    Property('sNSSAI', 'string', '{\\"get_input\\":\\"sNSSAI\\"}'),
                    Property('sST', 'integer', '{\\"get_input\\":\\"sST\\"}'),
                    Property('maxBandwidth', 'integer', '{\\"get_input\\":\\"maxBandwidth\\"}')]


srv_slice_profile_tn = create_service_1(create_name('SliceProfile_TN'),
                                      'TN SliceProfile',
                                      vnfs = [vf],
                                      properties = tn_slice_profile)

# 13
logger.info('create slice SliceProfile_CN')
cn_slice_profile = [Property('ipAddress', 'string', '{\\"get_input\\":\\"ipAddress\\"}'),
                    Property('logicInterfaceId', 'string', '{\\"get_input\\":\\"logicInterfaceId\\"}'),
                    Property('nextHopInfo', 'string', '{\\"get_input\\":\\"nextHopInfo\\"}')]
srv_slice_profile_cn = create_service_1(create_name('SliceProfile_CN'),
                                      'CN SliceProfile',
                                      vnfs = [ vf ],
                                      properties = cn_slice_profile)

# 14
logger.info('create service ServiceProfile O2')
service_profile = '[{' \
                '\\"resourceSharingLevel\\":{\\"get_input\\":\\"spProp_resourceSharingLevel\\"},' \
                '\\"sNSSAI\\":{\\"get_input\\":\\"spProp_sNSSAI\\"},' \
                '\\"coverageAreaTAList\\":{\\"get_input\\":\\"spProp_coverageAreaTAList\\"},' \
                '\\"sST\\":{\\"get_input\\":\\"spProp_sST\\"},' \
                '\\"dLThptPerUE\\":{\\"get_input\\":\\"spProp_dLThptPerUE\\"},' \
    '\\"uEMobilityLevel\\":{\\"get_input\\":\\"spProp_uEMobilityLevel\\"},'\
    '\\"latency\\":{\\"get_input\\":\\"spProp_latency\\"},' \
    '\\"uLThptPerUE\\":{\\"get_input\\":\\"spProp_uLThptPerUE\\"},' \
    '\\"maxNumberofUEs\\":{\\"get_input\\":\\"spProp_maxNumberofUEs\\"}' \
    '}]'
service_props = [Property('spProp', 'org.openecomp.datatypes.ServiceProfile', slice_profile)]
srv_profile_o2 = create_service_1(create_name('ServiceProfile_O2'),
                                'ServiceProfile',
                                properties=service_props,
                                vnfs = [vf, srv_slice_profile_cn, srv_slice_profile_tn, srv_slice_profile_an_o2],
                                role = 'option2')


# 15
logger.info('create service CST O2')
props = '[{' \
                 '\\"coverageAreaList\\":{\\"get_input\\":\\"csProp_coverageAreaList\\"},' \
                '\\"expDataRateDL\\":{\\"get_input\\":\\"csProp_expDataRateDL\\"},' \
                '\\"expDataRateUL\\":{\\"get_input\\":\\"csProp_expDataRateUL\\"},' \
                '\\"latency\\":{\\"get_input\\":\\"csProp_latency\\"},' \
                '\\"maxNumberofUEs\\":{\\"get_input\\":\\"csProp_maxNumberofEUs\\"},' \
                '\\"resourceSharingLevel\\":{\\"get_input\\":\\"csProp_resourceSharingLevel\\"},' \
                '\\"uEMobilityLevel\\":{\\"get_input\\":\\"csProp_uEMobilityLevel\\"},' \
                '\\"useInterval\\":{\\"get_input\\":\\"csProp_useInterval\\"},' \
        '}]'
srv = create_service_1(create_name('CST_O2'),
                     'CST',
                     role = 'option2',
                     service_type = 'embb',
                     vnfs = [srv_profile_o2],
                     properties = [Property('csProp', 'org.openecomp.datatypes.CSProperties', props)])
