#!/bin/bash

REPO_URL_BLOB=$(cat /opt/config/repo_url_blob.txt)
REPO_URL_ARTIFACTS=$(cat /opt/config/repo_url_artifacts.txt)
DEMO_ARTIFACTS_VERSION=$(cat /opt/config/demo_artifacts_version.txt)
INSTALL_SCRIPT_VERSION=$(cat /opt/config/install_script_version.txt)
VPP_SOURCE_REPO_URL=$(cat /opt/config/vpp_source_repo_url.txt)
VPP_SOURCE_REPO_BRANCH=$(cat /opt/config/vpp_source_repo_branch.txt)
VPP_PATCH_URL=$(cat /opt/config/vpp_patch_url.txt)
HC2VPP_SOURCE_REPO_URL=$(cat /opt/config/hc2vpp_source_repo_url.txt)
HC2VPP_SOURCE_REPO_BRANCH=$(cat /opt/config/hc2vpp_source_repo_branch.txt)
CLOUD_ENV=$(cat /opt/config/cloud_env.txt)

# Build states are:
# 'build' - just build the code
# 'done' - code is build, install and setup
# 'auto' - bulid, install and setup
BUILD_STATE="auto"

if [[ -f /opt/config/compile_state.txt ]]
then
    BUILD_STATE=$(cat /opt/config/compile_state.txt)
fi

# Convert Network CIDR to Netmask
cdr2mask () {
        # Number of args to shift, 255..255, first non-255 byte, zeroes
        set -- $(( 5 - ($1 / 8) )) 255 255 255 255 $(( (255 << (8 - ($1 % 8))) & 255 )) 0 0 0
        [ $1 -gt 1 ] && shift $1 || shift
        echo ${1-0}.${2-0}.${3-0}.${4-0}
}

# OpenStack network configuration
if [[ $BUILD_STATE != "build" ]]
then
    if [[ $CLOUD_ENV == "openstack" ]]
    then
        echo 127.0.0.1 $(hostname) >> /etc/hosts

        # Allow remote login as root
        mv /root/.ssh/authorized_keys /root/.ssh/authorized_keys.bk
        cp /home/ubuntu/.ssh/authorized_keys /root/.ssh
    fi
fi  # endif BUILD_STATE != "build"

if [[ $BUILD_STATE != "done" ]]
then
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
    git clone ${VPP_SOURCE_REPO_URL} -b ${VPP_SOURCE_REPO_BRANCH} vpp
    wget -O VPP-Add-Option82-Nat-Filter-For-vBRG.patch ${VPP_PATCH_URL}

    cd vpp
    patch -p1 < ../VPP-Add-Option82-Nat-Filter-For-vBRG.patch
    expect -c "
            set timeout 60;
            spawn make install-dep;
            expect {
                    \"Do you want to continue?*\" {send \"Y\r\"; interact}
            }
    "

    cd build-root
    ./bootstrap.sh
    make V=0 PLATFORM=vpp TAG=vpp install-deb

    # Install the VPP package
    dpkg -i *.deb
    systemctl stop vpp
fi  # endif BUILD_STATE != "done"

if [[ $BUILD_STATE != "build" ]]
then
    # Auto-start configuration for the VPP
    cat > /etc/vpp/startup.conf << EOF

unix {
  nodaemon
  log /tmp/vpp.log
  full-coredump
  cli-listen localhost:5002
  startup-config /etc/vpp/setup.gate
}

api-trace {
  on
}

api-segment {
  gid vpp
}

cpu {
        ## In the VPP there is one main thread and optionally the user can create worker(s)
        ## The main thread and worker thread(s) can be pinned to CPU core(s) manually or automatically

        ## Manual pinning of thread(s) to CPU core(s)

        ## Set logical CPU core where main thread runs
        # main-core 1

        ## Set logical CPU core(s) where worker threads are running
        # corelist-workers 2-3,18-19

        ## Automatic pinning of thread(s) to CPU core(s)

        ## Sets number of CPU core(s) to be skipped (1 ... N-1)
        ## Skipped CPU core(s) are not used for pinning main thread and working thread(s).
        ## The main thread is automatically pinned to the first available CPU core and worker(s)
        ## are pinned to next free CPU core(s) after core assigned to main thread
        # skip-cores 4

        ## Specify a number of workers to be created
        ## Workers are pinned to N consecutive CPU cores while skipping "skip-cores" CPU core(s)
        ## and main thread's CPU core
        # workers 2

        ## Set scheduling policy and priority of main and worker threads

        ## Scheduling policy options are: other (SCHED_OTHER), batch (SCHED_BATCH)
        ## idle (SCHED_IDLE), fifo (SCHED_FIFO), rr (SCHED_RR)
        # scheduler-policy fifo

        ## Scheduling priority is used only for "real-time policies (fifo and rr),
        ## and has to be in the range of priorities supported for a particular policy
        # scheduler-priority 50
}

