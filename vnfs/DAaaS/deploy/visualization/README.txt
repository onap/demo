#Add datasource
Modify "url" in the datasources section of ./charts/grafana/grafana-values.yaml based on the Prometheus or any other service created

#copy the custom dashboards definition into the folder ./charts/grafana/dashboards
Example, 
 	cp ~/dashboard1.json  ./charts/grafana/dashboards/

#create configmap that imports .json file as config
configmap found at ./charts/grafana/templates/configmap-add-dashboard.yaml

#Install visualization helm package

helm install -n graf-prometheus . -f ./charts/grafana/values.yaml -f ./charts/grafana/grafana-values.yaml

