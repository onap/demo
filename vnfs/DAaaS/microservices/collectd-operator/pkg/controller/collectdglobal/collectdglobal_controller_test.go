/*
Copyright 2019 Intel Corporation.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package collectdglobal

import (
	"context"
	"os"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	onapv1alpha1 "collectd-operator/pkg/apis/onap/v1alpha1"
	utils "collectd-operator/pkg/controller/utils"

	appsv1 "k8s.io/api/apps/v1"
	corev1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/labels"
	"k8s.io/apimachinery/pkg/runtime"
	"k8s.io/apimachinery/pkg/types"
	"k8s.io/apimachinery/pkg/util/intstr"
	"k8s.io/client-go/kubernetes/scheme"
	"sigs.k8s.io/controller-runtime/pkg/client/fake"
	"sigs.k8s.io/controller-runtime/pkg/reconcile"
	logf "sigs.k8s.io/controller-runtime/pkg/runtime/log"
)

const (
	name      = "collectd-operator"
	namespace = "test1"
	gOpts     = "BaseDir     \"/opt/collectd/var/lib/collectd\"\nPIDFile     \"/opt/collectd/var/run/collectd.pid\"\nPluginDir" +
		"\"/opt/collectd/lib/collectd\"\nTypesDB     \"/opt/collectd/share/collectd/types.db\"\n" +
		"#Hostname \"localhost\"\nInterval 1 \nWriteQueueLimitHigh" +
		"10000000\nWriteQueueLimitLow   8000000\nTimeout \"10\"\nReadThreads \"50\"\nWriteThreads" +
		"\"50\"\n\n#Enable plugins:\n\nLoadPlugin cpufreq\nLoadPlugin ipmi\nLoadPlugin" +
		"turbostat\nLoadPlugin irq\nLoadPlugin memcached\nLoadPlugin memory\nLoadPlugin" +
		"processes\nLoadPlugin load\n"
	typesCmName = "types-configmap"
	watchLabels = "app=collectd"
)

var (
	one   = intstr.FromInt(1)
	ls, _ = labels.ConvertSelectorToLabelsMap(watchLabels)
	cm    = &corev1.ConfigMap{
		ObjectMeta: metav1.ObjectMeta{
			Name:      "cp-config-map",
			Namespace: namespace,
			Labels:    ls,
		},
		Data: map[string]string{
			"collectd.conf": "",
		},
	}
	typesCm = &corev1.ConfigMap{
		ObjectMeta: metav1.ObjectMeta{
			Name:      typesCmName,
			Namespace: namespace,
			Labels:    ls,
		},
		Data: map[string]string{
			"types.db": "types.db data",
		},
	}
	ds = &appsv1.DaemonSet{
		ObjectMeta: metav1.ObjectMeta{
			Name:      "cp-collectd",
			Namespace: namespace,
			Labels:    ls,
		},
		Spec: appsv1.DaemonSetSpec{
			Selector: &metav1.LabelSelector{MatchLabels: ls},
			UpdateStrategy: appsv1.DaemonSetUpdateStrategy{
				Type: appsv1.RollingUpdateDaemonSetStrategyType,
				RollingUpdate: &appsv1.RollingUpdateDaemonSet{
					MaxUnavailable: &one,
				},
			},
			Template: corev1.PodTemplateSpec{
				ObjectMeta: metav1.ObjectMeta{
					Labels: ls,
				},
				Spec: corev1.PodSpec{
					Containers: []corev1.Container{{Name: "foo", Image: "bar"}},
				},
			},
		},
	}

	cp = &onapv1alpha1.CollectdPlugin{
		ObjectMeta: metav1.ObjectMeta{
			Name:      "cpu",
			Namespace: namespace,
		},
		Spec: onapv1alpha1.CollectdPluginSpec{
			PluginName: "cpu",
			PluginConf: "Interval 10",
		},
	}

	cpList = &onapv1alpha1.CollectdPluginList{
		Items: []onapv1alpha1.CollectdPlugin{},
	}
)

// TestCollectdGlobalController runs ReconcileCollectdGlobal.Reconcile() against a
// fake client that tracks a CollectdGlobal object.

// Reconcile No CR exist.
func TestCollectdGlobalNoCR(t *testing.T) {
	// Set the logger to development mode for verbose logs.
	logf.SetLogger(logf.ZapLogger(true))

	os.Setenv("WATCH_LABELS", watchLabels)

	cg := &onapv1alpha1.CollectdGlobal{}

	cgList := &onapv1alpha1.CollectdGlobalList{
		Items: []onapv1alpha1.CollectdGlobal{},
	}

	_ = cp

	// Objects to track in the fake client.
	objs := []runtime.Object{
		cm,
		ds,
		cgList,
	}
	// Register operator types with the runtime scheme.
	s := scheme.Scheme
	s.AddKnownTypes(onapv1alpha1.SchemeGroupVersion, cg, cgList, cpList)

	// Create a fake client to mock API calls
	fc1 := fake.NewFakeClient(objs...)
	rcg := &ReconcileCollectdGlobal{client: fc1, scheme: s}

	req := reconcile.Request{
		NamespacedName: types.NamespacedName{
			Namespace: namespace,
			Name:      name,
		},
	}
	res, err := rcg.Reconcile(req)
	if err != nil {
		t.Fatalf("reconcile: (%v)", err)
	}
	// Check the result of reconciliation to make sure it has the desired state.
	if res.Requeue {
		t.Error("Unexpected Reconcile requeue request")
	}
}

// Test CollectdGlobalController - Add CR with non existent ConfigMap.
func TestCollectdGlobalNoCM(t *testing.T) {
	// Set the logger to development mode for verbose logs.
	logf.SetLogger(logf.ZapLogger(true))

	os.Setenv("WATCH_LABELS", watchLabels)

	cg := &onapv1alpha1.CollectdGlobal{
		ObjectMeta: metav1.ObjectMeta{
			Name:      name,
			Namespace: namespace,
		},
		Spec: onapv1alpha1.CollectdGlobalSpec{
			GlobalOptions: gOpts,
		},
	}

	cgList := &onapv1alpha1.CollectdGlobalList{
		Items: []onapv1alpha1.CollectdGlobal{*cg},
	}

	_ = cp

	// Objects to track in the fake client.
	objs := []runtime.Object{
		ds,
		cgList,
	}
	// Register operator types with the runtime scheme.
	s := scheme.Scheme
	s.AddKnownTypes(onapv1alpha1.SchemeGroupVersion, cg, cgList, cpList)

	// Create a fake client to mock API calls
	fc1 := fake.NewFakeClient(objs...)
	rcg := &ReconcileCollectdGlobal{client: fc1, scheme: s}

	req := reconcile.Request{
		NamespacedName: types.NamespacedName{
			Namespace: namespace,
			Name:      name,
		},
	}
	defer func() {
		if r := recover(); r == nil {
			t.Errorf("The code did not panic")
		} else {
			t.Log("Successful Panic")
		}
	}()
	res, err := rcg.Reconcile(req)
	if err != nil {
		t.Fatalf("reconcile: (%v)", err)
	}
	// Check the result of reconciliation to make sure it has the desired state.
	if !res.Requeue {
		t.Error("Reconcile did not requeue request as expected")
	}
}

// Test CollectdGlobalController - HandleDelete
func TestCollectdGlobalHandleDelete(t *testing.T) {
	// Set the logger to development mode for verbose logs.
	logf.SetLogger(logf.ZapLogger(true))

	os.Setenv("WATCH_LABELS", watchLabels)

	cg := &onapv1alpha1.CollectdGlobal{
		ObjectMeta: metav1.ObjectMeta{
			Name:      name,
			Namespace: namespace,
			DeletionTimestamp: &metav1.Time{
				Time: time.Now(),
			},
			Finalizers: []string{
				utils.CollectdFinalizer,
			},
		},
		Spec: onapv1alpha1.CollectdGlobalSpec{
			GlobalOptions: gOpts,
		},
	}

	cgList := &onapv1alpha1.CollectdGlobalList{
		Items: []onapv1alpha1.CollectdGlobal{*cg},
	}

	_ = cp

	// Objects to track in the fake client.
	objs := []runtime.Object{
		cm,
		ds,
		cgList,
	}
	// Register operator types with the runtime scheme.
	s := scheme.Scheme
	s.AddKnownTypes(onapv1alpha1.SchemeGroupVersion, cg, cgList, cpList)

	// Create a fake client to mock API calls
	fc1 := fake.NewFakeClient(objs...)
	rcg := &ReconcileCollectdGlobal{client: fc1, scheme: s}

	req := reconcile.Request{
		NamespacedName: types.NamespacedName{
			Namespace: namespace,
			Name:      name,
		},
	}
	res, err := rcg.Reconcile(req)
	if err != nil {
		t.Fatalf("reconcile: (%v)", err)
	}
	// Check the result of reconciliation to make sure it has the desired state.
	if res.Requeue {
		t.Error("Unexpected Reconcile requeue request")
	}
}

// Test CollectdGlobalController
func TestCollectdGlobalController(t *testing.T) {
	// Set the logger to development mode for verbose logs.
	logf.SetLogger(logf.ZapLogger(true))

	os.Setenv("WATCH_LABELS", watchLabels)

	cg := &onapv1alpha1.CollectdGlobal{
		ObjectMeta: metav1.ObjectMeta{
			Name:      name,
			Namespace: namespace,
		},
		Spec: onapv1alpha1.CollectdGlobalSpec{
			GlobalOptions: gOpts,
		},
	}

	cgList := &onapv1alpha1.CollectdGlobalList{
		Items: []onapv1alpha1.CollectdGlobal{*cg},
	}

	_ = cp

	// Objects to track in the fake client.
	objs := []runtime.Object{
		cm,
		ds,
		cgList,
	}
	// Register operator types with the runtime scheme.
	s := scheme.Scheme
	s.AddKnownTypes(onapv1alpha1.SchemeGroupVersion, cg, cgList, cpList)

	// Create a fake client to mock API calls
	fc1 := fake.NewFakeClient(objs...)
	rcg := &ReconcileCollectdGlobal{client: fc1, scheme: s}

	req := reconcile.Request{
		NamespacedName: types.NamespacedName{
			Namespace: namespace,
			Name:      name,
		},
	}
	res, err := rcg.Reconcile(req)
	if err != nil {
		t.Fatalf("reconcile: (%v)", err)
	}
	// Check the result of reconciliation to make sure it has the desired state.
	if res.Requeue {
		t.Error("Unexpected Reconcile requeue request")
	}
}

// Test HandleTypesDB
func TestHandleTypesDB(t *testing.T) {
	// Set the logger to development mode for verbose logs.
	logf.SetLogger(logf.ZapLogger(true))

	os.Setenv("WATCH_LABELS", watchLabels)

	cg := &onapv1alpha1.CollectdGlobal{
		ObjectMeta: metav1.ObjectMeta{
			Name:      name,
			Namespace: namespace,
		},
		Spec: onapv1alpha1.CollectdGlobalSpec{
			GlobalOptions: gOpts,
			ConfigMap:     typesCmName,
		},
	}

	cgList := &onapv1alpha1.CollectdGlobalList{
		Items: []onapv1alpha1.CollectdGlobal{*cg},
	}

	_ = cp

	testCases := []struct {
		name          string
		createTypesCm bool
	}{
		{name: "Handle TypesDB missing TypesDB ConfigMap", createTypesCm: false},
		{name: "Handle TypesDB", createTypesCm: true},
	}
	for _, testCase := range testCases {
		t.Run(testCase.name, func(t *testing.T) {
			// Objects to track in the fake client.
			objs := []runtime.Object{
				cm,
				ds,
				cgList,
			}
			if testCase.createTypesCm {
				objs = append(objs, typesCm)
			}
			// Register operator types with the runtime scheme.
			s := scheme.Scheme
			s.AddKnownTypes(onapv1alpha1.SchemeGroupVersion, cg, cgList, cpList)

			// Create a fake client to mock API calls
			fc1 := fake.NewFakeClient(objs...)
			rcg := &ReconcileCollectdGlobal{client: fc1, scheme: s}

			req := reconcile.Request{
				NamespacedName: types.NamespacedName{
					Namespace: namespace,
					Name:      name,
				},
			}
			res, err := rcg.Reconcile(req)
			if err != nil {
				t.Fatalf("reconcile: (%v)", err)
			}
			// Check the result of reconciliation to make sure it has the desired state.
			if res.Requeue {
				t.Error("Unexpected Reconcile requeue request")
			}
		})
	}
}

// Test UpdateStatus
func TestUpdateStatus(t *testing.T) {
	// Set the logger to development mode for verbose logs.
	logf.SetLogger(logf.ZapLogger(true))

	os.Setenv("WATCH_LABELS", watchLabels)

	testCases := []struct {
		name           string
		cg             *onapv1alpha1.CollectdGlobal
		createPods     bool
		expectedStatus string
		expectedError  error
	}{
		{
			name: "Update Status of New CR",
			cg: &onapv1alpha1.CollectdGlobal{
				ObjectMeta: metav1.ObjectMeta{
					Name:      name,
					Namespace: namespace,
				},
				Spec: onapv1alpha1.CollectdGlobalSpec{
					GlobalOptions: gOpts,
				},
			},
			expectedStatus: onapv1alpha1.Created,
			expectedError:  nil,
		},
		{
			name: "Update Initial state to Created",
			cg: &onapv1alpha1.CollectdGlobal{
				ObjectMeta: metav1.ObjectMeta{
					Name:      name,
					Namespace: namespace,
				},
				Spec: onapv1alpha1.CollectdGlobalSpec{
					GlobalOptions: gOpts,
				},
				Status: onapv1alpha1.CollectdGlobalStatus{
					Status: onapv1alpha1.Initial,
				},
			},
			expectedStatus: onapv1alpha1.Created,
			expectedError:  nil,
		},
		{
			name: "Update Created state - No Pods",
			cg: &onapv1alpha1.CollectdGlobal{
				ObjectMeta: metav1.ObjectMeta{
					Name:      name,
					Namespace: namespace,
				},
				Spec: onapv1alpha1.CollectdGlobalSpec{
					GlobalOptions: gOpts,
				},
				Status: onapv1alpha1.CollectdGlobalStatus{
					Status: onapv1alpha1.Created,
				},
			},
			expectedStatus: onapv1alpha1.Created,
			expectedError:  nil,
		},
		{
			name: "Update Created state to Enabled",
			cg: &onapv1alpha1.CollectdGlobal{
				ObjectMeta: metav1.ObjectMeta{
					Name:      name,
					Namespace: namespace,
				},
				Spec: onapv1alpha1.CollectdGlobalSpec{
					GlobalOptions: gOpts,
				},
				Status: onapv1alpha1.CollectdGlobalStatus{
					Status: onapv1alpha1.Created,
				},
			},
			createPods:     true,
			expectedStatus: onapv1alpha1.Enabled,
			expectedError:  nil,
		},
	}
	req := reconcile.Request{
		NamespacedName: types.NamespacedName{
			Namespace: namespace,
			Name:      name,
		},
	}

	for _, testCase := range testCases {
		t.Run(testCase.name, func(t *testing.T) {
			// Objects to track in the fake client.
			objs := []runtime.Object{
				testCase.cg,
			}
			if testCase.createPods {
				pods := &corev1.Pod{
					ObjectMeta: metav1.ObjectMeta{
						Namespace: testCase.cg.Namespace,
						Name:      "cp-collectd-abcde",
						Labels:    ls,
					},
					Spec: corev1.PodSpec{
						Containers: []corev1.Container{
							corev1.Container{
								Image: "collectd",
								Name:  "collectd",
							},
						},
					},
				}
				objs = append(objs, runtime.Object(pods))
			}
			// Register operator types with the runtime scheme.
			s := scheme.Scheme
			s.AddKnownTypes(onapv1alpha1.SchemeGroupVersion, testCase.cg)

			// Create a fake client to mock API calls
			fc1 := fake.NewFakeClient(objs...)
			rcg := &ReconcileCollectdGlobal{client: fc1, scheme: s}

			err := rcg.updateStatus(testCase.cg)
			assert.Equal(t, testCase.expectedError, err)
			// Fetch the CollectdGlobal instance
			actual := &onapv1alpha1.CollectdGlobal{}
			err = fc1.Get(context.TODO(), req.NamespacedName, actual)
			assert.Equal(t, testCase.expectedStatus, actual.Status.Status)
			if testCase.createPods {
				assert.Equal(t, []string{"cp-collectd-abcde"}, actual.Status.CollectdAgents)
			}
		})
	}
}