# dpdk {
        ## Change default settings for all intefaces
        # dev default {
                ## Number of receive queues, enables RSS
                ## Default is 1
                # num-rx-queues 3

                ## Number of transmit queues, Default is equal
                ## to number of worker threads or 1 if no workers treads
                # num-tx-queues 3

                ## Number of descriptors in transmit and receive rings
                ## increasing or reducing number can impact performance
                ## Default is 1024 for both rx and tx
                # num-rx-desc 512
                # num-tx-desc 512

                ## VLAN strip offload mode for interface
                ## Default is off
                # vlan-strip-offload on
        # }

        ## Whitelist specific interface by specifying PCI address
        # dev 0000:02:00.0
        ## Whitelist specific interface by specifying PCI address and in
        ## addition specify custom parameters for this interface
        # dev 0000:02:00.1 {
        #       num-rx-queues 2
        # }

        ## Change UIO driver used by VPP, Options are: igb_uio, vfio-pci
        ## and uio_pci_generic (default)
        # uio-driver vfio-pci

        ## Disable mutli-segment buffers, improves performance but
        ## disables Jumbo MTU support
        # no-multi-seg

        ## Increase number of buffers allocated, needed only in scenarios with
        ## large number of interfaces and worker threads. Value is per CPU socket.
        ## Default is 16384
        # num-mbufs 128000

        ## Change hugepages allocation per-socket, needed only if there is need for
        ## larger number of mbufs. Default is 256M on each detected CPU socket
        # socket-mem 2048,2048
# }

EOF

    #Get list of network device PCI bus addresses
    get_nic_pci_list() {
        while read -r line ; do
            if [ "$line" != "${line#*network device}" ];
            then
                 echo -n "${line%% *} "
            fi
        done < <(lspci)
    }

    NICS=$(get_nic_pci_list)
    NICS=`echo ${NICS} | sed 's/[0]\+\([0-9]\)/\1/g' | sed 's/[.:]/\//g'`

    BRG_BNG_NIC=GigabitEthernet`echo ${NICS} | cut -d " " -f 2`  # second interface in list
    echo $BRG_BNG_NIC > /opt/config/brg_nic.txt


    cat > /etc/vpp/setup.gate << EOF
set int state ${BRG_BNG_NIC} up
set dhcp client intfc ${BRG_BNG_NIC} hostname brg-emulator
EOF

echo "sdnc_ip: $(cat /opt/config/sdnc_ip.txt)" > /opt/config/ip.txt
echo "hcip: 192.168.4.20" >> /opt/config/ip.txt


    cat > /opt/bind_nic.sh << 'EOF'
#!/bin/bash
while :
do
        if [[ ! $(ps -aux | grep [[:alnum:]]*/vpp/startup.conf | wc -l) = 2 ]];
        then
                echo "vpp not running"
        else
                break
        fi
done


BRG_BNG_NIC=$(cat /opt/config/brg_nic.txt)
sdnc_ip=$(cat /opt/config/sdnc_ip.txt)

lstack_tap=$(vppctl tap connect lstack)
vppctl set int state $lstack_tap up

vppctl set interface l2 bridge $lstack_tap 10 0
vppctl set bridge-domain arp term 10

tap0_tap=$(vppctl tap connect tap0)
sleep 3
vppctl set int state $tap0_tap up
vppctl set int ip addr $tap0_tap 20.0.0.40/24
ifconfig tap0 192.168.4.20/24
route add -host $sdnc_ip tap0
route add -host 20.0.0.40 tap0
vppctl ip route add 192.168.4.0/24 via $tap0_tap
vppctl set interface snat in $tap0_tap out ${BRG_BNG_NIC}
vppctl snat add interface address ${BRG_BNG_NIC}

