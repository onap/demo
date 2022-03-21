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
    env_variable = dict(os.environ)


class Config:
    def __init__(self, filename: str = "service_config.yaml", env_dict=None, print_final_file=False):
        if env_dict is None:
            env_dict = {}
        self.filepath = os.path.join(os.path.dirname(os.path.dirname(
            os.path.realpath(__file__))), filename)
        self.content_env: Dict = env_dict
        self.service_instance: Union[None, Dict] = None
        self.service_model: Union[None, Dict] = None
        self.user_params: Union[None, Dict] = None
        self.cloud_regions: Union[None, List] = None
        self.so_input: Union[None, Dict] = None
        self.render(print_final_file)
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
                rend_dict[k] = t.render(**render_keys).strip()
            elif isinstance(v, dict):
                Config.templating(rend_dict=v, render_keys=render_keys)
            elif isinstance(v, list):
                for i in v:
                    Config.templating(rend_dict=i, render_keys=render_keys)
            else:
                pass
        return rend_dict

    def render(self, print_final_file=False):
        raw_file = self._load_file()
        config_dict = self._render(templated_file=raw_file)

        while not self._completed(templated_file=config_dict):
            config_dict = self._render(templated_file=config_dict)

        self.__dict__.update(**config_dict)
        if print_final_file:
            print(yaml.dump(config_dict, sort_keys=False))

    def _render(self, templated_file: dict) -> dict:
        config_dict = self.templating(
            rend_dict=templated_file,
            render_keys={**self.content_env, **templated_file})

        return config_dict

    def _completed(self, templated_file: dict) -> bool:
        for v in templated_file.values():
            if isinstance(v, str):
                if "{{" in v and "}}" in v:
                    return False
            elif isinstance(v, dict):
                return self._completed(templated_file=v)
            elif isinstance(v, list):
                for i in v:
                    return self._completed(templated_file=i)
            else:
                pass
        return True

    def create_so_input(self, other_cluster=False) -> dict:
        so_input_dict = dict()
        so_input_dict["subscription_service_type"] = self.service_instance.get("model_name")
        _vnfs = self.service_instance.get("vnfs")
        vnfs = list()

        for vnf in _vnfs:
            _vnf_raw = dict()
            # filter vnfs with cloud_region, code block not required with ONAP Jakrta+
            if vnf.get("cloud_region") and vnf.get("tenant_name"):
                if not other_cluster:
                    continue
                _vnf_raw["cloud_region"] = vnf.get("cloud_region")
                _vnf_raw["tenant_name"] = vnf.get("tenant_name")
            else:
                if other_cluster:
                    continue
            # end of filter, end of code block
            _vnf_raw["model_name"] = vnf.get("model_name")
            if vnf.get("vnf_name_suffix"):
                _vnf_raw["instance_name"] = "Instance_" + vnf.get("model_name") + "_" + vnf.get("vnf_name_suffix")
            else:
                _vnf_raw["instance_name"] = "Instance_" + vnf.get("model_name") + "_" + str(_vnfs.index(vnf))
            if vnf.get("processing_priority"):
                _vnf_raw["processing_priority"] = vnf.get("processing_priority")
            _vnf_raw["parameters"] = vnf.get("parameters")
            _vnf_raw["vf_modules"] = list()
            _vf_modules = vnf.get("vf_modules")
            for vf_module in _vf_modules:
                _vf_module_raw = dict()
                _vf_module_raw["model_name"] = vf_module.get("model_name")
                if vf_module.get("vf_module_name_suffix"):
                    _vf_module_raw["instance_name"] = \
                        "Instance_" + vf_module.get("model_name") + "_" + vf_module.get("vf_module_name_suffix")
                else:
                    _vf_module_raw["instance_name"] = \
                        "Instance_" + vf_module.get("model_name") + "_" + str(_vnfs.index(vnf)) + \
                        "_" + str(_vf_modules.index(vf_module))
                if vf_module.get("processing_priority"):
                    _vf_module_raw["processing_priority"] = vf_module["processing_priority"]
                _vf_module_raw["parameters"] = vf_module.get("parameters")
                _vnf_raw["vf_modules"].append(_vf_module_raw)
            vnfs.append(_vnf_raw)
        so_input_dict["vnfs"] = vnfs

        return so_input_dict

if __name__ == "__main__":
    config = Config(env_dict=VariablesDict.env_variable, print_final_file=True)
