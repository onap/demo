package grafanadatasource

import (
	logr "github.com/go-logr/logr"

	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"

	onapv1alpha1 "visualization-operator/pkg/apis/onap/v1alpha1"
	visualizationutils "visualization-operator/pkg/controller/utils"

	"k8s.io/apimachinery/pkg/api/errors"
	"k8s.io/apimachinery/pkg/runtime"
	"net/http"
	"sigs.k8s.io/controller-runtime/pkg/client"
	"sigs.k8s.io/controller-runtime/pkg/controller"
	"sigs.k8s.io/controller-runtime/pkg/handler"
	"sigs.k8s.io/controller-runtime/pkg/manager"
	"sigs.k8s.io/controller-runtime/pkg/reconcile"
	logf "sigs.k8s.io/controller-runtime/pkg/runtime/log"
	"sigs.k8s.io/controller-runtime/pkg/source"
)

var log = logf.Log.WithName("controller_grafanadatasource")

// Add creates a new GrafanaDataSource Controller and adds it to the Manager. The Manager will set fields on the Controller
// and Start it when the Manager is Started.
func Add(mgr manager.Manager) error {
	return add(mgr, newReconciler(mgr))
}

// newReconciler returns a new reconcile.Reconciler
func newReconciler(mgr manager.Manager) reconcile.Reconciler {
	return &ReconcileGrafanaDataSource{client: mgr.GetClient(), scheme: mgr.GetScheme()}
}

// add adds a new Controller to mgr with r as the reconcile.Reconciler
func add(mgr manager.Manager, r reconcile.Reconciler) error {
	// Create a new controller
	c, err := controller.New("grafanadatasource-controller", mgr, controller.Options{Reconciler: r})
	if err != nil {
		return err
	}

	// Watch for changes to primary resource GrafanaDataSource
	err = c.Watch(&source.Kind{Type: &onapv1alpha1.GrafanaDataSource{}}, &handler.EnqueueRequestForObject{})
	if err != nil {
		return err
	}

	return nil
}

// blank assignment to verify that ReconcileGrafanaDataSource implements reconcile.Reconciler
var _ reconcile.Reconciler = &ReconcileGrafanaDataSource{}

// ReconcileGrafanaDataSource reconciles a GrafanaDataSource object
type ReconcileGrafanaDataSource struct {
	// This client, initialized using mgr.Client() above, is a split client
	// that reads objects from the cache and writes to the apiserver
	client client.Client
	scheme *runtime.Scheme
}

