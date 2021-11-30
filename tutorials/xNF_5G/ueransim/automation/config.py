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
from typing import Dict, Union, List
import oyaml as yaml
import os
from jinja2 import Template


class VariablesDict:
    env_variable = {
        "CI_PIPELINE_ID": os.getenv("CI_PIPELINE_ID", "2000000"),
        "USER": os.getenv("USER", "default"),
        "NAME_SUFFIX": os.getenv("TEST", os.getenv("CI_PIPELINE_ID"))
    }


class Config:
    def __init__(self, filename: str = "service_config.yaml", env_dict=None):
        if env_dict is None:
            env_dict = {}
        self.filepath = os.path.join(os.path.dirname(os.path.dirname(
            os.path.realpath(__file__))), filename)
        # self.filepath = os.path.join(os.path.dirname(
        #     os.path.realpath(__file__)), '..', filename)
        self.content_env: Dict = {**self._load_file(), **env_dict}
        self.service_instance: Union[None, Dict] = None
        self.service_model: Union[None, Dict] = None
        self.user_params: Union[None, Dict] = None
        self.cloud_regions: Union[None, List] = None
        self.so_input: Union[None, Dict] = None
        self.render()
        self.so_input = self.create_so_input()

    def _load_file(self) -> dict:
        with open(self.filepath) as file:
            file_content = yaml.safe_load(file)
        return file_content

    @staticmethod
    def templating(rend_dict: dict, render_keys: dict):
        for k, v in rend_dict.items():
            if isinstance(v, str):
                t = Template(v)
                rend_dict[k] = t.render(**render_keys)
            elif isinstance(v, dict):
                Config.templating(rend_dict=v, render_keys=render_keys)
            elif isinstance(v, list):
                for i in v:
                    Config.templating(rend_dict=i, render_keys=render_keys)
            else:
                pass
        return rend_dict

    def render(self):
        raw_file = self._load_file()
        config_dict = self.templating(
            rend_dict=raw_file,
            render_keys=self.content_env)

        self.__dict__.update(**config_dict)

    def create_so_input(self) -> dict:
        so_input_dict = dict()
        so_input_dict["subscription_service_type"] = self.service_instance.get("model_name")
        _vnfs = self.service_instance.get("vnfs")
        vnfs = list()

        for vnf in _vnfs:
            _vnf_raw = dict()
            _vnf_raw["model_name"] = vnf.get("model_name")
            if vnf.get("vnf_name_suffix"):
                _vnf_raw["vnf_name"] = "Instance_" + vnf.get("model_name") + "_" + vnf.get("vnf_name_suffix")
            else:
                _vnf_raw["vnf_name"] = "Instance_" + vnf.get("model_name") + "_" + str(_vnfs.index(vnf))
            if vnf.get("processing_priority"):
                _vnf_raw["processing_priority"] = vnf.get("processing_priority")
            _vnf_raw["parameters"] = vnf.get("parameters")
            _vnf_raw["vf_modules"] = list()
            _vf_modules = vnf.get("vf_modules")
            for vf_module in _vf_modules:
                _vf_module_raw = dict()
                _vf_module_raw["model_name"] = vf_module.get("model_name")
                if vf_module.get("vf_module_name_suffix"):
                    _vf_module_raw["vf_module_name"] = \
                        "Instance_" + vf_module.get("model_name") + "_" + vf_module.get("vf_module_name_suffix")
                else:
                    _vf_module_raw["vf_module_name"] = \
                        "Instance_" + vf_module.get("model_name") + "_" + str(_vf_modules.index(vf_module))
                if _vf_module_raw.get("processing_priority"):
                    _vf_module_raw["processing_priority"] = vf_module["processing_priority"]
                _vf_module_raw["parameters"] = vf_module.get("parameters")
                _vnf_raw["vf_modules"].append(_vf_module_raw)
            vnfs.append(_vnf_raw)
        so_input_dict["vnfs"] = vnfs

        return so_input_dict


config = Config(env_dict=VariablesDict.env_variable)
