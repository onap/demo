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
import zipfile
from io import BytesIO

from onapsdk.cds import Blueprint

from config import Config

logger = logging.getLogger("")
logger.setLevel(logging.DEBUG)
fh = logging.StreamHandler()
fh_formatter = logging.Formatter('%(asctime)s %(levelname)s %(lineno)d:%(filename)s(%(process)d) - %(message)s')
fh.setFormatter(fh_formatter)
logger.addHandler(fh)


def update_cba(file):
    mypath = os.path.dirname(os.path.realpath(__file__))
    file_path = os.path.join(mypath, file)
    try:
        with zipfile.ZipFile(file_path, 'r') as package:
            cba_io = BytesIO(package.read("CBA.zip"))

        blueprint = Blueprint(cba_io)
        blueprint.publish()
    except FileNotFoundError:
        logger.error("Error - File Not Found")
        exit(1)


def main():
    update_cba(Config.VSPFILE)


if __name__ == "__main__":
    main()
