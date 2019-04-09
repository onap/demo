#!/bin/bash
set -x

# Convert Network CIDR to Netmask
mask2cidr() {
    nbits=0
    IFS=.
    for dec in $1 ; do
        case $dec in
            255) let nbits+=8;;
            254) let nbits+=7;;
            252) let nbits+=6;;
            248) let nbits+=5;;
            240) let nbits+=4;;
            224) let nbits+=3;;
            192) let nbits+=2;;
            128) let nbits+=1;;
            0);;
            *) echo "Error: $dec is not recognized"; exit 1
        esac
    done
    echo "$nbits"
}

# Start VPP
if ! which start; then
	echo "#!/bin/bash" > /usr/local/sbin/start
	echo "systemctl start \$1" >> /usr/local/sbin/start
	chmod u+x /usr/local/sbin/start
fi
start vpp
sleep 1

# Configure VPP for vFirewall
IPADDR1=$(ifconfig enp2s0 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
IPADDR2=$(ifconfig enp3s0 | grep "inet addr" | tr -s ' ' | cut -d' ' -f3 | cut -d':' -f2)
HWADDR1=$(ifconfig enp2s0 | grep -Po "HWaddr \K(.*)")
HWADDR2=$(ifconfig enp3s0 | grep -Po "HWaddr \K(.*)")
FAKE_HWADDR1=$(echo -n 00; dd bs=1 count=5 if=/dev/urandom 2>/dev/null | hexdump -v -e '/1 ":%02X"')
FAKE_HWADDR2=$(echo -n 00; dd bs=1 count=5 if=/dev/urandom 2>/dev/null | hexdump -v -e '/1 ":%02X"')

IPADDR1_MASK=$(ifconfig enp2s0 | grep "Mask" | awk '{print $4}' | awk -F ":" '{print $2}')
IPADDR1_CIDR=$(mask2cidr $IPADDR1_MASK)
IPADDR2_MASK=$(ifconfig enp3s0 | grep "Mask" | awk '{print $4}' | awk -F ":" '{print $2}')
IPADDR2_CIDR=$(mask2cidr $IPADDR2_MASK)

ifconfig enp2s0 down
ifconfig enp3s0 down
ifconfig enp2s0 hw ether $FAKE_HWADDR1
ifconfig enp3s0 hw ether $FAKE_HWADDR2
ip addr flush dev enp2s0
ip addr flush dev enp3s0
ifconfig enp2s0 up
ifconfig enp3s0 up
vppctl tap connect tap111 hwaddr $HWADDR1
vppctl tap connect tap222 hwaddr $HWADDR2
vppctl set int ip address tapcli-0 $IPADDR1"/"$IPADDR1_CIDR
vppctl set int ip address tapcli-1 $IPADDR2"/"$IPADDR2_CIDR
vppctl set int state tapcli-0 up
vppctl set int state tapcli-1 up
brctl addbr br0
brctl addif br0 tap111
brctl addif br0 enp2s0
brctl addbr br1
brctl addif br1 tap222
brctl addif br1 enp3s0
ifconfig br0 up
ifconfig br1 up
sleep 1

# Start HoneyComb
VERSION=$(cat /opt/config/demo_artifacts_version.txt)
mkdir -p /var/lib/honeycomb/persist/{config,context}/
echo "" > /var/lib/honeycomb/persist/context/data.json
echo "" > /var/lib/honeycomb/persist/config/data.json
# /opt/honeycomb/sample-distribution-$VERSION/honeycomb &>/dev/null &disown
systemctl restart honeycomb
sleep 1

# Start VES client
cd /opt/VES/evel/evel-library/code/VESreporting/
./go-client.sh &>/dev/null &disown
