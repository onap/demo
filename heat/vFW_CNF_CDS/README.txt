Demo directory structure:

/templates/           #Base directory containing vFW resources
    |- /base_dummy    #Directory containing base payload of VSP package in OpenStack VSP format, doesn't need further proceeding
    |- /base_native   #Directory containing base payload of VSP package in Helm VSP, doesn't need further proceeding
    |- /helm          #Directory containing helm charts that need to be packaged and attached to VSP package
    \- /cba           #Directory containing CBA content to be included to csar package. It is prepared for 
/examples/            #Directory with context-specific overrides over general resources
/automation/          #Directory with automation scripts. For more details read README file inside.

Note: Makefile script generates two VSP packages, one in Frankfurt format with helm associated with dummy heat templates and second
with native Helm VSP format where helm packages are standalone. CBA folder contains CBA Definition for native VSP format but 
make is coverting the definition into Frankfurt format for Frankfurt VSP. Frankfurt VSP is still supported in Guilin. 
