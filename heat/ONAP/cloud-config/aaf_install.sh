#!/bin/bash

# Read configuration files
GERRIT_BRANCH=$(cat /opt/config/gerrit_branch.txt)
CODE_REPO=$(cat /opt/config/remote_repo.txt)
HTTP_PROXY=$(cat /opt/config/http_proxy.txt)
HTTPS_PROXY=$(cat /opt/config/https_proxy.txt)

if [ $HTTP_PROXY != "no_proxy" ]
then
    export http_proxy=$HTTP_PROXY
    export https_proxy=$HTTPS_PROXY
fi


# Download dependencies
echo "deb http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
echo "deb-src http://ppa.launchpad.net/openjdk-r/ppa/ubuntu $(lsb_release -c -s) main" >>  /etc/apt/sources.list.d/java.list
apt-get update
apt-get install --allow-unauthenticated -y openjdk-8-jdk maven

# Clone Gerrit repository and run docker containers
cd /opt
git clone -b $GERRIT_BRANCH --single-branch $CODE_REPO
chmod +x /opt/authz/auth/auth-cass/docker/dinstall.sh
chmod +x /opt/authz/auth/auth-cass/docker/backup/backup.sh
chmod +x /opt/authz/auth/docker/dbuild.sh
chmod +x /opt/authz/auth/docker/drun.sh
chmod +x /opt/authz/auth/docker/dstart.sh
chmod +x /opt/authz/auth/docker/dstop.sh

#Update maven settings
cat > /usr/share/maven/conf/settings.xml << EOF
<?xml version="1.0" encoding="UTF-8"?>

<!--
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
-->

<!--
 | This is the configuration file for Maven. It can be specified at two levels:
 |
 |  1. User Level. This settings.xml file provides configuration for a single user,
|                 and is normally provided in \${user.home}/.m2/settings.xml.
 |
 |                 NOTE: This location can be overridden with the CLI option:
 |
 |                 -s /path/to/user/settings.xml
 |
 |  2. Global Level. This settings.xml file provides configuration for all Maven
 |                 users on a machine (assuming they're all using the same Maven
 |                 installation). It's normally provided in
|                 \${maven.home}/conf/settings.xml.
 |
 |                 NOTE: This location can be overridden with the CLI option:
 |
 |                 -gs /path/to/global/settings.xml
 |
 | The sections in this sample file are intended to give you a running start at
 | getting the most out of your Maven installation. Where appropriate, the default
 | values (values used when the setting is not specified) are provided.
 |
 |-->
<settings xmlns="http://maven.apache.org/SETTINGS/1.0.0"
          xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
          xsi:schemaLocation="http://maven.apache.org/SETTINGS/1.0.0 http://maven.apache.org/xsd/settings-1.0.0.xsd">
  <!-- localRepository
   | The path to the local repository maven will use to store artifacts.
   |
| Default: \${user.home}/.m2/repository
  <localRepository>/path/to/local/repo</localRepository>
  -->

  <!-- interactiveMode
   | This will determine whether maven prompts you when it needs input. If set to false,
   | maven will use a sensible default value, perhaps based on some other setting, for
   | the parameter in question.
   |
   | Default: true
  <interactiveMode>true</interactiveMode>
  -->

  <!-- offline
   | Determines whether maven should attempt to connect to the network when executing a build.
   | This will have an effect on artifact downloads, artifact deployment, and others.
   |
   | Default: false
  <offline>false</offline>
  -->

  <!-- pluginGroups
   | This is a list of additional group identifiers that will be searched when resolving plugins by their prefix, i.e.
   | when invoking a command line like "mvn prefix:goal". Maven will automatically add the group identifiers
   | "org.apache.maven.plugins" and "org.codehaus.mojo" if these are not already contained in the list.
   |-->
  <pluginGroups>
    <!-- pluginGroup
     | Specifies a further group identifier to use for plugin lookup.
    <pluginGroup>com.your.plugins</pluginGroup>
    -->
  </pluginGroups>

EOF

