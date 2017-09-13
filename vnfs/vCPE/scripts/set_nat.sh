#! /bin/bash

while :
do
	if [[ ! $(ps -aux | grep [[:alnum:]]*/vpp/startup.conf | wc -l) = 2 ]]; then
		echo "vpp not running"
		continue
	fi
	flag=0
	while read -r line
	do
		if [ flag = 0 ]; then
			re=${line#*/[0-9]/[0-9]}
			if [ "$line" != "$re" ]; then
				flag=1
			else
				flag=0
				continue
			fi
		else
			ip=${line%/*}
			if [[ $ip = *\.*\.*\.* ]]; then
				#echo "ip address is $ip"
				if [ ! -f /etc/vpp/ip.txt ]; then
					echo "file /etc/vpp/ip.txt doesn't exists"
					continue
				fi
				while read -r tap_ip
				do
					if [[ $tap_ip = hcip* ]]; then
						tap_ip=${tap_ip#*" "}
						echo "hc tap ip address is $tap_ip"
						vppctl snat add static mapping local $tap_ip external $ip
						exit 0
					fi
				done < /etc/vpp/ip.txt
			else
				if [[ ! $ip = */[0-9] ]]; then
					flag=0
					echo "not correct"
				fi
			fi
		fi
	done < <(vppctl show int addr)
done