#Get HW addr of $tap0_tap
while read -r hw
do
    if [[ "$hw" = $tap0_tap* ]];
    then
        read -r hw
        hw_addr=${hw##* }
        break
    fi
done < <(vppctl show hardware)

arp -s $sdnc_ip $hw_addr
arp -s 20.0.0.40 $hw_addr

#Get HW addr of tap0
var=$(ifconfig tap0)
var=${var##*HWaddr}
var=${var%inet*}
tap0_addr=${var%inet*}

vppctl set ip arp $tap0_tap 192.168.4.20 $tap0_addr

EOF
    chmod +x /opt/bind_nic.sh

    #set nat rule
    cat > /opt/set_nat.sh << 'EOF'
#! /bin/bash

while :
do
    if [[ ! $(ps -aux | grep [[:alnum:]]*/vpp/startup.conf | wc -l) = 2 ]];
    then
        #echo "vpp not running"
        continue
    fi

    flag=0
    while read -r line
    do
        if [ flag = 0 ];
        then
            re=${line#*/[0-9]/[0-9]}
            if [ "$line" != "$re" ];
            then
                flag=1
            else
                flag=0
                continue
            fi
        else
            ip=${line%/*}
            if [[ $ip = *\.*\.*\.* ]];
            then
                #echo "ip address is $ip"
                if [ ! -f /opt/config/ip.txt ];
                then
                    echo "file /opt/config/ip.txt doesn't exists"
                    continue
                fi
                while read -r tap_ip
                do
                    if [[ $tap_ip = hcip* ]];
                    then
                        tap_ip=${tap_ip#*" "}
                        echo "hc tap ip address is $tap_ip"
                        vppctl snat add static mapping tcp local $tap_ip 8183 external $ip 8183
                        exit 0
                    fi
                done < /opt/config/ip.txt
            else
                if [[ ! $ip = */[0-9] ]];
                then
                    flag=0
                    #echo "not correct"
                fi
            fi
        fi
    done < <(vppctl show int addr)
    sleep 1
done
EOF

    chmod +x /opt/set_nat.sh

    #Create script to run bind_nic.sh and set_nat.sh
    cat > /opt/nat_service.sh << 'EOF'
#! /bin/bash

sleep 15
/opt/bind_nic.sh
/opt/set_nat.sh

EOF
    chmod +x /opt/nat_service.sh

fi  # endif BUILD_STATE != "build"

if [[ $BUILD_STATE != "done" ]]
then

    # Download and install HC2VPP from source
    cd /opt
    git clone ${HC2VPP_SOURCE_REPO_URL} -b ${HC2VPP_SOURCE_REPO_BRANCH} hc2vpp

    apt-get -f -y install
    apt-get install -y maven
    mkdir -p /root/.m2
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
fi  # endif BUILD_STATE != "done"

if [[ $BUILD_STATE != "build" ]]
then
    # Create systemctl service for Honeycomb
    cat > /etc/systemd/system/honeycomb.service << EOF
[Unit]
Description=Honeycomb Agent for the VPP control plane
Documentation=https://wiki.fd.io/view/Honeycomb
Requires=vpp.service
After=vpp.service

[Service]
ExecStart=/opt/honeycomb/honeycomb
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF
    systemctl enable /etc/systemd/system/honeycomb.service


    # Create systemctl service for nat script
    cat > /etc/systemd/system/nat.service << EOF
[Unit]
Description=Runs vbrg scripts for nat configuration
Requires=vpp.service
After=vpp.service

[Service]
RemainAfterExit=True
ExecStart=/opt/nat_service.sh
Restart=always

[Install]
WantedBy=multi-user.target
EOF
    systemctl enable /etc/systemd/system/nat.service

    # Download DHCP config files
    cd /opt
    wget $REPO_URL_BLOB/org.onap.demo/vnfs/vcpe/$INSTALL_SCRIPT_VERSION/v_brgemu_init.sh
    wget $REPO_URL_BLOB/org.onap.demo/vnfs/vcpe/$INSTALL_SCRIPT_VERSION/v_brgemu.sh
    sed -i '/# Provides:/c\# Provides:     vbrg ' /opt/v_brgemu.sh
    chmod +x v_brgemu_init.sh
    chmod +x v_brgemu.sh
    mv v_brgemu.sh /etc/init.d
    update-rc.d v_brgemu.sh defaults

    # Rename network interface in openstack Ubuntu 16.04 images. Then, reboot the VM to pick up changes
    if [[ $CLOUD_ENV != "rackspace" ]]
    then
        sed -i "s/GRUB_CMDLINE_LINUX=.*/GRUB_CMDLINE_LINUX=\"net.ifnames=0 biosdevname=0\"/g" /etc/default/grub
        grub-mkconfig -o /boot/grub/grub.cfg
        sed -i "s/ens[0-9]*/eth0/g" /etc/network/interfaces.d/*.cfg
        sed -i "s/ens[0-9]*/eth0/g" /etc/udev/rules.d/70-persistent-net.rules
        echo 'network: {config: disabled}' >> /etc/cloud/cloud.cfg.d/99-disable-network-config.cfg
        reboot
    fi

    ./v_brgemu_init.sh
fi  # endif BUILD_STATE != "build"
