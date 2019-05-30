package v1alpha1

import (
	corev1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

// EDIT THIS FILE!  THIS IS SCAFFOLDING FOR YOU TO OWN!
// NOTE: json tags are required.  Any new fields you add must have json tags for the fields to be serialized.

// CollectdPluginSpec defines the desired state of CollectdPlugin
// +k8s:openapi-gen=true
type CollectdPluginSpec struct {
	// INSERT ADDITIONAL SPEC FIELDS - desired state of cluster
	// Important: Run "operator-sdk generate k8s" to regenerate code after modifying this file
	// Add custom validation using kubebuilder tags: https://book.kubebuilder.io/beyond_basics/generating_crd.html
	PluginName string `json:"pluginName"`
	PluginConf string `json:"pluginConf"`
}

// CollectdPluginStatus defines the observed state of CollectdPlugin
// +k8s:openapi-gen=true
type CollectdPluginStatus struct {
	// INSERT ADDITIONAL STATUS FIELD - define observed state of cluster
	// Important: Run "operator-sdk generate k8s" to regenerate code after modifying this file
	// Add custom validation using kubebuilder tags: https://book.kubebuilder.io/beyond_basics/generating_crd.html
	// CollectdAgents are the collectd pods in the Daemonset
	CollectdAgents []string         `json:"collectdAgents"`
	CollectdConf   corev1.ConfigMap `json:"collectdConf"`
}

// +k8s:deepcopy-gen:interfaces=k8s.io/apimachinery/pkg/runtime.Object

// CollectdPlugin is the Schema for the collectdplugins API
// +k8s:openapi-gen=true
// +kubebuilder:subresource:status
type CollectdPlugin struct {
	metav1.TypeMeta   `json:",inline"`
	metav1.ObjectMeta `json:"metadata,omitempty"`

	Spec   CollectdPluginSpec   `json:"spec,omitempty"`
	Status CollectdPluginStatus `json:"status,omitempty"`
}

// +k8s:deepcopy-gen:interfaces=k8s.io/apimachinery/pkg/runtime.Object

// CollectdPluginList contains a list of CollectdPlugin
type CollectdPluginList struct {
	metav1.TypeMeta `json:",inline"`
	metav1.ListMeta `json:"metadata,omitempty"`
	Items           []CollectdPlugin `json:"items"`
}

func init() {
	SchemeBuilder.Register(&CollectdPlugin{}, &CollectdPluginList{})
}
