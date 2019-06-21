1. Day2 config to add a remote_write to existing config

Assuming initial/day0 config in namespace "edge1" and helm release name as "cp" (helm install -n cp collection/ --namespace=edge1)

Run below command to apply day2 config

kubectl patch --namespace=edge1 prometheus cp-prometheus-prometheus -p "$(cat add_remote_write.yaml)" --type=merge

2. Day2 config to remove remote_read and remote_write from existing config

Assuming initial/day0 config in namespace "edge1" and helm release name as "cp" (helm install -n cp collection/ --namespace=edge1)

Run below commands to apply day2 config

To remove remote_read ----->  kubectl patch -n edge1 prometheus cp-prometheus-prometheus --type=json -p="$(cat remove_remote_read.json)"
To remove remote_write ---->  kubectl patch -n edge1 prometheus cp-prometheus-prometheus --type=json -p="$(cat remove_remote_write.json)"
