"""Global settings module."""  # pylint: disable=bad-whitespace
# uncomment if socks is used
#from onapsdk.onap_service import OnapService

######################
#                    #
# ONAP SERVICES URLS #
#                    #
######################

AAI_URL     = "https://aai.api.sparky.simpledemo.onap.org:30233"
AAI_API_VERSION = "v20"
CDS_URL     = "http://portal.api.simpledemo.onap.org:30449"
MSB_URL     = "https://msb.api.simpledemo.onap.org:30283"
SDC_BE_URL  = "https://sdc.api.be.simpledemo.onap.org:30204"
SDC_FE_URL  = "https://sdc.api.fe.simpledemo.onap.org:30207"
SDNC_URL    = "https://sdnc.api.simpledemo.onap.org:30267"
SO_URL      = "http://so.api.simpledemo.onap.org:30277"
SO_API_VERSION = "v7"
VID_URL     = "https://vid.api.simpledemo.onap.org:30200"
VID_API_VERSION = "/vid"
CLAMP_URL   = "https://clamp.api.simpledemo.onap.org:30258"
VES_URL = "http://ves.api.simpledemo.onap.org:30417"
DMAAP_URL = "http://dmaap.api.simpledemo.onap.org:3904"

# uncomment if socks is used
#OnapService.set_proxy({'http': 'socks5h://127.0.0.1:8081', 'https': 'socks5h://127.0.0.1:8081'})

# execute in the shell to apply these settings
# export ONAP_PYTHON_SDK_SETTINGS="onap_settings"
