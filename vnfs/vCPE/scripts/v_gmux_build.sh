#!/bin/bash


VPP_SOURCE_REPO_URL=$(cat /opt/config/vpp_source_repo_url.txt)
VPP_SOURCE_REPO_RELEASE_TAG=$(cat /opt/config/vpp_source_repo_release_tag.txt)
VPP_PATCH_URL=$(cat /opt/config/vpp_patch_url.txt)
HC2VPP_SOURCE_REPO_URL=$(cat /opt/config/hc2vpp_source_repo_url.txt)
HC2VPP_SOURCE_REPO_RELEASE_TAG=$(cat /opt/config/hc2vpp_source_repo_release_tag.txt)
HC2VPP_PATCH_URL=$(cat /opt/config/hc2vpp_patch_url.txt)
LIBEVEL_PATCH_URL=$(cat /opt/config/libevel_patch_url.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)
ERROR_MESSAGE="Execution of vG-MUX build script failed. Reason"

# Download required dependencies
echo "deb http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
echo "deb-src http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
apt-get update
apt-get install --allow-unauthenticated -y wget openjdk-8-jdk apt-transport-https ca-certificates g++ libcurl4-gnutls-dev
sleep 1

# Install the tools required for download codes
apt-get install -y expect git patch make linux-image-extra-`uname -r`

#Download and build the VPP codes
cd /opt
git clone ${VPP_SOURCE_REPO_URL} -b ${VPP_SOURCE_REPO_RELEASE_TAG} vpp
wget -O Vpp-Add-VES-agent-for-vG-MUX.patch ${VPP_PATCH_URL} 

cd vpp
patch -p1 < ../Vpp-Add-VES-agent-for-vG-MUX.patch
yes Y | make install-dep

# Check vpp build status
if [ $? -ne 0 ];
then
    echo '$ERROR_MESSAGE VPP build failed' > /opt/script_status.txt 
    exit
fi

# Install the evel-library first since we need the lib
cd /opt
apt-get install -y libcurl4-openssl-dev
git clone http://gerrit.onap.org/r/demo
wget -O vCPE-vG-MUX-libevel-fixup.patch ${LIBEVEL_PATCH_URL} 
cd demo
git checkout 3234c8ffab9faf85fd3bccfa720a3869ba39d13c
patch -p1 < ../vCPE-vG-MUX-libevel-fixup.patch
cd vnfs/VES5.0/evel/evel-library/bldjobs 
make

# Check eval-library installation status
if [ $? -ne 0 ];
then
    echo '$ERROR_MESSAGE Installation of eval-library failed' > /opt/script_status.txt 
    exit
fi


cp ../libs/x86_64/libevel.so /usr/lib
ldconfig

cd /opt/vpp/build-root
./bootstrap.sh
make V=0 PLATFORM=vpp TAG=vpp install-deb

# Check vpp/build-root build status
if [ $? -ne 0 ];
then
    echo '$ERROR_MESSAGE VPP/build-root build failed' > /opt/script_status.txt 
    exit
fi

# Install the VPP package
apt install -y python-ply-lex-3.5 python-ply-yacc-3.5 python-pycparser python-cffi
dpkg -i *.deb

# Check VPP package installation status
if [ $? -ne 0 ];
then
    echo '$ERROR_MESSAGE Installation of VPP package failed' > /opt/script_status.txt 
    exit
fi
systemctl stop vpp


# Download and install HC2VPP from source
cd /opt
git clone ${HC2VPP_SOURCE_REPO_URL} -b ${HC2VPP_SOURCE_REPO_RELEASE_TAG} hc2vpp
wget -O Hc2vpp-Add-VES-agent-for-vG-MUX.patch ${HC2VPP_PATCH_URL}

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
patch -p1 < ../Hc2vpp-Add-VES-agent-for-vG-MUX.patch
p_version_snap=$(cat ves/ves-impl/pom.xml | grep -A 1 "jvpp-ves" | tail -1)
p_version_snap=$(echo "${p_version_snap%<*}")
p_version_snap=$(echo "${p_version_snap#*>}")
p_version=$(echo "${p_version_snap%-*}")
mkdir -p  ~/.m2/repository/io/fd/vpp/jvpp-ves/${p_version_snap}
mvn install:install-file -Dfile=/usr/share/java/jvpp-ves-${p_version}.jar -DgroupId=io.fd.vpp -DartifactId=jvpp-ves -Dversion=${p_version_snap} -Dpackaging=jar
mvn clean install -nsu -DskipTests=true

# Check hc2vpp installation status
if [ $? -ne 0 ];
then
    echo '$ERROR_MESSAGE Installation of hc2vpp failed' > /opt/script_status.txt 
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

echo 'Execution of vG-MUX build script completed' > /opt/script_status.txt

