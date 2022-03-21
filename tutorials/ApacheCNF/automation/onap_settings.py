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

"""Global settings module."""  # pylint: disable=bad-whitespace
# uncomment if socks is used
#from onapsdk.onap_service import OnapService

######################
#                    #
# ONAP SERVICES URLS #
#                    #
######################

AAI_URL         = "https://aai.api.sparky.simpledemo.onap.org:30233"
AAI_API_VERSION = "v23"
AAI_AUTH        = "Basic QUFJOkFBSQ=="
CDS_URL         = "http://portal.api.simpledemo.onap.org:30449"
CDS_AUTH        = ("ccsdkapps", "ccsdkapps")
MSB_URL         = "https://msb.api.simpledemo.onap.org:30283"
SDC_BE_URL      = "https://sdc.api.be.simpledemo.onap.org:30204"
SDC_FE_URL      = "https://sdc.api.fe.simpledemo.onap.org:30207"
SDC_AUTH        = "Basic YWFpOktwOGJKNFNYc3pNMFdYbGhhazNlSGxjc2UyZ0F3ODR2YW9HR21KdlV5MlU="
SDNC_URL        = "https://sdnc.api.simpledemo.onap.org:30267"
SDNC_AUTH       = "Basic YWRtaW46S3A4Yko0U1hzek0wV1hsaGFrM2VIbGNzZTJnQXc4NHZhb0dHbUp2VXkyVQ=="
SO_URL          = "http://so.api.simpledemo.onap.org:30277"
SO_API_VERSION  = "v7"
SO_AUTH         = "Basic SW5mcmFQb3J0YWxDbGllbnQ6cGFzc3dvcmQxJA=="
SO_CAT_DB_AUTH  = "Basic YnBlbDpwYXNzd29yZDEk"
VID_URL         = "https://vid.api.simpledemo.onap.org:30200"
VID_API_VERSION = "/vid"
CLAMP_URL       = "https://clamp.api.simpledemo.onap.org:30258"
CLAMP_AUTH      = "Basic ZGVtb0BwZW9wbGUub3NhYWYub3JnOmRlbW8xMjM0NTYh"
VES_URL         = "http://ves.api.simpledemo.onap.org:30417"
DMAAP_URL       = "http://dmaap.api.simpledemo.onap.org:3904"
NBI_URL         = "https://nbi.api.simpledemo.onap.org:30274"
NBI_API_VERSION = "/nbi/api/v4"
DCAEMOD_URL = ""
HOLMES_URL = "https://aai.api.sparky.simpledemo.onap.org:30293"
POLICY_URL = ""

## GUI
AAI_GUI_URL = "https://aai.api.sparky.simpledemo.onap.org:30220"
AAI_GUI_SERVICE = f"{AAI_GUI_URL}/services/aai/webapp/index.html#/browse"
CDS_GUI_SERVICE = f"{CDS_URL}/"
SO_MONITOR_GUI_SERVICE = f"{SO_URL}/"
SDC_GUI_SERVICE = f"{SDC_FE_URL}/sdc1/portal"
SDNC_DG_GUI_SERVICE = f"{SDNC_URL}/nifi/"
SDNC_ODL_GUI_SERVICE = f"{SDNC_URL}/odlux/index.html"

DCAEMOD_GUI_SERVICE = f"{DCAEMOD_URL}/"
HOLMES_GUI_SERVICE = f"{HOLMES_URL}/iui/holmes/default.html"
POLICY_GUI_SERVICE = f"{POLICY_URL}/onap/login.html"
POLICY_CLAMP_GUI_SERVICE = f"{CLAMP_URL}/"

# uncomment if socks is used
#OnapService.set_proxy({'http': 'socks5h://127.0.0.1:8081', 'https': 'socks5h://127.0.0.1:8081'})

# execute in the shell to apply these settings
# export ONAP_PYTHON_SDK_SETTINGS="onap_settings"
