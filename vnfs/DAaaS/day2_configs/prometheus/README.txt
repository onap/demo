This day2 config adds a remote_write to existing config

Assuming initial/day0 config in namespace "edge1" and helm release name as "cp" (helm install -n cp collection/ --namespace=edge1)

Run below command to apply day2 config

kubectl patch --namespace=edge1 prometheus cp-prometheus-prometheus -p "$(cat add_remote_write.yaml)" --type=merge