if [[ $(cat /opt/config/https_proxy.txt) != "no_proxy" ]]; then
  HTTPS_PROXY_HOST=$(cat /opt/config/https_proxy.txt | cut -d ':' -f1)
  HTTPS_PROXY_PORT=$(cat /opt/config/https_proxy.txt | cut -d ':' -f2)

  cat >> settings.xml << EOF
  <!-- proxies
   | This is a list of proxies which can be used on this machine to connect to the network.
   | Unless otherwise specified (by system property or command-line switch), the first proxy
   | specification in this list marked as active will be used.
   |-->
  <proxies>
    <proxy>
      <id>optional</id>
      <active>true</active>
      <protocol>http</protocol>
      <username>proxyuser</username>
      <password>proxypass</password>
      <host>$HTTPS_PROXY_HOST</host>
      <port>$HTTPS_PROXY_PORT</port>
      <nonProxyHosts>local.net|some.host.com</nonProxyHosts>
    </proxy>
    <proxy>
      <id>optional</id>
      <active>true</active>
      <protocol>https</protocol>
      <username>proxyuser</username>
      <password>proxypass</password>
      <host>$HTTPS_PROXY_HOST</host>
      <port>$HTTPS_PROXY_PORT</port>
      <nonProxyHosts>local.net|some.host.com</nonProxyHosts>
    </proxy>
  </proxies>

EOF
fi

