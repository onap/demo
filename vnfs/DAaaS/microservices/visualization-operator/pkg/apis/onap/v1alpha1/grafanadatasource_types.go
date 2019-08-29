package v1alpha1

import (
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

// GrafanaDataSourceSpec defines the desired state of GrafanaDataSource
// +k8s:openapi-gen=true
type GrafanaDataSourceSpec struct {
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
	Name              string            `json:"name"`
	Type              string            `json:"type"`
	URL               string            `json:"url"`
	IsDefault         bool              `json:"isDefault,omitempty"`
	Access            string            `json:"access"`
	WithCredentials   bool              `json:"withCredentials,omitempty"`
	BasicAuth         bool              `json:"basicAuth,omitempty"`
	BasicAuthUser     string            `json:"basicAuthUser,omitempty"`
	BasicAuthPassword string            `json:"basicAuthPassword,omitempty"`
	JsonData          map[string]string `json:"jsonData,omitempty"`
	Database          string            `json:"database,omitempty"`
	User              string            `json:"user,omitempty"`
	Password          string            `json:"password,omitempty"`
	ReadOnly          bool              `json:"readOnly,omitempty"`
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
