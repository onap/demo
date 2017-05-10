Vagrant
=======

vagrant is to create ONAP demo. It is verified to work in the env:
   Host: Ubuntu 16.04 64bit with 125G memory & 256G disk
   Vagrant: 1.8.7
   Virtualbox: 5.1.10

Vagrant Setup
-------------

sudo apt-get install -y virtualbox
wget --no-check-certificate https://releases.hashicorp.com/vagrant/1.8.7/vagrant_1.8.7_x86_64.deb
sudo dpkg -i vagrant_1.8.7_x86_64.deb

ONAP Setup
----------

"vagrant up" is to create openstack cluster.
"vagrant ssh control -c '/vagrant/create_onap.sh'" is to create ONAP after
openstack is up.


ONAP Cleanup
------------

vagrant destroy -f
