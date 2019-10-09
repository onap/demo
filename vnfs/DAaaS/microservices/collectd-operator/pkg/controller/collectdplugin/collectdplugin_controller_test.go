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

package collectdplugin

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
	name      = "cpu"
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
	one    = intstr.FromInt(1)
	ls, _  = labels.ConvertSelectorToLabelsMap(watchLabels)
	cgList = &onapv1alpha1.CollectdGlobalList{
		Items: []onapv1alpha1.CollectdGlobal{},
	}
	cm = &corev1.ConfigMap{
		ObjectMeta: metav1.ObjectMeta{
			Name:      "cp-config-map",
			Namespace: namespace,
			Labels:    ls,
		},
		Data: map[string]string{
			"collectd.conf": "",
		},
	}
	readonlyCM = &corev1.ConfigMap{
		ObjectMeta: metav1.ObjectMeta{
			Name:      "cp-config-map-read-only",
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
					Containers: []corev1.Container{{Name: "cp-collectd-1", Image: "collectd"}},
				},
			},
		},
	}

	cp1 = &onapv1alpha1.CollectdPlugin{
		TypeMeta: metav1.TypeMeta{
			Kind:       "CollectdPlugin",
			APIVersion: "onap.org/v1alpha1",
		},
		ObjectMeta: metav1.ObjectMeta{
			Name:      "cpu",
			Namespace: namespace,
		},
		Spec: onapv1alpha1.CollectdPluginSpec{
			PluginName: "cpu",
			PluginConf: "<Plugin cpu>\n" +
				"ReportByCpu true\n" +
				"ReportByState true\n" +
				"ValuesPercentage true\n" +
				"</Plugin>\n",
		},
	}

	delCp1 = &onapv1alpha1.CollectdPlugin{
		TypeMeta: metav1.TypeMeta{
			Kind:       "CollectdPlugin",
			APIVersion: "onap.org/v1alpha1",
		},
		ObjectMeta: metav1.ObjectMeta{
			Name:      "cpu",
			Namespace: namespace,
			DeletionTimestamp: &metav1.Time{
				Time: time.Now(),
			},
			Finalizers: []string{
				utils.CollectdFinalizer,
			},
		},
		Spec: onapv1alpha1.CollectdPluginSpec{
			PluginName: "cpu",
			PluginConf: "<Plugin cpu>\n" +
				"ReportByCpu true\n" +
				"ReportByState true\n" +
				"ValuesPercentage true\n" +
				"</Plugin>\n",
		},
	}

	cp2 = &onapv1alpha1.CollectdPlugin{
		ObjectMeta: metav1.ObjectMeta{
			Name:      "write_prometheus",
			Namespace: namespace,
		},
		Spec: onapv1alpha1.CollectdPluginSpec{
			PluginName: "cpu",
			PluginConf: "<Plugin write_prometheus>\n" +
				"Port 9103\n" +
				"</Plugin>\n",
		},
	}

	cpList = &onapv1alpha1.CollectdPluginList{
		Items: []onapv1alpha1.CollectdPlugin{},
	}
)

