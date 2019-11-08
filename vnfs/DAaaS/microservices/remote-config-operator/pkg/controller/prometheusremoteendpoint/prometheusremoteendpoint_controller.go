package prometheusremoteendpoint

import (
	"bytes"
	"context"
	"encoding/json"
	"net/http"
	"strconv"
	"strings"

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

// Reconcile reads that state of the cluster for a PrometheusRemoteEndpoint object
// and makes changes based on the state read and what is in the PrometheusRemoteEndpoint.Spec
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

	// Check if CR is being Deleted
	if instance.GetDeletionTimestamp() != nil {
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
	pName := instance.ObjectMeta.Labels["app"]
	if err1 := r.client.Get(context.TODO(), types.NamespacedName{Namespace: instance.Namespace, Name: pName}, prom); err1 != nil {
		reqLogger.Error(err1, "Error getting prometheus")
		return err1
	}
	reqLogger.Info("Found prometheus to update")

	var patch []byte

	rws := prom.Spec.RemoteWrite
	remoteURL, id, err := getAdapterInfo(instance)
	instanceKey := types.NamespacedName{Namespace: instance.Namespace, Name: instance.Name}
	if err != nil {
		reqLogger.Error(err, "Unable to get adapter url")
		return err
	}

	isUpdate := false
	for i, spec := range rws {
		// Update event - check the prometheus remote write Url against remoteURL in the Status
		// to consider the case when URL itself is updated.
		if spec.URL == instance.Status.RemoteURL {
			reqLogger.Info("Remote write already exists, updating it")
			patch, _ = formPatch("replace", strconv.Itoa(i), remoteURL, instance, reqLogger)
			isUpdate = true
			break
		}
	}

	if !isUpdate {
		reqLogger.Info("Remote write does not exist, creating one...")
		// rwsLength := len(rws)
		patch, _ = formPatch("add", "-", remoteURL, instance, reqLogger)
	}
	patchErr := r.client.Patch(context.TODO(), prom, client.ConstantPatch(types.JSONPatchType, patch))
	if patchErr != nil {
		reqLogger.Error(patchErr, "Unable to process patch to prometheus")
		cleanUpExternalResources(instance)
		r.updateStatus("Error", instanceKey, "", "", "")
		return patchErr
	}
	r.updateStatus("Enabled", instanceKey, pName, remoteURL, id)
	reqLogger.V(1).Info("Patch merged")

	return nil
}

func (r *ReconcilePrometheusRemoteEndpoint) processDeletionRequest(reqLogger logr.Logger, instance *onapv1alpha1.PrometheusRemoteEndpoint) error {
	prom := &monitoringv1.Prometheus{}
	if err := r.client.Get(context.TODO(), types.NamespacedName{Namespace: instance.Namespace, Name: instance.ObjectMeta.Labels["app"]}, prom); err != nil {
		reqLogger.Error(err, "Error getting prometheus")
		return err
	}
	reqLogger.Info("Found prometheus to update")

	var patch []byte
	remoteURL, _, err := getAdapterInfo(instance)
	if err != nil {
		reqLogger.Error(err, "Unable to get adapter info")
		return err
	}

	rws := prom.Spec.RemoteWrite
	for i, spec := range rws {
		if spec.URL == remoteURL {
			reqLogger.Info("Found remote write to be removed, removing it")
			patch, _ = formPatch("remove", strconv.Itoa(i), remoteURL, instance, reqLogger)
			break
		}
	}
	patchErr := r.client.Patch(context.TODO(), prom, client.ConstantPatch(types.JSONPatchType, patch))
	if patchErr != nil {
		reqLogger.Error(patchErr, "Unable to process patch to prometheus")
		return patchErr
	}
	reqLogger.V(1).Info("Patch merged, remote write removed")
	cleanUpExternalResources(instance)
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

func (r *ReconcilePrometheusRemoteEndpoint) updateStatus(status string, key types.NamespacedName, prom string, remoteURL string, kwid string) error {
	// Fetch the CollectdGlobal instance
	instance := &onapv1alpha1.PrometheusRemoteEndpoint{}
	err := r.client.Get(context.TODO(), key, instance)
	if err != nil {
		return err
	}
	instance.Status.Status = status
	instance.Status.PrometheusInstance = prom
	instance.Status.KafkaWriterID = kwid
	instance.Status.RemoteURL = remoteURL
	err = r.client.Status().Update(context.TODO(), instance)
	return err
}

func cleanUpExternalResources(instance *onapv1alpha1.PrometheusRemoteEndpoint) {
	if instance.Spec.Type == "kafka" {
		deleteKafkaWriter(instance.Spec.AdapterURL + "/pkw/" + instance.Status.KafkaWriterID)
	}
}

func getAdapterInfo(instance *onapv1alpha1.PrometheusRemoteEndpoint) (remoteURL string, id string, err error) {
	switch strings.ToLower(instance.Spec.Type) {
	case "m3db":
		return instance.Spec.AdapterURL + "/api/v1/prom/remote/write", "", nil
	case "kafka":
		kwid, err := getKafkaWriter(instance)
		return instance.Spec.AdapterURL + "/pkw/" + kwid + "/receive", kwid, err
	default:
		return instance.Spec.AdapterURL, "", nil
	}
}

func deleteKafkaWriter(kwURL string) error {
	client := &http.Client{}
	req, err := http.NewRequest(http.MethodDelete, kwURL, nil)
	if err != nil {
		log.Error(err, "Failed to form delete Kafka Writer request")
		return err
	}
	_, err = client.Do(req)
	if err != nil {
		log.Error(err, "Failed to delete Kafka Writer", "Kafka Writer", kwURL)
		return err
	}
	return nil
}

func getKafkaWriter(instance *onapv1alpha1.PrometheusRemoteEndpoint) (string, error) {
	// TODO - check update events
	if instance.Status.KafkaWriterID != "" {
		return instance.Status.KafkaWriterID, nil
	}
	return createKafkaWriter(instance)
}

func createKafkaWriter(instance *onapv1alpha1.PrometheusRemoteEndpoint) (string, error) {

	log.V(1).Info("Processing Kafka Remote Endpoint", "Kafka Writer Config", instance.Spec)
	baseURL := instance.Spec.AdapterURL
	kwc := instance.Spec.KafkaConfig
	kwURL := baseURL + "/pkw"

	postBody, err := json.Marshal(kwc)
	if err != nil {
		log.Error(err, "JSON Marshalling error")
		return "", err
	}

	resp, err := http.Post(kwURL, "application/json", bytes.NewBuffer(postBody))
	if err != nil {
		log.Error(err, "Failed to create Kafka Writer", "Kafka Writer", kwURL, "Kafka Writer Config", kwc)
		return "", err
	}
	defer resp.Body.Close()
	var kwid string
	json.NewDecoder(resp.Body).Decode(&kwid)
	log.Info("Kafka Writer created", "Kafka Writer Id", kwid)

	return kwid, err
}