// Reconcile reads that state of the cluster for a GrafanaDataSource object and makes changes based on the state read
// and what is in the GrafanaDataSource.Spec
// Note:
// The Controller will requeue the Request to be processed again if the returned error is non-nil or
// Result.Requeue is true, otherwise upon completion it will remove the work from the queue.
func (r *ReconcileGrafanaDataSource) Reconcile(request reconcile.Request) (reconcile.Result, error) {
	reqLogger := log.WithValues("Request.Namespace", request.Namespace, "Request.Name", request.Name)
	reqLogger.Info("Reconciling GrafanaDataSource")

	// Fetch the GrafanaDataSource instance
	instance := &onapv1alpha1.GrafanaDataSource{}
	err := r.client.Get(context.TODO(), request.NamespacedName, instance)
	if err != nil {
		if errors.IsNotFound(err) {
			// Request object not found, could have been deleted after reconcile request.
			// Owned objects are automatically garbage collected. For additional cleanup logic use finalizers.
			// Return and don't requeue
			reqLogger.Info("GrafanaDatasource object not found")
			return reconcile.Result{}, nil
		}
		// Error reading the object - requeue the request.
		reqLogger.Info("Error reading the Grafanadatasource object, Requeing")
		return reconcile.Result{}, err
	}

	//Check if deletion timestamp is set. If yes, delete the GrafanaDataSource object
	isBeingDeleted := checkDeletionTimestamp(reqLogger, instance)
	if isBeingDeleted {
		//Delete the datasource from grafana
		err := deleteDatasource(instance)
		if err != nil {
			reqLogger.Error(err, "Unable to delete datasource")
			return reconcile.Result{}, err
		}
		//remove Finalizer after deletion
		if visualizationutils.Contains(instance.GetFinalizers(), visualizationutils.VisualizationFinalizer) {
			if err := removeFinalizer(reqLogger, instance); err != nil {
				return reconcile.Result{}, err
			}
			err := r.client.Update(context.TODO(), instance)
			if err != nil {
				reqLogger.Error(err, "Unable to update instance")
				return reconcile.Result{}, err
			}
			return reconcile.Result{}, nil
		}
	}

	//Add finalizer for the CR object
	if !visualizationutils.Contains(instance.GetFinalizers(), visualizationutils.VisualizationFinalizer) {
		reqLogger.Info("Adding finalizer for GrafanaDatasource")
		if err := addFinalizer(reqLogger, instance); err != nil {
			return reconcile.Result{}, err
		}
		err := r.client.Update(context.TODO(), instance)
		if err != nil {
			reqLogger.Error(err, "Unable to update instance")
			return reconcile.Result{}, err
		}
		return reconcile.Result{}, nil
	}

	datasources := instance.Spec.Datasources
	grafana := instance.Spec.Grafana

	reqLogger.V(1).Info(" Datasource Name ", "datasources", datasources)

	//loop through all datasources in the spec
	for _, datasource := range datasources {

		//check if datasource exists
		grafanaURL := grafana["url"] + "/api/datasources/name/" + datasource.Name
		grafanaUsername := grafana["username"]
		grafanaPassword := grafana["password"]

		client := &http.Client{}
		req, err := http.NewRequest("GET", grafanaURL, nil)
		if err != nil {
			reqLogger.Error(err, "GET REQUEST error")
			return reconcile.Result{}, err
		}
		req.SetBasicAuth(grafanaUsername, grafanaPassword)
		getResp, err := client.Do(req)
		if err != nil {
			reqLogger.Error(err, "GET RESPONSE error")
			return reconcile.Result{}, err
		}

		respBody, err := ioutil.ReadAll(getResp.Body)
		if err != nil {
			reqLogger.Error(err, "Response data not read properly")
			return reconcile.Result{}, err
		}

		respBodyBytes := []byte(respBody)
		var respData map[string]interface{}

		if err := json.Unmarshal(respBodyBytes, &respData); err != nil {
			reqLogger.Error(err, "JSON unmarshalling error")
			return reconcile.Result{}, err
		}

		respURL := fmt.Sprintf("%v", respData["url"])
		respID := fmt.Sprintf("%v", respData["id"])
		respIsDefault := respData["isDefault"]
		respAccess := fmt.Sprintf("%v", respData["access"])

		defer getResp.Body.Close()

		//add datasource if datasource does not exist already
		if getResp.StatusCode == http.StatusNotFound {
			reqLogger.Info("Datasource does not exist, creating one...")
			// create datasource
			if err := createDataSource(grafana, datasource); err != nil {
				return reconcile.Result{}, err
			}
		} else if getResp.StatusCode == http.StatusOK {
			//if datasource already exists and there is any change in the spec - update it
			reqLogger.V(1).Info("datasource already exists", "datasource", datasource.Name)
			if respURL != datasource.URL || respIsDefault.(bool) != datasource.IsDefault || respAccess != datasource.Access {
				if err := updateDatasource(grafana, datasource, respID); err != nil {
					return reconcile.Result{}, err
				}
			} else {
				reqLogger.Info("No creation/updation of datasource needed")
				return reconcile.Result{}, nil
			}
		}
	}
	return reconcile.Result{}, nil
}

