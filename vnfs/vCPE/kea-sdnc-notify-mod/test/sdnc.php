###################################################################################
# simple php script to act as sdnc emulator for testing DHCP notifications
#
# License
# -------
#
# Copyright (C) 2017 AT&T Intellectual Property. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#        http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
###################################################################################

<?php
header('Content-type: application/json');
$date = new DateTime();
$date = $date->format("y:m:d h:i:s");
$req_dump = print_r( $_REQUEST, true );
$fp = file_put_contents( '/var/log/onap/macaddr.log', $date . "|" . $req_dump . "|" ,FILE_APPEND | LOCK_EX );
$fp = file_put_contents( '/var/log/onap/macaddr.log', file_get_contents('php://input'),FILE_APPEND | LOCK_EX );
$fp = file_put_contents( '/var/log/onap/macaddr.log', "\n",FILE_APPEND | LOCK_EX );

print "{\"succes\"}";
?>
