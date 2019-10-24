package v1alpha1

import (
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

type Metadata struct {
	Name   string   `json:"Name"`
	Labels []string `json:"Labels"`
}

// PrometheusRemoteEndpointSpec defines the desired state of PrometheusRemoteEndpoint
// +k8s:openapi-gen=true
type PrometheusRemoteEndpointSpec struct {
	AdapterUrl     string                `json:"adapterUrl"`
	FilterSelector *metav1.LabelSelector `json:"filterSelector,omitempty"`
	Type           string                `json:"type"`
	KafkaConfig    string                `json:"kafkaConfig,omitempty"`
	QueueConfig    QueueConfig           `json:"queueConfig,omitempty"`
	RemoteTimeout  string                `json:"remoteTimeout,omitempty"`
}

type KafkaConfig struct {
	BrokerUrl string `json:"brokerUrl"`
	Group     string `json:"group,omitempty"`
	Topic     string `json:"topic"`
}

type QueueConfig struct {
	BatchSendDeadline string `json:"batchSendDeadline,omitempty"`
	Capacity          string `json:"capacity,omitempty"`
	MaxBackoff        string `json:"maxBackoff,omitempty"`
	MaxRetries        string `json:"maxRetries,omitempty"`
	MaxSamplesPerSend string `json:"maxSamplesPerSend,omitempty"`
	MaxShards         string `json:"maxShards,omitempty"`
	MinBackoff        string `json:"minBackoff,omitempty"`
	MinShards         string `json:"minShards,omitempty"`
}

// PrometheusRemoteEndpointStatus defines the observed state of PrometheusRemoteEndpoint
// +k8s:openapi-gen=true
type PrometheusRemoteEndpointStatus struct {
	// INSERT ADDITIONAL STATUS FIELD - define observed state of cluster
	// Important: Run "operator-sdk generate k8s" to regenerate code after modifying this file
	// Add custom validation using kubebuilder tags: https://book-v1.book.kubebuilder.io/beyond_basics/generating_crd.html
}

// +k8s:deepcopy-gen:interfaces=k8s.io/apimachinery/pkg/runtime.Object

// PrometheusRemoteEndpoint is the Schema for the prometheusremoteendpoints API
// +k8s:openapi-gen=true
// +kubebuilder:subresource:status
// +kubebuilder:resource:path=prometheusremoteendpoints,scope=Namespaced
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
