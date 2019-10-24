package v1alpha1

import (
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

// PrometheusRemoteEndpointSpec defines the desired state of PrometheusRemoteEndpoint
// +k8s:openapi-gen=true
type PrometheusRemoteEndpointSpec struct {
	RemoteWrite []RemoteWriteEndpoint `json:"remoteWrite"`
}

// PrometheusRemoteEndpointStatus defines the observed state of PrometheusRemoteEndpoint
// +k8s:openapi-gen=true
type PrometheusRemoteEndpointStatus struct {
}

// RemoteWriteEndpoint defines the endpoints and configs of the remote systems
// +k8s:openapi-gen=true
type RemoteWriteEndpoint struct {
	URL                 string               `json:"url"`
	Type                string               `json:"type"`
	WriteRelabelConfigs []RemoteFilterAction `json:"writeRelabelConfigs"`
	FilterSelector      Selector             `json:"filterSelector,omitEmpty"`
	KafkaConfig         KafkaConfig          `json:"kafkaConfig,omitEmpty"`
}

// RemoteFilterAction defines the actions to be taken before writing to the remote systems
// +k8s:openapi-gen=true
type RemoteFilterAction struct {
	Action      string   `json:"action,omitEmpty"`
	Regex       string   `json:"regex,omitEmpty"`
	SourceLabels []string `json:"sourceLabels,omitEmpty"`
	TargetLabel string   `json:"targetLabel,omitEmpty"`
	Replacement string   `json:"replacement,omitEmpty"`
}

// Selector defines selectors for selecting appropriate systems
// +k8s:openapi-gen=true
type Selector struct {
	Remote string `json:"remote,omitEmpty"`
}

// KafkaConfig defines config for kafka broker
// +k8s:openapi-gen=true
type KafkaConfig struct {
	BrokerURL string `json:"brokerURL"`
	Group     string `json:"group"`
	Topic     string `json:"topic"`
}

// +k8s:deepcopy-gen:interfaces=k8s.io/apimachinery/pkg/runtime.Object

// PrometheusRemoteEndpoint is the Schema for the prometheusremoteendpoints API
// +k8s:openapi-gen=true
// +kubebuilder:subresource:status
type PrometheusRemoteEndpoint struct {
	metav1.TypeMeta   `json:",inline"`
	metav1.ObjectMeta `json:"metadata,omitempty"`

	Spec   PrometheusRemoteEndpointSpec   `json:"spec,omitempty"`
	Status PrometheusRemoteEndpointStatus `json:"status,omitempty"`
}

// +k8s:deepcopy-gen:interfaces=k8s.io/apimachinery/pkg/runtime.Object

// PrometheusRemoteEndpointList contains a list of PrometheusRemoteEndpoint
type PrometheusRemoteEndpointList struct {
	metav1.TypeMeta `json:",inline"`
	metav1.ListMeta `json:"metadata,omitempty"`
	Items           []PrometheusRemoteEndpoint `json:"items"`
}

func init() {
	SchemeBuilder.Register(&PrometheusRemoteEndpoint{}, &PrometheusRemoteEndpointList{})
}