func createDataSource(grafana map[string]string, datasource onapv1alpha1.Datasource) error {
	reqLogger := log.WithValues("Datasource name", datasource.Name, "Datasource URL", datasource.URL)
	reqLogger.Info("Creating datasource")

	grafanaURL := grafana["url"] + "/api/datasources"
	grafanaUsername := grafana["username"]
	grafanaPassword := grafana["password"]

	client := &http.Client{}
	postBody, err := json.Marshal(datasource)
	if err != nil {
		reqLogger.Error(err, "JSON Marshalling error")
		return err
	}

	postReq, err := http.NewRequest("POST", grafanaURL, bytes.NewBuffer(postBody))
	if err != nil {
		reqLogger.Error(err, "POST REQUEST error")
		return err
	}
	postReq.Header.Set("Content-Type", "application/json")
	postReq.SetBasicAuth(grafanaUsername, grafanaPassword)
	postResp, err := client.Do(postReq)
	if err != nil {
		reqLogger.Error(err, "POST RESPONSE error")
		return err
	}
	defer postReq.Body.Close()

	if postResp.StatusCode == http.StatusOK {
		reqLogger.Info("Datasource created")
		return nil
	}
	return err
}

func updateDatasource(grafana map[string]string, datasource onapv1alpha1.Datasource, datasourceID string) error {
	reqLogger := log.WithValues("Datasource name", datasource.Name, "Datasource URL", datasource.URL)
	reqLogger.Info("Updating datasource")

	grafanaURL := grafana["url"] + "/api/datasources/" + datasourceID
	grafanaUsername := grafana["username"]
	grafanaPassword := grafana["password"]

	client := &http.Client{}
	putBody, err := json.Marshal(datasource)
	if err != nil {
		reqLogger.Error(err, "JSON Marshalling error")
		return err
	}
	putReq, err := http.NewRequest("PUT", grafanaURL, bytes.NewBuffer(putBody))
	if err != nil {
		reqLogger.Error(err, "PUT REQUEST error")
		return err
	}
	putReq.Header.Set("Content-Type", "application/json")
	putReq.Header.Set("Accept", "application/json")
	putReq.SetBasicAuth(grafanaUsername, grafanaPassword)

	putResp, err := client.Do(putReq)
	if err != nil {
		reqLogger.Error(err, "PUT RESPONSE error")
		return err
	}
	defer putReq.Body.Close()

	if putResp.StatusCode == http.StatusOK {
		reqLogger.Info("Datasource updated")
		return nil
	}
	return err
}

func deleteDatasource(instance *onapv1alpha1.GrafanaDataSource) error {

	datasources := instance.Spec.Datasources
	grafana := instance.Spec.Grafana

	for _, datasource := range datasources {

		reqLogger := log.WithValues("Datasource name", datasource.Name, "Datasource URL", datasource.URL)
		reqLogger.Info("Deleting datasource")

		grafanaURL := grafana["url"] + "/api/datasources/name/" + datasource.Name
		grafanaUsername := grafana["username"]
		grafanaPassword := grafana["password"]

		client := &http.Client{}
		deleteReq, err := http.NewRequest("DELETE", grafanaURL, nil)
		if err != nil {
			reqLogger.Error(err, "DELETE request error")
			return err
		}

		deleteReq.SetBasicAuth(grafanaUsername, grafanaPassword)

		deleteResp, err := client.Do(deleteReq)
		if err != nil {
			reqLogger.Error(err, "DELETE RESPONSE error")
			return err
		}

		if deleteResp.StatusCode == http.StatusOK {
			reqLogger.Info("Datasource deleted")
			return nil
		}
		return err
	}
	return nil
}

func checkDeletionTimestamp(reqlogger logr.Logger, instance *onapv1alpha1.GrafanaDataSource) bool {
	isMarkedForDeletion := instance.GetDeletionTimestamp() != nil
	return isMarkedForDeletion
}

func addFinalizer(reqlogger logr.Logger, instance *onapv1alpha1.GrafanaDataSource) error {
	reqlogger.Info("Adding finalizer for the GrafanaDatasource")
	instance.SetFinalizers(append(instance.GetFinalizers(), visualizationutils.VisualizationFinalizer))
	return nil
}

func removeFinalizer(reqlogger logr.Logger, instance *onapv1alpha1.GrafanaDataSource) error {
	reqlogger.Info("Removing finalizer for the GrafanaDatasource")
	instance.SetFinalizers(visualizationutils.Remove(instance.GetFinalizers(), visualizationutils.VisualizationFinalizer))
	return nil
}
