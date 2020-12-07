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
import os

from config import Config
from onapsdk.msb.k8s import ConnectivityInfo

logger = logging.getLogger("")
logger.setLevel(logging.DEBUG)
fh = logging.StreamHandler()
fh_formatter = logging.Formatter('%(asctime)s %(levelname)s %(lineno)d:%(filename)s(%(process)d) - %(message)s')
fh.setFormatter(fh_formatter)
logger.addHandler(fh)

MYPATH = os.path.dirname(os.path.realpath(__file__))

logger.info("******** Connectivity Info *******")
with open(os.path.join(MYPATH, Config.CLUSTER_KUBECONFIG_PATH), 'rb') as kubeconfig_file:
    kubeconfig = kubeconfig_file.read()
try:
    connectivity_info = ConnectivityInfo.get_connectivity_info_by_region_id(cloud_region_id=Config.CLOUD_REGION)
    logger.info("Connectivity Info exists ")
    logger.info("Delete Connectivity Info exists ")
    connectivity_info.delete()
    connectivity_info = ConnectivityInfo.create(cloud_region_id=Config.CLOUD_REGION,
                                                cloud_owner=Config.CLOUD_OWNER,
                                                kubeconfig=kubeconfig)
except:
    logger.info("Connectivity Info does not exists ")
    connectivity_info = ConnectivityInfo.create(cloud_region_id=Config.CLOUD_REGION,
                                                cloud_owner=Config.CLOUD_OWNER,
                                                kubeconfig=kubeconfig)
    logger.info("Connectivity Info created ")
