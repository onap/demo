#!/bin/bash


VPP_SOURCE_REPO_URL=$(cat /opt/config/vpp_source_repo_url.txt)
VPP_SOURCE_REPO_RELEASE_TAG=$(cat /opt/config/vpp_source_repo_release_tag.txt)
HC2VPP_SOURCE_REPO_URL=$(cat /opt/config/hc2vpp_source_repo_url.txt)
HC2VPP_SOURCE_REPO_RELEASE_TAG=$(cat /opt/config/hc2vpp_source_repo_release_tag.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)
ERROR_MESSAGE= "Execution of vGbuild script failed. Reason:"

# Download required dependencies
echo "deb http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
echo "deb-src http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
apt-get --allow-unauthenticated update
apt-get install --allow-unauthenticated -y wget openjdk-8-jdk apt-transport-https ca-certificates g++ libcurl4-gnutls-dev
sleep 1

# Install the tools required for download codes
apt-get --allow-unauthenticated install -y expect git make linux-image-extra-`uname -r`

#Install DHCP server
apt-get install -y isc-dhcp-server

#Download and build the VPP codes
cd /opt
git clone ${VPP_SOURCE_REPO_URL} -b ${VPP_SOURCE_REPO_RELEASE_TAG} vpp

cd vpp
yes Y | make install-dep

# Check vpp build status
if [ $? -ne 0 ];
then
    echo '$ERROR_MESSAGE VPP build failed' > /opt/script_status.txt 
    exit
fi

cd build-root
./bootstrap.sh
make V=0 PLATFORM=vpp TAG=vpp install-deb

# Check vpp/build-root build status
if [ $? -ne 0 ];
then
    echo '$ERROR_MESSAGE VPP/build-root build failed' > /opt/script_status.txt 
    exit
fi

apt --allow-unauthenticated install -y python-ply-lex-3.5 python-ply-yacc-3.5 python-pycparser python-cffi

# Install the VPP package
dpkg -i *.deb

# Check vpp package installation status
if [ $? -ne 0 ];
then
    echo '$ERROR_MESSAGE VPP package installation failed' > /opt/script_status.txt 
    exit
fi

systemctl stop vpp


# Download and install HC2VPP from source
cd /opt
git clone ${HC2VPP_SOURCE_REPO_URL} -b ${HC2VPP_SOURCE_REPO_RELEASE_TAG} hc2vpp
apt-get install -y maven
mkdir -p ~/.m2
cat > ~/.m2/settings.xml << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!-- vi: set et smarttab sw=2 tabstop=2: -->
<settings xmlns="http://maven.apache.org/SETTINGS/1.0.0"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xsi:schemaLocation="http://maven.apache.org/SETTINGS/1.0.0 http://maven.apache.org/xsd/settings-1.0.0.xsd">

  <profiles>
    <profile>
      <id>fd.io-release</id>
      <repositories>
        <repository>
          <id>fd.io-mirror</id>
          <name>fd.io-mirror</name>
          <url>https://nexus.fd.io/content/groups/public/</url>
          <releases>
            <enabled>true</enabled>
            <updatePolicy>never</updatePolicy>
          </releases>
          <snapshots>
            <enabled>false</enabled>
          </snapshots>
        </repository>
      </repositories>
      <pluginRepositories>
        <pluginRepository>
          <id>fd.io-mirror</id>
          <name>fd.io-mirror</name>
          <url>https://nexus.fd.io/content/repositories/public/</url>
          <releases>
            <enabled>true</enabled>
            <updatePolicy>never</updatePolicy>
          </releases>
          <snapshots>
            <enabled>false</enabled>
          </snapshots>
        </pluginRepository>
      </pluginRepositories>
    </profile>

    <profile>
      <id>fd.io-snapshots</id>
      <repositories>
        <repository>
          <id>fd.io-snapshot</id>
          <name>fd.io-snapshot</name>
          <url>https://nexus.fd.io/content/repositories/fd.io.snapshot/</url>
          <releases>
            <enabled>false</enabled>
          </releases>
          <snapshots>
            <enabled>true</enabled>
          </snapshots>
        </repository>
      </repositories>
      <pluginRepositories>
        <pluginRepository>
          <id>fd.io-snapshot</id>
          <name>fd.io-snapshot</name>
          <url>https://nexus.fd.io/content/repositories/fd.io.snapshot/</url>
          <releases>
            <enabled>false</enabled>
          </releases>
          <snapshots>
            <enabled>true</enabled>
          </snapshots>
        </pluginRepository>
      </pluginRepositories>
    </profile>
    <profile>
      <id>opendaylight-snapshots</id>
      <repositories>
        <repository>
          <id>opendaylight-snapshot</id>
          <name>opendaylight-snapshot</name>
          <url>https://nexus.opendaylight.org/content/repositories/opendaylight.snapshot/</url>
          <releases>
            <enabled>false</enabled>
          </releases>
          <snapshots>
            <enabled>true</enabled>
          </snapshots>
        </repository>
      </repositories>
      <pluginRepositories>
        <pluginRepository>
          <id>opendaylight-shapshot</id>
          <name>opendaylight-snapshot</name>
          <url>https://nexus.opendaylight.org/content/repositories/opendaylight.snapshot/</url>
          <releases>
            <enabled>false</enabled>
          </releases>
          <snapshots>
            <enabled>true</enabled>
          </snapshots>
        </pluginRepository>
      </pluginRepositories>
    </profile>
  </profiles>

  <activeProfiles>
    <activeProfile>fd.io-release</activeProfile>
    <activeProfile>fd.io-snapshots</activeProfile>
    <activeProfile>opendaylight-snapshots</activeProfile>
  </activeProfiles>
</settings>
EOF

cd hc2vpp
mvn clean install

# Check hc2vpp build status
if [ $? -ne 0 ];
then
    echo '$ERROR_MESSAGE hc2vpp build failed' > /opt/script_status.txt 
    exit
fi

l_version=$(cat pom.xml | grep "<version>" | head -1)
l_version=$(echo "${l_version%<*}")
l_version=$(echo "${l_version#*>}")
mv vpp-integration/minimal-distribution/target/vpp-integration-distribution-${l_version}-hc/vpp-integration-distribution-${l_version} /opt/honeycomb
sed -i 's/127.0.0.1/0.0.0.0/g' /opt/honeycomb/config/honeycomb.json

# Disable automatic upgrades
if [[ $CLOUD_ENV != "rackspace" ]]
then
    echo "APT::Periodic::Unattended-Upgrade \"0\";" >> /etc/apt/apt.conf.d/10periodic
    sed -i 's/\(APT::Periodic::Unattended-Upgrade\) "1"/\1 "0"/' /etc/apt/apt.conf.d/20auto-upgrades
fi

echo 'Execution of vG build script completed' > /opt/script_status.txt
