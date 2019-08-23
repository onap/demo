package grafanadatasource

import (
	"bytes"
	"context"
	"encoding/json"

	onapv1alpha1 "visualization-operator/pkg/apis/onap/v1alpha1"

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
			return reconcile.Result{}, nil
		}
		// Error reading the object - requeue the request.

		return reconcile.Result{}, err
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

		defer getResp.Body.Close()

		//add datasource if datasource does not exist already
		if getResp.StatusCode == http.StatusNotFound {
			reqLogger.Info("Datasource does not exist, creating one...")
			// create datasource
			if err := createDataSource(grafana, datasource); err != nil {
				return reconcile.Result{}, err
			}
		} else if getResp.StatusCode == http.StatusOK {
			//if datasource already exists
			reqLogger.V(1).Info("datasource already exists", "datasource", datasource.Name)
		} else {
			reqLogger.Error(err, "unknown error", datasource.Name)
		}

	}
	return reconcile.Result{}, nil
}

func createDataSource(grafana map[string]string, datasource onapv1alpha1.Datasource) error {
	reqLogger := log.WithValues("Datasource name", datasource.Name, "Datasource URL", datasource.URL)
	reqLogger.Info("creating datasource")

	grafanaURL := grafana["url"] + "/api/datasources"
	grafanaUsername := grafana["username"]
	grafanaPassword := grafana["password"]

	client := &http.Client{}
	postBody, err := json.Marshal(datasource)
	if err != nil {
		reqLogger.Error(err, "JSON Marshalling error")
		return err
	}

	req, err := http.NewRequest("POST", grafanaURL, bytes.NewBuffer(postBody))
	if err != nil {
		reqLogger.Error(err, "POST REQUEST error")
		return err
	}
	req.Header.Set("Content-Type", "application/json")
	req.SetBasicAuth(grafanaUsername, grafanaPassword)
	postResp, err := client.Do(req)
	if err != nil {
		reqLogger.Error(err, "POST RESPONSE error")
		return err
	}
	defer req.Body.Close()

	if postResp.StatusCode == http.StatusOK {
		reqLogger.Info("Datasource created")
		return nil
	}
	return err
}
