Assuming initial/day0 config in namespace "edge1" and helm release name as "cp" (helm install -n cp collection/ --namespace=edge1)

*******************Day2 config (add more plugins)
This day2 config adds cpu, memory and cpufreq plugins to existing config

Run below commands to apply day2 config

1. kubectl patch --namespace=edge1 configmaps cp-collectd-config -p "$(cat add_plugins.yaml)"

2. Restart pods: kubectl delete pods --namespace=edge1 cp-collectd-db7mf cp-collectd-pfc9t cp-collectd-sqjvq


************Day3 config (replace image being used)
1. kubectl patch --namespace=edge1 daemonset cp-collectd -p "$(cat replace_image.yaml)"
