
A version of DANOS with the VES reporting client will be made available for download and uploading into your glance repository.
      wget http:// .....  danos-1908-amd64-vrouter_20200425T1203-amd64.hybrid.iso

Remember that a flavor for DANOS must be available in your openstack instance for 4 vcpu, 4096 MB, 8 GB since the standard 
ubuntu flavors used for most ONAP VNF demonstrations are not the right size.


The following instructions were used to build the custom version and can be used as well.

 
1. Install the DANOS package build tool
     apt install docker.io
     git clone https://github.com/jsouthworth/danos-buildpackage
     cd danos-buildpackage
     go install jsouthworth.net/go/danos-buildimage/cmd/danos-buildpackage

2. Change to the directory above debian 
     cd ~/demo/vnfs/VESreporting_vFW5.0_DANOS

3. Build ves library and create debian package

     danos-buildpackage -version 1908

     vpp-measurement-reporter-danos_0.1_amd64.deb will be created in the ~/demo/vnfs directory (..) 

4. Confirm the debian package has the libevel.so and vpp_measurement_reporter_danos executables

     dpkg-deb -c  vpp-measurement-reporter-danos_0.1_amd64.deb

5. Create a DANOS ISO with the addition of the VES debian
           a. Follow steps in https://danosproject.atlassian.net/wiki/spaces/DAN/pages/491554/Creating+a+DANOS+ISO+using+binary+packages
	   b. At the step:   "If any additional packages are required to be added to the DANOS image:"
                      mkdir -p config/packages.chroot/
                      cp  <path-to-deb>/vpp-measurement-reporter-danos_0.1_amd64.deb  config/packages.chroot/

           c. Since this is a new debian for the iso  you need to add the reference to it under config/package-lists
                      cd config/package-lists
                      create a file:
                               vpp-measurement-reporter-danos.list.chroot
                      add the line to the file:
                               vpp-measurement-reporter-danos
           c. continue the steps include:
                 sed -i 's/--.*distribution .*\\/--distribution '"stretch"' \\/' auto/config
	         auto/clean
                 auto/config
                 auto/build

           d. an iso image should be built like:
                   danos-1908-amd64-vrouter_20200425T1203-amd64.hybrid.iso



6. Follow the instructions up through Step K. for installing a DANOS ISO in Openstack from 
 
         https://danosproject.atlassian.net/wiki/spaces/DAN/pages/79560705/Creating+a+DANOS+Virtual+Router+VNF+in+Openstack

         
Ignore the creation of the DANOS intance from the new glance image and the traffic source and traffic sink parts since the heat template for ONAP will cover those pieces.
         

