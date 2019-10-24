package v1alpha1

import (
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

// PrometheusRemoteEndpointSpec defines the desired state of PrometheusRemoteEndpoint
// +k8s:openapi-gen=true
type PrometheusRemoteEndpointSpec struct {
	AdapterUrl     string                `json:"adapterUrl"`
	FilterSelector *metav1.LabelSelector `json:"filterSelector,omitEmpty"`
	Type           string                `json:"type"`
	KafkaConfig    string                `json:"kafkaConfig,omitEmpty"`
	QueueConfig    QueueConfig           `json:"queueConfig,omitEmpty"`
	RemoteTimeout  string                `json:"remoteTimeout,omitEmpty"`
}

type KafkaConfig struct {
	BrokerUrl string `json:"brokerUrl"`
	Group     string `json:"group,omitEmpty"`
	Topic     string `json:"topic"`
}

type QueueConfig struct {
	BatchSendDeadline string `json:"batchSendDeadline,omitEmpty"`
	Capacity          string `json:"capacity,omitEmpty"`
	MaxBackoff        string `json:"maxBackoff,omitEmpty"`
	MaxRetries        string `json:"maxRetries,omitEmpty"`
	MaxSamplesPerSend string `json:"maxSamplesPerSend,omitEmpty"`
	MaxShards         string `json:"maxShards,omitEmpty"`
	MinBackoff        string `json:"minBackoff,omitEmpty"`
	MinShards         string `json:"minShards,omitEmpty"`
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