cat >> settings.xml << EOF

  <!-- servers
   | This is a list of authentication profiles, keyed by the server-id used within the system.
   | Authentication profiles can be used whenever maven must make a connection to a remote server.
   |-->
  <servers>
    <!-- server
     | Specifies the authentication information to use when connecting to a particular server, identified by
     | a unique name within the system (referred to by the 'id' attribute below).
     |
     | NOTE: You should either specify username/password OR privateKey/passphrase, since these pairings are
     |       used together.
     |
    <server>
      <id>deploymentRepo</id>
      <username>repouser</username>
      <password>repopwd</password>
    </server>
    -->

    <!-- Another sample, using keys to authenticate.
    <server>
      <id>siteServer</id>
      <privateKey>/path/to/private/key</privateKey>
      <passphrase>optional; leave empty if not used.</passphrase>
    </server>
    -->
  </servers>

  <!-- mirrors
   | This is a list of mirrors to be used in downloading artifacts from remote repositories.
   |
   | It works like this: a POM may declare a repository to use in resolving certain artifacts.
   | However, this repository may have problems with heavy traffic at times, so people have mirrored
   | it to several places.
   |
   | That repository definition will have a unique id, so we can create a mirror reference for that
   | repository, to be used as an alternate download site. The mirror site will be the preferred
   | server for that repository.
   |-->

  <!-- profiles
   | This is a list of profiles which can be activated in a variety of ways, and which can modify
   | the build process. Profiles provided in the settings.xml are intended to provide local machine-
   | specific paths and repository locations which allow the build to work in the local environment.
   |
   | For example, if you have an integration testing plugin - like cactus - that needs to know where
   | your Tomcat instance is installed, you can provide a variable here such that the variable is
   | dereferenced during the build process to configure the cactus plugin.
   |
   | As noted above, profiles can be activated in a variety of ways. One way - the activeProfiles
   | section of this document (settings.xml) - will be discussed later. Another way essentially
   | relies on the detection of a system property, either matching a particular value for the property,
   | or merely testing its existence. Profiles can also be activated by JDK version prefix, where a
   | value of '1.4' might activate a profile when the build is executed on a JDK version of '1.4.2_07'.
   | Finally, the list of active profiles can be specified directly from the command line.
   |
   | NOTE: For profiles defined in the settings.xml, you are restricted to specifying only artifact
   |       repositories, plugin repositories, and free-form properties to be used as configuration
   |       variables for plugins in the POM.
   |
   |-->



  
  
  <profiles>
      <profile>
          
          <id>10_nexus</id>
          <!--Enable snapshots for the built in central repo to direct -->
          <!--all requests to nexus via the mirror -->
          <repositories>
              <repository>
                  <id>10_nexus</id>
                  <url>http://repo.maven.apache.org/maven2/</url>
                  <releases><enabled>true</enabled></releases>
                  <snapshots><enabled>true</enabled></snapshots>
              </repository>
          </repositories>
          
          <pluginRepositories>
              <pluginRepository>
                  <id>10_nexus</id>
                  <url>http://repo.maven.apache.org/maven2/</url>
                  <releases><enabled>true</enabled></releases>
                  <snapshots><enabled>true</enabled></snapshots>
              </pluginRepository>
          </pluginRepositories>
          
      </profile>
      <profile>
          <id>20_openecomp-public</id>
          <repositories>
              <repository>
                  <id>20_openecomp-public</id>
                  <name>20_openecomp-public</name>
                  <url>https://nexus.onap.org/content/repositories/public/</url>
                  <releases>
                      <enabled>true</enabled>
                      <updatePolicy>daily</updatePolicy>
                  </releases>
                  <snapshots>
                      <enabled>false</enabled>
                  </snapshots>
              </repository>
          </repositories>
          <pluginRepositories>
              <pluginRepository>
                  <id>20_openecomp-public</id>
                  <name>20_openecomp-public</name>
                  <url>https://nexus.onap.org/content/repositories/public/</url>
                  <releases>
                      <enabled>true</enabled>
                      <updatePolicy>daily</updatePolicy>
                  </releases>
                  <snapshots>
                      <enabled>false</enabled>
                  </snapshots>
              </pluginRepository>
          </pluginRepositories>
      </profile>
      <profile>
          <id>30_openecomp-staging</id>
          <repositories>
              <repository>
                  <id>30_openecomp-staging</id>
                  <name>30_openecomp-staging</name>
                  <url>https://nexus.onap.org/content/repositories/staging/</url>
                  <releases>
                      <enabled>true</enabled>
                      <updatePolicy>daily</updatePolicy>
                  </releases>
                  <snapshots>
                      <enabled>false</enabled>
                  </snapshots>
              </repository>
          </repositories>
          <pluginRepositories>
              <pluginRepository>
                  <id>30_openecomp-staging</id>
                  <name>30_openecomp-staging</name>
                  <url>https://nexus.onap.org/content/repositories/staging/</url>
                  <releases>
                      <enabled>true</enabled>
                      <updatePolicy>daily</updatePolicy>
                  </releases>
                  <snapshots>
                      <enabled>false</enabled>
                  </snapshots>
              </pluginRepository>
          </pluginRepositories>
      </profile>
      <profile>
          <id>40_openecomp-release</id>
          <repositories>
              <repository>
                  <id>40_openecomp-release</id>
                  <name>40_openecomp-release</name>
                  <url>https://nexus.onap.org/content/repositories/releases/</url>
                  <releases>
                      <enabled>true</enabled>
                      <updatePolicy>daily</updatePolicy>
                  </releases>
                  <snapshots>
                      <enabled>false</enabled>
                  </snapshots>
              </repository>
          </repositories>
          <pluginRepositories>
              <pluginRepository>
                  <id>40_openecomp-release</id>
                  <name>40_openecomp-release</name>
                  <url>https://nexus.onap.org/content/repositories/releases/</url>
                  <releases>
                      <enabled>true</enabled>
                      <updatePolicy>daily</updatePolicy>
                  </releases>
                  <snapshots>
                      <enabled>false</enabled>
                  </snapshots>
              </pluginRepository>
          </pluginRepositories>
      </profile>
      
      <profile>
          <id>50_openecomp-snapshots</id>
          <repositories>
              <repository>
                  <id>50_openecomp-snapshot</id>
                  <name>50_openecomp-snapshot</name>
                  <url>https://nexus.onap.org/content/repositories/snapshots/</url>
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
                  <id>50_openecomp-snapshot</id>
                  <name>50_openecomp-snapshot</name>
                  <url>https://nexus.onap.org/content/repositories/snapshots/</url>
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
          <id>60_opendaylight-release</id>
          <repositories>
              <repository>
                  <id>60_opendaylight-mirror</id>
                  <name>60_opendaylight-mirror</name>
                  <url>https://nexus.opendaylight.org/content/repositories/public/</url>
                  <releases>
                      <enabled>true</enabled>
                      <updatePolicy>daily</updatePolicy>
                  </releases>
                  <snapshots>
                      <enabled>false</enabled>
                  </snapshots>
              </repository>
          </repositories>
          <pluginRepositories>
              <pluginRepository>
                  <id>60_opendaylight-mirror</id>
                  <name>60_opendaylight-mirror</name>
                  <url>https://nexus.opendaylight.org/content/repositories/public/</url>
                  <releases>
                      <enabled>true</enabled>
                      <updatePolicy>daily</updatePolicy>
                  </releases>
                  <snapshots>
                      <enabled>false</enabled>
                  </snapshots>
              </pluginRepository>
          </pluginRepositories>
      </profile>
      
      <profile>
          <id>70_opendaylight-snapshots</id>
          <repositories>
              <repository>
                  <id>70_opendaylight-snapshot</id>
                  <name>70_opendaylight-snapshot</name>
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
                  <id>70_opendaylight-snapshot</id>
                  <name>70_opendaylight-snapshot</name>
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
      <activeProfile>10_nexus</activeProfile>
      <activeProfile>20_openecomp-public</activeProfile>
      <activeProfile>30_openecomp-staging</activeProfile>
      <activeProfile>40_openecomp-release</activeProfile>
      <activeProfile>50_openecomp-snapshots</activeProfile>
      <activeProfile>60_opendaylight-release</activeProfile>
      <activeProfile>70_opendaylight-snapshots</activeProfile>

  </activeProfiles>
  
</settings>
EOF

cd /opt/authz
mvn install -Dmaven.test.skip=true

cd /opt
./aaf_vm_init.sh
