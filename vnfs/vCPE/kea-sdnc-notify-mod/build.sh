rm -rf  ./build
mkdir ./build
g++ -ggdb -I /usr/include/kea -L /usr/lib/kea/lib -fpic -shared -o ./build/kea-sdnc-notify.so \
  src/load_unload.cc src/pkt4_send.cc src/version.cc \
  -lkea-dhcpsrv -lkea-dhcp++ -lkea-hooks -lkea-log -lkea-util \
  -lkea-exceptions -lcurl
