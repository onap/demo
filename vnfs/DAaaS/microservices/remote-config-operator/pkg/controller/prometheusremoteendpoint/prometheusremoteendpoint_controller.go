package prometheusremoteendpoint

import (
	"context"
	"encoding/json"
	"strconv"

	onapv1alpha1 "remote-config-operator/pkg/apis/onap/v1alpha1"

	monitoringv1 "github.com/coreos/prometheus-operator/pkg/apis/monitoring/v1"
	logr "github.com/go-logr/logr"
	corev1 "k8s.io/api/core/v1"
	"k8s.io/apimachinery/pkg/api/errors"
	"k8s.io/apimachinery/pkg/runtime"
	"k8s.io/apimachinery/pkg/types"
	remoteconfigutils "remote-config-operator/pkg/controller/utils"
	"sigs.k8s.io/controller-runtime/pkg/client"
	"sigs.k8s.io/controller-runtime/pkg/controller"
	"sigs.k8s.io/controller-runtime/pkg/handler"
	logf "sigs.k8s.io/controller-runtime/pkg/log"
	"sigs.k8s.io/controller-runtime/pkg/manager"
	"sigs.k8s.io/controller-runtime/pkg/reconcile"
	"sigs.k8s.io/controller-runtime/pkg/source"
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

	log.V(1).Info("Add watcher for primary resource PrometheusRemoteEndpoint")
	err = c.Watch(&source.Kind{Type: &onapv1alpha1.PrometheusRemoteEndpoint{}}, &handler.EnqueueRequestForObject{})
	if err != nil {
		return err
	}

	log.V(1).Info("Add watcher for secondary resource RemoteFilterAction")
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
		reqLogger.Error(err, "Error reading PrometheusRemoteEndpoint object, Requeing ")
		return reconcile.Result{}, err
	}

	isBeingDeleted := checkDeletionTimestamp(reqLogger, instance)
	if isBeingDeleted {
		//Delete Remote write
		if err := r.processDeletionRequest(reqLogger, instance); err != nil {
			reqLogger.Error(err, "Error processing deletion request")
			return reconcile.Result{}, err
		}
		return reconcile.Result{}, nil
	}

	//Add finalizer for the CR object
	if !remoteconfigutils.Contains(instance.GetFinalizers(), remoteconfigutils.RemoteConfigFinalizer) {
		reqLogger.Info("Adding finalizer for PrometheusRemoteEndpoint")
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

	if err := r.processPatchRequest(reqLogger, instance); err != nil {
		reqLogger.Error(err, "Error processing request")
		return reconcile.Result{}, err
	}
	return reconcile.Result{}, nil
}

func (r *ReconcilePrometheusRemoteEndpoint) processPatchRequest(reqLogger logr.Logger, instance *onapv1alpha1.PrometheusRemoteEndpoint) error {

	prom := &monitoringv1.Prometheus{}
	if err1 := r.client.Get(context.TODO(), types.NamespacedName{Namespace: instance.Namespace, Name: instance.ObjectMeta.Labels["app"]}, prom); err1 != nil {
		reqLogger.Error(err1, "Error getting prometheus")
		return err1
	}
	reqLogger.Info("Found prometheus to update")

	var patch []byte

	rws := prom.Spec.RemoteWrite
	adapterURL := instance.Spec.AdapterUrl

	isUpdate := false
	for i, spec := range rws {
		if spec.URL == instance.Spec.AdapterUrl {
			reqLogger.Info("Remote write already exists, updating it")
			patch, _ = formPatch("replace", strconv.Itoa(i), adapterURL, instance, reqLogger)
			isUpdate = true
			break
		}
	}

	if !isUpdate {
		reqLogger.Info("Remote write does not exist, creating one...")
		// rwsLength := len(rws)
		patch, _ = formPatch("add", "-", adapterURL, instance, reqLogger)
	}
	patchErr := r.client.Patch(context.TODO(), prom, client.ConstantPatch(types.JSONPatchType, patch))
	if patchErr != nil {
		reqLogger.Error(patchErr, "Unable to process patch to prometheus")
		return patchErr
	}
	reqLogger.Info("Patch merged")

	return nil
}

func checkDeletionTimestamp(reqlogger logr.Logger, instance *onapv1alpha1.PrometheusRemoteEndpoint) bool {
	isMarkedForDeletion := instance.GetDeletionTimestamp() != nil
	return isMarkedForDeletion
}

func (r *ReconcilePrometheusRemoteEndpoint) processDeletionRequest(reqLogger logr.Logger, instance *onapv1alpha1.PrometheusRemoteEndpoint) error {
	prom := &monitoringv1.Prometheus{}
	if err := r.client.Get(context.TODO(), types.NamespacedName{Namespace: instance.Namespace, Name: instance.ObjectMeta.Labels["app"]}, prom); err != nil {
		reqLogger.Error(err, "Error getting prometheus")
		return err
	}
	reqLogger.Info("Found prometheus to update")

	var patch []byte
	adapterURL := instance.Spec.AdapterUrl

	rws := prom.Spec.RemoteWrite
	for i, spec := range rws {
		if spec.URL == instance.Spec.AdapterUrl {
			reqLogger.Info("Found remote write to be removed, removing it")
			patch, _ = formPatch("remove", strconv.Itoa(i), adapterURL, instance, reqLogger)
			break
		}
	}
	patchErr := r.client.Patch(context.TODO(), prom, client.ConstantPatch(types.JSONPatchType, patch))
	if patchErr != nil {
		reqLogger.Error(patchErr, "Unable to process patch to prometheus")
		return patchErr
	}
	reqLogger.Info("Patch merged, remote write removed")

	//remove Finalizer after deletion
	if remoteconfigutils.Contains(instance.GetFinalizers(), remoteconfigutils.RemoteConfigFinalizer) {
		if err := removeFinalizer(reqLogger, instance); err != nil {
			return err
		}
		err := r.client.Update(context.TODO(), instance)
		if err != nil {
			reqLogger.Error(err, "Unable to update instance")
			return err
		}
	}
	return nil
}

func addFinalizer(reqlogger logr.Logger, instance *onapv1alpha1.PrometheusRemoteEndpoint) error {
	reqlogger.Info("Adding finalizer for the PrometheusRemoteEndpoint")
	instance.SetFinalizers(append(instance.GetFinalizers(), remoteconfigutils.RemoteConfigFinalizer))
	return nil
}

func removeFinalizer(reqlogger logr.Logger, instance *onapv1alpha1.PrometheusRemoteEndpoint) error {
	reqlogger.Info("Removing finalizer for the PrometheusRemoteEndpoint")
	instance.SetFinalizers(remoteconfigutils.Remove(instance.GetFinalizers(), remoteconfigutils.RemoteConfigFinalizer))
	return nil
}

func formPatch(method string, index string, adapterURL string, instance *onapv1alpha1.PrometheusRemoteEndpoint, reqLogger logr.Logger) ([]byte, error) {
	var err error
	var mergePatch []byte
	path := "/spec/remoteWrite/" + index
	mergePatch, err = json.Marshal(map[string]interface{}{
		"op":   method,
		"path": path,
		"value": map[string]interface{}{
			"url":           adapterURL,
			"remoteTimeout": instance.Spec.RemoteTimeout,
		},
	})
	if err != nil {
		reqLogger.Error(err, "Unable to form patch")
		return nil, err
	}
	prependMergePatch := append([]byte{91}, mergePatch...)
	finalMergePatch := append(prependMergePatch, 93)
	return finalMergePatch, nil
}