func TestReconcileCollectdPlugin_Reconcile(t *testing.T) {
	logf.SetLogger(logf.ZapLogger(true))

	os.Setenv("WATCH_LABELS", watchLabels)

	tests := []struct {
		name      string
		objs      []runtime.Object
		want      reconcile.Result
		wantErr   bool
		wantPanic bool
	}{
		{
			name:    "CollectdPlugin Reconcile No CR",
			objs:    getObjs(nil, []runtime.Object{cm, ds}),
			want:    reconcile.Result{Requeue: false},
			wantErr: false,
		},
		{
			name:    "CollectdPlugin Add new CR cp1",
			objs:    getObjs([]onapv1alpha1.CollectdPlugin{*cp1}, []runtime.Object{cm, ds}),
			want:    reconcile.Result{Requeue: false},
			wantErr: false,
		},
		{
			name:    "CollectdPlugin Delete CR cp1",
			objs:    getObjs([]onapv1alpha1.CollectdPlugin{*delCp1}, []runtime.Object{cm, ds}),
			want:    reconcile.Result{Requeue: false},
			wantErr: false,
		},
		{
			name:      "CollectdPlugin Add new CR No configMap",
			objs:      getObjs([]onapv1alpha1.CollectdPlugin{*cp1}, []runtime.Object{ds}),
			want:      reconcile.Result{Requeue: false},
			wantErr:   false,
			wantPanic: true,
		},
		{
			name:      "CollectdPlugin Add new CR no DaemonSet",
			objs:      getObjs([]onapv1alpha1.CollectdPlugin{*cp1}, []runtime.Object{cm}),
			want:      reconcile.Result{Requeue: false},
			wantErr:   false,
			wantPanic: true,
		},
		{
			name:    "CollectdPlugin Add new CR cp2",
			objs:    getObjs([]onapv1alpha1.CollectdPlugin{*cp2}, []runtime.Object{cm, ds}),
			want:    reconcile.Result{Requeue: false},
			wantErr: false,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {

			if tt.wantPanic {
				defer func() {
					if r := recover(); r == nil {
						t.Errorf("The code did not panic")
					} else {
						t.Log("Successful Panic")
					}
				}()
			}
			// Objects to track in the fake client.
			objs := tt.objs

			// Register operator types with the runtime scheme.
			s := scheme.Scheme
			s.AddKnownTypes(onapv1alpha1.SchemeGroupVersion, cp1, cpList, cgList)

			// Create a fake client to mock API calls
			fc1 := fake.NewFakeClient(objs...)
			rcp := &ReconcileCollectdPlugin{client: fc1, scheme: s}
			req := reconcile.Request{
				NamespacedName: types.NamespacedName{
					Namespace: namespace,
					Name:      name,
				},
			}
			got, err := rcp.Reconcile(req)
			if err != nil {
				t.Fatalf("reconcile: (%v)", err)
			}
			assert.Equal(t, tt.want, got)
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
		cp             *onapv1alpha1.CollectdPlugin
		createPods     bool
		expectedStatus string
		expectedError  error
	}{
		{
			name:           "Update Status of New CR",
			cp:             getNewCPWithStatus(cp1, onapv1alpha1.Initial),
			expectedStatus: onapv1alpha1.Created,
			expectedError:  nil,
		},
		{
			name:           "Update Initial state to Created",
			cp:             getNewCPWithStatus(cp1, onapv1alpha1.Created),
			expectedStatus: onapv1alpha1.Created,
			expectedError:  nil,
		},
		{
			name:           "Update Created state - No Pods",
			cp:             getNewCPWithStatus(cp1, onapv1alpha1.Created),
			expectedStatus: onapv1alpha1.Created,
			expectedError:  nil,
		},
		{
			name:           "Update Created state to Enabled",
			cp:             getNewCPWithStatus(cp1, onapv1alpha1.Created),
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
				testCase.cp,
			}
			if testCase.createPods {
				pods := &corev1.Pod{
					ObjectMeta: metav1.ObjectMeta{
						Namespace: testCase.cp.Namespace,
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
			s.AddKnownTypes(onapv1alpha1.SchemeGroupVersion, testCase.cp)

			// Create a fake client to mock API calls
			fc1 := fake.NewFakeClient(objs...)
			rcp := &ReconcileCollectdPlugin{client: fc1, scheme: s}

			err := rcp.updateStatus(testCase.cp)
			assert.Equal(t, testCase.expectedError, err)
			// Fetch the CollectdGlobal instance
			actual := &onapv1alpha1.CollectdPlugin{}
			err = fc1.Get(context.TODO(), req.NamespacedName, actual)
			assert.Equal(t, testCase.expectedStatus, actual.Status.Status)
			if testCase.createPods {
				assert.Equal(t, []string{"cp-collectd-abcde"}, actual.Status.CollectdAgents)
			}
		})
	}
}

func getNewCPWithStatus(cp *onapv1alpha1.CollectdPlugin, status string) *onapv1alpha1.CollectdPlugin {
	cpTemp := cp.DeepCopy()
	cpTemp.Status.Status = status
	return cpTemp
}

func getObjs(cp []onapv1alpha1.CollectdPlugin, objs []runtime.Object) []runtime.Object {
	cpL := cpList.DeepCopy()
	cpL.Items = nil
	items := append(cpL.Items, cp...)
	cpL.Items = items
	objs = append(objs, cpL)
	return objs
}
