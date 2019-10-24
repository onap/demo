package prometheusremoteendpoint

import (
	"context"
	"encoding/json"

	onapv1alpha1 "remote-config-operator/pkg/apis/onap/v1alpha1"

	monitoringv1 "github.com/coreos/prometheus-operator/pkg/apis/monitoring/v1"
	// "github.com/coreos/prometheus-operator/pkg/prometheus"
	logr "github.com/go-logr/logr"
	corev1 "k8s.io/api/core/v1"
	"k8s.io/apimachinery/pkg/api/errors"
	"k8s.io/apimachinery/pkg/runtime"
	"k8s.io/apimachinery/pkg/types"
	"sigs.k8s.io/controller-runtime/pkg/client"
	"sigs.k8s.io/controller-runtime/pkg/controller"
	"sigs.k8s.io/controller-runtime/pkg/handler"
	logf "sigs.k8s.io/controller-runtime/pkg/log"
	"sigs.k8s.io/controller-runtime/pkg/manager"
	"sigs.k8s.io/controller-runtime/pkg/reconcile"
	"sigs.k8s.io/controller-runtime/pkg/source"
	// metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	// appsv1 "k8s.io/api/apps/v1"
)

var log = logf.Log.WithName("controller_prometheusremoteendpoint")

// Add creates a new PrometheusRemoteEndpoint Controller and adds it to the Manager. The Manager will set fields on the Controller
// and Start it when the Manager is Started.
func Add(mgr manager.Manager) error {
	return add(mgr, newReconciler(mgr))
}

// newReconciler returns a new reconcile.Reconciler
func newReconciler(mgr manager.Manager) reconcile.Reconciler {
	return &ReconcilePrometheusRemoteEndpoint{client: mgr.GetClient(), scheme: mgr.GetScheme()}
}

// add adds a new Controller to mgr with r as the reconcile.Reconciler
func add(mgr manager.Manager, r reconcile.Reconciler) error {
	// Create a new controller
	c, err := controller.New("prometheusremoteendpoint-controller", mgr, controller.Options{Reconciler: r})
	if err != nil {
		return err
	}

	// Watch for changes to primary resource PrometheusRemoteEndpoint
	err = c.Watch(&source.Kind{Type: &onapv1alpha1.PrometheusRemoteEndpoint{}}, &handler.EnqueueRequestForObject{})
	if err != nil {
		return err
	}

	// TODO(user): Modify this to be the types you create that are owned by the primary resource
	// Watch for changes to secondary resource Pods and requeue the owner PrometheusRemoteEndpoint
	err = c.Watch(&source.Kind{Type: &corev1.Pod{}}, &handler.EnqueueRequestForOwner{
		IsController: true,
		OwnerType:    &onapv1alpha1.PrometheusRemoteEndpoint{},
	})
	if err != nil {
		log.Error(err, "Error enqueuing requests due to remoteFilterAction changes")
		return err
	}

	log.Info("Enqueued reconcile requests due to remoteFilterAction changes")
	return nil
}

// blank assignment to verify that ReconcilePrometheusRemoteEndpoint implements reconcile.Reconciler
var _ reconcile.Reconciler = &ReconcilePrometheusRemoteEndpoint{}

// ReconcilePrometheusRemoteEndpoint reconciles a PrometheusRemoteEndpoint object
type ReconcilePrometheusRemoteEndpoint struct {
	// This client, initialized using mgr.Client() above, is a split client
	// that reads objects from the cache and writes to the apiserver
	client client.Client
	scheme *runtime.Scheme
}

// Reconcile reads that state of the cluster for a PrometheusRemoteEndpoint object and makes changes based on the state read
// and what is in the PrometheusRemoteEndpoint.Spec

// Note:
// The Controller will requeue the Request to be processed again if the returned error is non-nil or
// Result.Requeue is true, otherwise upon completion it will remove the work from the queue.
func (r *ReconcilePrometheusRemoteEndpoint) Reconcile(request reconcile.Request) (reconcile.Result, error) {
	reqLogger := log.WithValues("Request.Namespace", request.Namespace, "Request.Name", request.Name)
	reqLogger.Info("Reconciling PrometheusRemoteEndpoint")

	// Fetch the PrometheusRemoteEndpoint instance
	instance := &onapv1alpha1.PrometheusRemoteEndpoint{}
	err := r.client.Get(context.TODO(), request.NamespacedName, instance)
	if err != nil {
		if errors.IsNotFound(err) {
			// Request object not found, could have been deleted after reconcile request.
			// Owned objects are automatically garbage collected. For additional cleanup logic use finalizers.
			// Return and don't requeue
			reqLogger.Error(err, "PrometheusRemoteEndpoint object not found")
			return reconcile.Result{}, nil
		}
		// Error reading the object - requeue the request.
		reqLogger.Error(err, "Error reading PrometheusRemoteEndpoint object ")
		return reconcile.Result{}, err
	}
	remoteEndpoint := instance.Spec

	//check for type of storage and process accordingly
	if remoteEndpoint.Type == "m3db" {
		if err := r.processM3dbRequest(reqLogger, instance); err != nil {
			reqLogger.Error(err, "Error processing M3DB request")
			return reconcile.Result{}, err
		}
	} else if remoteEndpoint.Type == "kafka" {
		if err := r.processKafkaRequest(reqLogger); err != nil {
			reqLogger.Error(err, "Error processing Kafka request")
			return reconcile.Result{}, err
		}
	} else {
		reqLogger.Error(err, "Inappropriate type of storage")
		return reconcile.Result{}, err
	}

	return reconcile.Result{}, nil
}

func (r *ReconcilePrometheusRemoteEndpoint) processM3dbRequest(reqLogger logr.Logger, instance *onapv1alpha1.PrometheusRemoteEndpoint) error {
	prom := &monitoringv1.Prometheus{}
	
	if err1 := r.client.Get(context.TODO(), types.NamespacedName{Namespace: "edge1", Name: "cp-prometheus-prometheus"}, prom); err1 != nil {
		reqLogger.Error(err1, "Error getting prometheus")
	}

	var err error
	var mergePatch []byte

	mergePatch, err = json.Marshal(map[string]interface{}{
		"op":   "replace",
		"path": "/spec/remoteWrite/0",
		"value": map[string]interface{}{
			"url":           instance.Spec.AdapterUrl,
			"remoteTimeout": instance.Spec.RemoteTimeout,
		},
	})

	prependMergePatch := append([]byte{91}, mergePatch...)
	finalMergePatch := append(prependMergePatch, 93)
	patchErr := r.client.Patch(context.TODO(), prom, client.ConstantPatch(types.JSONPatchType, finalMergePatch))
	if patchErr != nil {
		reqLogger.Error(err, "Unable to process patch to prometheus")
		return err
	}
	return nil
}

func (r *ReconcilePrometheusRemoteEndpoint) processKafkaRequest(reqLogger logr.Logger) error {

	return nil
}
