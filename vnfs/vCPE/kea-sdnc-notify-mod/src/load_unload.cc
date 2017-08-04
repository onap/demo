/**************************************************************************//**
* @file
* load_unload the module in kea 
*
*
* License
* -------
*
* Copyright (C) 2017 AT&T Intellectual Property. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*        http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
****************************************************************************/


#include "library_common.h"

using namespace isc::hooks;
namespace pt = boost::property_tree;

std::string param_url;
std::string json_params[3];

extern "C" {
int load(LibraryHandle& handle) {
    pt::ptree root;
    pt::read_json("/etc/kea/kea-sdnc-notify.conf", root);

    json_params[0] = (root.get<std::string>("url"));
    json_params[1] = (root.get<std::string>("siaddr"));

    std::cout << "kea-sdnc-notify.so loaded\n" << std::endl;
    return 0;
}
int unload() {
    return (0);
}
}
