from fabric.api import *
from fabric.context_managers import *

def uptime():
    res = run('cat /proc/uptime')
    print res

def host_type():
	run('uname -s')

def upgrade_docker(server_name, my_env):
	server_name=server_name.split("-")[1]
	if server_name in "message-router":
		execute_file = "/opt/mr_vm_init.sh"
	elif server_name in "dcae-controller":
		execute_file = "/opt/dcae2_vm_init.sh"
    	elif server_name in "openo-server":
		execute_file = "/opt/openo_all_serv.sh" 
	elif server_name in "dns":
		execute_file = None
	else:
		execute_file = "/opt/" + server_name + "_vm_init.sh"
	print "Executing file %s" % execute_file
	if execute_file:
		with settings( warn_only=True, key_filename=my_env['key_filename'], user=my_env['user']):
			sudo(execute_file)

def health_checks_robot(my_env):
	with settings( warn_only=True, key_filename=my_env['key_filename'], user=my_env['user']):
		with cd('/opt'):
			sudo('./ete.sh health')
			
