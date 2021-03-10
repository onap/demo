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

from abc import ABC
from onapsdk.so.so_element import SoElement
from onapsdk.onap_service import OnapService
from onapsdk.utils.headers_creator import headers_so_creator


class SoDBUpdate(SoElement, ABC):

    @classmethod
    def add_region_to_so_db(cls,
                            cloud_region_id: str,
                            complex_id: str,
                            identity_service_id: str = None,
                            **kwargs
                            ):
        """Method to add cloud_site data with identity_service to SO db.

        Args:
            cloud_region_id: the name of cloud region
            complex_id: name of complex
            identity_service_id: optional - id of identity service
            **kwargs: keyword arguments with parameters for identity service creation, like below

        Important:
            identity_services data will be overwrite, but in the same time
            cloud_sites data will not (shouldn't) be overwrite!

        Return:
            response object
        """

        if not identity_service_id:
            identity_service_id = 'Keystone_K8s'

        # params for identity_service creation
        orchestrator = kwargs.get('orchestrator', 'multicloud')
        identity_url = kwargs.get('identity_url', "http://1.2.3.4:5000/v2.0")
        mso_id = kwargs.get('mso_id', 'onapsdk_user')
        mso_pass = kwargs.get('mso_pass', 'mso_pass_onapsdk')
        project_domain_name = kwargs.get("project_domain_name", None)
        user_domain_name = kwargs.get("user_domain_name", None)
        member_role = kwargs.get('member_role', 'admin')
        admin_tenant = kwargs.get('admin_tenant', 'service')
        identity_server_type = kwargs.get('identity_server_type', 'KEYSTONE')
        identity_authentication_type = kwargs.get('identity_authentication_type', 'USERNAME_PASSWORD')

        data = {
            "id": cloud_region_id,
            "region_id": cloud_region_id,
            "aic_version": "2.5",
            "clli": complex_id,
            "orchestrator": orchestrator,
            "identityService": {
                "id": identity_service_id,
                "identityServerTypeAsString": "KEYSTONE",
                "hibernateLazyInitializer": {},
                "identity_url": identity_url,
                "mso_id": mso_id,
                "mso_pass": mso_pass,
                "project_domain_name": project_domain_name,
                "user_domain_name": user_domain_name,
                "admin_tenant": admin_tenant,
                "member_role": member_role,
                "tenant_metadata": True,
                "identity_server_type": identity_server_type,
                "identity_authentication_type": identity_authentication_type
            }
        }

        response = cls.send_message(
            "POST",
            "Create a region in SO db",
            f"{cls.base_url}/cloudSite",
            json=data,
            headers=headers_so_creator(OnapService.headers),
            exception=ValueError)

        return response
