package v1alpha1

import (
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

// RemoteFilterActionSpec defines the desired state of RemoteFilterAction
// +k8s:openapi-gen=true
type RemoteFilterActionSpec struct {
	Action       string   `json:"action,omitempty"`
	Regex        string   `json:"regex,omitempty"`
	SourceLabels []string `json:"sourceLabels,omitempty"`
	TargetLabel  string   `json:"targetLabel,omitempty"`
	Replacement  string   `json:"replacement,omitempty"`
}

// RemoteFilterActionStatus defines the observed state of RemoteFilterAction
// +k8s:openapi-gen=true
type RemoteFilterActionStatus struct {
	// INSERT ADDITIONAL STATUS FIELD - define observed state of cluster
	// Important: Run "operator-sdk generate k8s" to regenerate code after modifying this file
	// Add custom validation using kubebuilder tags: https://book-v1.book.kubebuilder.io/beyond_basics/generating_crd.html
}

// +k8s:deepcopy-gen:interfaces=k8s.io/apimachinery/pkg/runtime.Object

// RemoteFilterAction is the Schema for the remotefilteractions API
// +k8s:openapi-gen=true
// +kubebuilder:subresource:status
// +kubebuilder:resource:path=remotefilteractions,scope=Namespaced
type RemoteFilterAction struct {
	metav1.TypeMeta   `json:",inline"`
	metav1.ObjectMeta `json:"metadata,omitempty"`

	Spec   RemoteFilterActionSpec   `json:"spec,omitempty"`
	Status RemoteFilterActionStatus `json:"status,omitempty"`
}

// +k8s:deepcopy-gen:interfaces=k8s.io/apimachinery/pkg/runtime.Object

// RemoteFilterActionList contains a list of RemoteFilterAction
type RemoteFilterActionList struct {
	metav1.TypeMeta `json:",inline"`
	metav1.ListMeta `json:"metadata,omitempty"`
	Items           []RemoteFilterAction `json:"items"`
}

func init() {
	SchemeBuilder.Register(&RemoteFilterAction{}, &RemoteFilterActionList{})
}
