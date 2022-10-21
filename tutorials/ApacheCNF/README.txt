Demo directory structure:

/service_config.yaml  # Main configuration faile. Only here eventual changes are required
/templates/           # Base directory containing Apache CNF resources
    |- /base_native   # Directory containing base payload of VSP package in Helm VSP, doesn't need further proceeding
    |- /helm          # Directory containing helm charts that need to be packaged and attached to VSP package
    \- /cba           # Directory containing CBA content to be included to csar package. It is prepared for deployment
                      # and configuration of apache helm package
/automation/          # Directory with automation scripts. For more details read README file inside.

Installation requirements:
* Helm 3.5
* make
* python 3.8 or higher
