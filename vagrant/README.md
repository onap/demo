Demo
====

ONAP demo is created by vagrant. It is verified to work in Ubuntu 16.04 64bit
with 125G memory & 1T disk.

Setup
-----

./run_demo.sh

Cleanup
-------

vagrant destroy -f

Proxy
-----

Proxy setup is optional. If setup is behind sock5 proxy, SOCKS5_IP, SOCKS5_PORT
and DNS_SERVER is updated and ./setup_proxy is run before running demo.
