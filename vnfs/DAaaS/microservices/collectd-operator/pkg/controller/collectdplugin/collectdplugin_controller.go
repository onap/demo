package collectdplugin

import (
	"context"
	"crypto/sha256"
	"fmt"
	"time"

	onapv1alpha1 "demo/vnfs/DAaaS/microservices/collectd-operator/pkg/apis/onap/v1alpha1"

	corev1 "k8s.io/api/core/v1"
	extensionsv1beta1 "k8s.io/api/extensions/v1beta1"
	"k8s.io/apimachinery/pkg/api/errors"
	"k8s.io/apimachinery/pkg/runtime"
	"sigs.k8s.io/controller-runtime/pkg/client"
	"sigs.k8s.io/controller-runtime/pkg/controller"
	"sigs.k8s.io/controller-runtime/pkg/handler"
	"sigs.k8s.io/controller-runtime/pkg/manager"
	"sigs.k8s.io/controller-runtime/pkg/reconcile"
	logf "sigs.k8s.io/controller-runtime/pkg/runtime/log"
	"sigs.k8s.io/controller-runtime/pkg/source"
)

var log = logf.Log.WithName("controller_collectdplugin")

// ResourceMap to hold objects to update/reload
type ResourceMap struct {
	configMap       *corev1.ConfigMap
	daemonSet       *extensionsv1beta1.DaemonSet
	collectdPlugins *[]onapv1alpha1.CollectdPlugin
}

/**
* USER ACTION REQUIRED: This is a scaffold file intended for the user to modify with their own Controller
* business logic.  Delete these comments after modifying this file.*
 */

// Add creates a new CollectdPlugin Controller and adds it to the Manager. The Manager will set fields on the Controller
// and Start it when the Manager is Started.
func Add(mgr manager.Manager) error {
	return add(mgr, newReconciler(mgr))
}

// newReconciler returns a new reconcile.Reconciler
func newReconciler(mgr manager.Manager) reconcile.Reconciler {
	return &ReconcileCollectdPlugin{client: mgr.GetClient(), scheme: mgr.GetScheme()}
}

// add adds a new Controller to mgr with r as the reconcile.Reconciler
func add(mgr manager.Manager, r reconcile.Reconciler) error {
	// Create a new controller
	log.V(1).Info("Creating a new controller for CollectdPlugin")
	c, err := controller.New("collectdplugin-controller", mgr, controller.Options{Reconciler: r})
	if err != nil {
		return err
	}

	// Watch for changes to primary resource CollectdPlugin
	log.V(1).Info("Add watcher for primary resource CollectdPlugin")
	err = c.Watch(&source.Kind{Type: &onapv1alpha1.CollectdPlugin{}}, &handler.EnqueueRequestForObject{})
	if err != nil {
		return err
	}

	return nil
}

// blank assignment to verify that ReconcileCollectdPlugin implements reconcile.Reconciler
var _ reconcile.Reconciler = &ReconcileCollectdPlugin{}

// ReconcileCollectdPlugin reconciles a CollectdPlugin object
type ReconcileCollectdPlugin struct {
	// This client, initialized using mgr.Client() above, is a split client
	// that reads objects from the cache and writes to the apiserver
	client client.Client
	scheme *runtime.Scheme
}

