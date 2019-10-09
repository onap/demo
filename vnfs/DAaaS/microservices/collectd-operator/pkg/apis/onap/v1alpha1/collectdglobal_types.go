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

package v1alpha1

import (
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

// NOTE: json tags are required.  Any new fields you add must have json tags for the fields to be serialized.

// CollectdGlobalSpec defines the desired state of CollectdGlobal
// +k8s:openapi-gen=true
type CollectdGlobalSpec struct {
	// INSERT ADDITIONAL SPEC FIELDS - desired state of cluster
	// Important: Run "operator-sdk generate k8s" to regenerate code after modifying this file
	// Add custom validation using kubebuilder tags: https://book.kubebuilder.io/beyond_basics/generating_crd.html
	GlobalOptions string `json:"globalOptions"`
	ConfigMap     string `json:"configMap,omitempty"`
}

// CollectdGlobalStatus defines the observed state of CollectdGlobal
// +k8s:openapi-gen=true
type CollectdGlobalStatus struct {
	// INSERT ADDITIONAL STATUS FIELD - define observed state of cluster
	// Important: Run "operator-sdk generate k8s" to regenerate code after modifying this file
	// Add custom validation using kubebuilder tags: https://book.kubebuilder.io/beyond_basics/generating_crd.html
	// CollectdAgents are the collectd pods in the Daemonset
	// Status can be one of "", Created, Deleting, Applied, Deprecated
	CollectdAgents []string `json:"collectdAgents,omitempty"`
	Status         string   `json:"status"`
}

// +k8s:deepcopy-gen:interfaces=k8s.io/apimachinery/pkg/runtime.Object

// CollectdGlobal is the Schema for the collectdglobals API
// +k8s:openapi-gen=true
// +kubebuilder:subresource:status
type CollectdGlobal struct {
	metav1.TypeMeta   `json:",inline"`
	metav1.ObjectMeta `json:"metadata,omitempty"`

	Spec   CollectdGlobalSpec   `json:"spec,omitempty"`
	Status CollectdGlobalStatus `json:"status,omitempty"`
}

// +k8s:deepcopy-gen:interfaces=k8s.io/apimachinery/pkg/runtime.Object

// CollectdGlobalList contains a list of CollectdGlobal
type CollectdGlobalList struct {
	metav1.TypeMeta `json:",inline"`
	metav1.ListMeta `json:"metadata,omitempty"`
	Items           []CollectdGlobal `json:"items"`
}

func init() {
	SchemeBuilder.Register(&CollectdGlobal{}, &CollectdGlobalList{})
}
