#!/bin/sh
set -ex

SOCKS5_IP=${SOCKS5_IP:-127.0.0.1}
SOCKS5_PORT=${SOCKS5:-1080}
DNS_SERVER=${DNS_SERVER:-10.248.2.1}

sudo apt-get install -y redsocks iptables

cat <<EOF | sudo tee /etc/redsocks.conf
base {
 log_debug = on;
 log_info = on;
 log = "file:/root/proxy.log";
 daemon = on;
 redirector = iptables;
}

redsocks {
 local_ip = 0.0.0.0;
 local_port = 6666;
 ip = $SOCKS5_IP;
 port = $SOCKS5_PORT;
 type = socks5;
}
EOF

echo 1 | sudo tee /proc/sys/net/ipv4/ip_forward
sudo iptables -t filter -F
sudo iptables -t mangle -F
sudo iptables -t nat -F

sudo iptables -t nat -N REDSOCKS || true
sudo iptables -t nat -A REDSOCKS -d 0.0.0.0/16 -j RETURN
sudo iptables -t nat -A REDSOCKS -d 10.0.0.0/16 -j RETURN
sudo iptables -t nat -A REDSOCKS -d 127.0.0.0/16 -j RETURN
sudo iptables -t nat -A REDSOCKS -d 169.254.0.0/16 -j RETURN
sudo iptables -t nat -A REDSOCKS -d 172.16.0.0/12 -j RETURN
sudo iptables -t nat -A REDSOCKS -d 192.168.0.0/16 -j RETURN
sudo iptables -t nat -A REDSOCKS -d 224.0.0.0/4 -j RETURN
sudo iptables -t nat -A REDSOCKS -d 240.0.0.0/4 -j RETURN
sudo iptables -t nat -A REDSOCKS -p tcp -j REDIRECT --to-ports 6666
sudo iptables -t nat -A OUTPUT -p tcp  -j REDSOCKS
sudo iptables -t nat -A PREROUTING -p tcp  -j REDSOCKS
sudo iptables -t nat -A POSTROUTING -s 192.168.1.0/24 -j MASQUERADE
sudo iptables -t nat -A PREROUTING  -p udp --dport 53  -j DNAT --to-destination $DNS_SERVER

sudo service redsocks restart
wget --no-proxy  www.sina.com.cn -O /dev/null
