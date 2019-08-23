package v1alpha1

import (
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

// EDIT THIS FILE!  THIS IS SCAFFOLDING FOR YOU TO OWN!
// NOTE: json tags are required.  Any new fields you add must have json tags for the fields to be serialized.

// GrafanaDataSourceSpec defines the desired state of GrafanaDataSource
// +k8s:openapi-gen=true
type GrafanaDataSourceSpec struct {
	// INSERT ADDITIONAL SPEC FIELDS - desired state of cluster
	// Important: Run "operator-sdk generate k8s" to regenerate code after modifying this file
	// Add custom validation using kubebuilder tags: https://book-v1.book.kubebuilder.io/beyond_basics/generating_crd.html
	Datasources []Datasource      `json:"datasources"`
	Grafana     map[string]string `json:"grafana"`
}

// GrafanaDataSourceStatus defines the observed state of GrafanaDataSource
// +k8s:openapi-gen=true
type GrafanaDataSourceStatus struct {
}

// Datasource defines the fields in a GrafanaDataSource
// +k8s:openapi-gen=true
type Datasource struct {
	Name      string `json:"name"`
	Type      string `json:"type"`
	URL       string `json:"url"`
	IsDefault bool   `json:"isDefault,omitempty"`
	Access    string `json:"access,omitempty"`
}

// +k8s:deepcopy-gen:interfaces=k8s.io/apimachinery/pkg/runtime.Object

// GrafanaDataSource is the Schema for the grafanadatasources API
// +k8s:openapi-gen=true
// +kubebuilder:subresource:status
type GrafanaDataSource struct {
	metav1.TypeMeta   `json:",inline"`
	metav1.ObjectMeta `json:"metadata,omitempty"`

	Spec   GrafanaDataSourceSpec   `json:"spec,omitempty"`
	Status GrafanaDataSourceStatus `json:"status,omitempty"`
}

// +k8s:deepcopy-gen:interfaces=k8s.io/apimachinery/pkg/runtime.Object

// GrafanaDataSourceList contains a list of GrafanaDataSource
type GrafanaDataSourceList struct {
	metav1.TypeMeta `json:",inline"`
	metav1.ListMeta `json:"metadata,omitempty"`
	Items           []GrafanaDataSource `json:"items"`
}

func init() {
	SchemeBuilder.Register(&GrafanaDataSource{}, &GrafanaDataSourceList{})
}
