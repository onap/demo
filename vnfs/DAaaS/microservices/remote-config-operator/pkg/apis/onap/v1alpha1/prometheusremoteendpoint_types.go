package v1alpha1

import (
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

// PrometheusRemoteEndpointSpec defines the desired state of PrometheusRemoteEndpoint
// +k8s:openapi-gen=true
type PrometheusRemoteEndpointSpec struct {
	AdapterURL     string                `json:"adapterURL"`
	FilterSelector *metav1.LabelSelector `json:"filterSelector,omitempty"`
	Type           string                `json:"type"`
	KafkaConfig    string                `json:"kafkaConfig,omitempty"`
	QueueConfig    QueueConfig           `json:"queueConfig,omitempty"`
	RemoteTimeout  string                `json:"remoteTimeout,omitempty"`
}

// KafkaConfig - defines the desired remote kafka writer configurations
type KafkaConfig struct {
	BrokerURL    string `json:"bootstrap.servers"`
	Topic        string `json:"topic"`
	UsePartition bool   `json:"usePartition"`
	BatchMsgNum  int    `json:"batch.num.messages,omitempty`
	Compression  string `json:"compression.codec,omitempty`
}

// QueueConfig - defines the prometheus remote write queue configurations
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
// +kubebuilder:subresource:status
type PrometheusRemoteEndpointStatus struct {
	// INSERT ADDITIONAL STATUS FIELD - define observed state of cluster
	// Important: Run "operator-sdk generate k8s" to regenerate code after modifying this file
	// Add custom validation using kubebuilder tags: https://book-v1.book.kubebuilder.io/beyond_basics/generating_crd.html
	// Status can be Error, Enabled
	PrometheusInstance string `json:"prometheusInstance,omitempty"`
	Status             string `json:"status"`
	KafkaWriterID      string `json:"kafkaWriterID,omitempty"`
	RemoteURL          string `json:"remoteURL,omitempty"`
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
