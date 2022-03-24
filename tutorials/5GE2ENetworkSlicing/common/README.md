# AAI automated configuration

In order to automate AAI manual configuration, custom script has been developed. Scope of the script:
 * Create customer and service type
 * Configure multicloud for CORE subnet instantiation in Option1

## Usage

You have to provide 3 required parameters to the script:

 1. MASTER_IP - IP Address of your ONAP
 2. TENANT_ID - Parameter used to create relationship between cloudregion and customer
 3. TENANT_NAME - Parameter used to create relationship between cloudregion and customer
 
Make sure to providate the same value for TENANT_ID as in the NBI deployment.