


# sdnc emulator
# install in /var/www/html with standard apache2 and php install
# create /var/log/onap directoy
# set owner to 'www-data'  on /var/log/onap and /var/log/onap/macaddr.log

# test with curl http://localhost/sdnc.php?macaddr=11:22:33:44
# or 
#  curl -H "Content-Type: application/json" -X POST -d '{"username":"xyz","password":"xyz"}' http://localhost/sdnc.php?macaddr=12:34:56:66
#
# response is {"succes"}
#
# macaddr.log should record the data submitted

