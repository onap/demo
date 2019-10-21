Demo directory structure:

/templates/     #Base directory containing vFW resources
    |- /base    #Directory containing base payload of CSAR package, doesn't need further proceeding
    |- /helm    #Directory containing helm charts that need to be packaged and attached to csar package
    \- /cba     #Directory containing CBA content to be included to csar package
/examples/      #Directory with context-specific overrides over general resources