// Reconcile reads that state of the cluster for a CollectdPlugin object and makes changes based on the state read
// and what is in the CollectdPlugin.Spec
// TODO(user): Modify this Reconcile function to implement your Controller logic.  This example creates
// a Pod as an example
// Note:
// The Controller will requeue the Request to be processed again if the returned error is non-nil or
// Result.Requeue is true, otherwise upon completion it will remove the work from the queue.
func (r *ReconcileCollectdPlugin) Reconcile(request reconcile.Request) (reconcile.Result, error) {
	reqLogger := log.WithValues("Request.Namespace", request.Namespace, "Request.Name", request.Name)
	reqLogger.Info("Reconciling CollectdPlugin")

	// Fetch the CollectdPlugin instance
	instance := &onapv1alpha1.CollectdPlugin{}
	err := r.client.Get(context.TODO(), request.NamespacedName, instance)
	if err != nil {
		if errors.IsNotFound(err) {
			// Request object not found, could have been deleted after reconcile request.
			// Owned objects are automatically garbage collected. For additional cleanup logic use finalizers.
			// Return and don't requeue
			reqLogger.V(1).Info("CollectdPlugin object Not found")
			return reconcile.Result{}, nil
		}
		// Error reading the object - requeue the request.
		reqLogger.V(1).Info("Error reading the CollectdPlugin object, Requeuing")
		return reconcile.Result{}, err
	}

	rmap, err := findResourceMapForCR(r, instance)
	if err != nil {
		reqLogger.Info("Skip reconcile: ConfigMap not found")
		return reconcile.Result{}, err
	}

	cm := rmap.configMap
	ds := rmap.daemonSet
	collectPlugins := rmap.collectdPlugins
	reqLogger.V(1).Info("Found ResourceMap")
	reqLogger.V(1).Info("ConfigMap.Namespace", cm.Namespace, "ConfigMap.Name", cm.Name)
	reqLogger.V(1).Info("DaemonSet.Namespace", ds.Namespace, "DaemonSet.Name", ds.Name)

	collectdConf, err := rebuildCollectdConf(collectPlugins)

	//Restart Collectd Pods

	ts := time.Now().Format(time.RFC850)
	reqLogger.V(1).Info("Timestamp : ", ts)
	ds.Spec.Template.SetAnnotations(map[string]string{
		"daaas-random": ComputeSHA256([]byte(ts)),
	})
	cm.SetAnnotations(map[string]string{
		"daaas-random": ComputeSHA256([]byte(ts)),
	})

	cm.Data["node-collectd.conf"] = collectdConf

	// Update the ConfigMap with new Spec and reload DaemonSets
	reqLogger.Info("Updating the ConfigMap", "ConfigMap.Namespace", cm.Namespace, "ConfigMap.Name", cm.Name)
	log.Info("ConfigMap Data", "Map: ", cm.Data)
	err = r.client.Update(context.TODO(), cm)
	if err != nil {
		return reconcile.Result{}, err
	}

	err = r.client.Update(context.TODO(), ds)
	if err != nil {
		return reconcile.Result{}, err
	}
	// Reconcile success
	reqLogger.Info("Updated the ConfigMap", "ConfigMap.Namespace", cm.Namespace, "ConfigMap.Name", cm.Name)
	return reconcile.Result{}, nil
}

// ComputeSHA256  returns hash of data as string
func ComputeSHA256(data []byte) string {
	hash := sha256.Sum256(data)
	return fmt.Sprintf("%x", hash)
}

// findConfigMapForCR returns the configMap used by collectd Daemonset
func findResourceMapForCR(r *ReconcileCollectdPlugin, cr *onapv1alpha1.CollectdPlugin) (ResourceMap, error) {
	cmList := &corev1.ConfigMapList{}
	opts := &client.ListOptions{}
	rmap := ResourceMap{}

	// Select ConfigMaps with label app=collectd
	opts.SetLabelSelector("app=collectd")
	opts.InNamespace(cr.Namespace)
	err := r.client.List(context.TODO(), opts, cmList)
	if err != nil {
		return rmap, err
	}

	if cmList.Items == nil || len(cmList.Items) == 0 {
		return rmap, err
	}

	// Select DaemonSets with label app=collectd
	dsList := &extensionsv1beta1.DaemonSetList{}
	err = r.client.List(context.TODO(), opts, dsList)
	if err != nil {
		return rmap, err
	}

	if dsList.Items == nil || len(dsList.Items) == 0 {
		return rmap, err
	}

	// Get all collectd plugins in the current namespace to rebuild conf.
	collectdPlugins := &onapv1alpha1.CollectdPluginList{}
	cpOpts := &client.ListOptions{}
	cpOpts.InNamespace(cr.Namespace)
	err = r.client.List(context.TODO(), cpOpts, collectdPlugins)
	if err != nil {
		return rmap, err
	}

	rmap.configMap = &cmList.Items[0]
	rmap.daemonSet = &dsList.Items[0]
	rmap.collectdPlugins = &collectdPlugins.Items //will be nil if no plugins exist
	return rmap, err
}

// Get all collectd plugins and reconstruct, compute Hash and check for changes
func rebuildCollectdConf(cpList *[]onapv1alpha1.CollectdPlugin) (string, error) {
	var collectdConf string
	if *cpList == nil || len(*cpList) == 0 {
		return "", errors.NewNotFound(corev1.Resource("collectdplugin"), "CollectdPlugin")
	}
	loadPlugin := make(map[string]string)
	for _, cp := range *cpList {
		if cp.Spec.PluginName == "global" {
			collectdConf += cp.Spec.PluginConf + "\n"
		} else {
			loadPlugin[cp.Spec.PluginName] = cp.Spec.PluginConf
		}
	}

	log.V(1).Info("::::::: Plugins Map ::::::: ", "PluginMap ", loadPlugin)

	for cpName, cpConf := range loadPlugin {
		collectdConf += "LoadPlugin" + " " + cpName + "\n"
		collectdConf += cpConf + "\n"
	}

	collectdConf += "\n#Last line (collectd requires ‘\\n’ at the last line)"

	return collectdConf, nil
}
