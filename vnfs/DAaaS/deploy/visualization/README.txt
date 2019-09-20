### To add a datasource
Modify "url" in the datasources section of ./charts/grafana/grafana-values.yaml based on the Prometheus or any other service created


### To add a custom dashboard
1. Place the custom dashboard definition inside the folder ./charts/grafana/dashboards
Example dashboard definition can be found at ./charts/grafana/dashboards/dashboard1.json
2. create a configmap.yaml that imports above created dashboard1.json file as config and place it insdie the folder ./charts/grafana/templates/
Example configmap can be found at ./charts/grafana/templates/configmap-add-dashboard.yaml
3. Add custom dashboard configuration to values.yaml or an overriding values.yaml. Example configuration can be found in the "dashboardProviders" section of grafana-values.yaml
4. For a fresh install of visualization package, do "helm install"
   e.g., helm install -n viz . -f ./values.yaml -f ./grafana-values.yaml
   If the custom dashboard is being added to an already existing Grafana, do "helm upgrade"
   e.g., helm upgrade -n viz . -f ./values.yaml -f ./grafana-values.yaml -f ......