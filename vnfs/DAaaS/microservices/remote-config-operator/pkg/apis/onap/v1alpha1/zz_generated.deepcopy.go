// +build !ignore_autogenerated

// Code generated by operator-sdk. DO NOT EDIT.

package v1alpha1

import (
	v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	runtime "k8s.io/apimachinery/pkg/runtime"
)

// DeepCopyInto is an autogenerated deepcopy function, copying the receiver, writing into out. in must be non-nil.
func (in *KafkaConfig) DeepCopyInto(out *KafkaConfig) {
	*out = *in
	return
}

// DeepCopy is an autogenerated deepcopy function, copying the receiver, creating a new KafkaConfig.
func (in *KafkaConfig) DeepCopy() *KafkaConfig {
	if in == nil {
		return nil
	}
	out := new(KafkaConfig)
	in.DeepCopyInto(out)
	return out
}

// DeepCopyInto is an autogenerated deepcopy function, copying the receiver, writing into out. in must be non-nil.
func (in *PrometheusRemoteEndpoint) DeepCopyInto(out *PrometheusRemoteEndpoint) {
	*out = *in
	out.TypeMeta = in.TypeMeta
	in.ObjectMeta.DeepCopyInto(&out.ObjectMeta)
	in.Spec.DeepCopyInto(&out.Spec)
	out.Status = in.Status
	return
}

// DeepCopy is an autogenerated deepcopy function, copying the receiver, creating a new PrometheusRemoteEndpoint.
func (in *PrometheusRemoteEndpoint) DeepCopy() *PrometheusRemoteEndpoint {
	if in == nil {
		return nil
	}
	out := new(PrometheusRemoteEndpoint)
	in.DeepCopyInto(out)
	return out
}

// DeepCopyObject is an autogenerated deepcopy function, copying the receiver, creating a new runtime.Object.
func (in *PrometheusRemoteEndpoint) DeepCopyObject() runtime.Object {
	if c := in.DeepCopy(); c != nil {
		return c
	}
	return nil
}

// DeepCopyInto is an autogenerated deepcopy function, copying the receiver, writing into out. in must be non-nil.
func (in *PrometheusRemoteEndpointList) DeepCopyInto(out *PrometheusRemoteEndpointList) {
	*out = *in
	out.TypeMeta = in.TypeMeta
	out.ListMeta = in.ListMeta
	if in.Items != nil {
		in, out := &in.Items, &out.Items
		*out = make([]PrometheusRemoteEndpoint, len(*in))
		for i := range *in {
			(*in)[i].DeepCopyInto(&(*out)[i])
		}
	}
	return
}

// DeepCopy is an autogenerated deepcopy function, copying the receiver, creating a new PrometheusRemoteEndpointList.
func (in *PrometheusRemoteEndpointList) DeepCopy() *PrometheusRemoteEndpointList {
	if in == nil {
		return nil
	}
	out := new(PrometheusRemoteEndpointList)
	in.DeepCopyInto(out)
	return out
}

// DeepCopyObject is an autogenerated deepcopy function, copying the receiver, creating a new runtime.Object.
func (in *PrometheusRemoteEndpointList) DeepCopyObject() runtime.Object {
	if c := in.DeepCopy(); c != nil {
		return c
	}
	return nil
}

// DeepCopyInto is an autogenerated deepcopy function, copying the receiver, writing into out. in must be non-nil.
func (in *PrometheusRemoteEndpointSpec) DeepCopyInto(out *PrometheusRemoteEndpointSpec) {
	*out = *in
	if in.FilterSelector != nil {
		in, out := &in.FilterSelector, &out.FilterSelector
		*out = new(v1.LabelSelector)
		(*in).DeepCopyInto(*out)
	}
	out.QueueConfig = in.QueueConfig
	return
}

// DeepCopy is an autogenerated deepcopy function, copying the receiver, creating a new PrometheusRemoteEndpointSpec.
func (in *PrometheusRemoteEndpointSpec) DeepCopy() *PrometheusRemoteEndpointSpec {
	if in == nil {
		return nil
	}
	out := new(PrometheusRemoteEndpointSpec)
	in.DeepCopyInto(out)
	return out
}

// DeepCopyInto is an autogenerated deepcopy function, copying the receiver, writing into out. in must be non-nil.
func (in *PrometheusRemoteEndpointStatus) DeepCopyInto(out *PrometheusRemoteEndpointStatus) {
	*out = *in
	return
}

// DeepCopy is an autogenerated deepcopy function, copying the receiver, creating a new PrometheusRemoteEndpointStatus.
func (in *PrometheusRemoteEndpointStatus) DeepCopy() *PrometheusRemoteEndpointStatus {
	if in == nil {
		return nil
	}
	out := new(PrometheusRemoteEndpointStatus)
	in.DeepCopyInto(out)
	return out
}

// DeepCopyInto is an autogenerated deepcopy function, copying the receiver, writing into out. in must be non-nil.
func (in *QueueConfig) DeepCopyInto(out *QueueConfig) {
	*out = *in
	return
}

// DeepCopy is an autogenerated deepcopy function, copying the receiver, creating a new QueueConfig.
func (in *QueueConfig) DeepCopy() *QueueConfig {
	if in == nil {
		return nil
	}
	out := new(QueueConfig)
	in.DeepCopyInto(out)
	return out
}

// DeepCopyInto is an autogenerated deepcopy function, copying the receiver, writing into out. in must be non-nil.
func (in *RemoteFilterAction) DeepCopyInto(out *RemoteFilterAction) {
	*out = *in
	out.TypeMeta = in.TypeMeta
	in.ObjectMeta.DeepCopyInto(&out.ObjectMeta)
	in.Spec.DeepCopyInto(&out.Spec)
	out.Status = in.Status
	return
}

// DeepCopy is an autogenerated deepcopy function, copying the receiver, creating a new RemoteFilterAction.
func (in *RemoteFilterAction) DeepCopy() *RemoteFilterAction {
	if in == nil {
		return nil
	}
	out := new(RemoteFilterAction)
	in.DeepCopyInto(out)
	return out
}

// DeepCopyObject is an autogenerated deepcopy function, copying the receiver, creating a new runtime.Object.
func (in *RemoteFilterAction) DeepCopyObject() runtime.Object {
	if c := in.DeepCopy(); c != nil {
		return c
	}
	return nil
}

// DeepCopyInto is an autogenerated deepcopy function, copying the receiver, writing into out. in must be non-nil.
func (in *RemoteFilterActionList) DeepCopyInto(out *RemoteFilterActionList) {
	*out = *in
	out.TypeMeta = in.TypeMeta
	out.ListMeta = in.ListMeta
	if in.Items != nil {
		in, out := &in.Items, &out.Items
		*out = make([]RemoteFilterAction, len(*in))
		for i := range *in {
			(*in)[i].DeepCopyInto(&(*out)[i])
		}
	}
	return
}

// DeepCopy is an autogenerated deepcopy function, copying the receiver, creating a new RemoteFilterActionList.
func (in *RemoteFilterActionList) DeepCopy() *RemoteFilterActionList {
	if in == nil {
		return nil
	}
	out := new(RemoteFilterActionList)
	in.DeepCopyInto(out)
	return out
}

// DeepCopyObject is an autogenerated deepcopy function, copying the receiver, creating a new runtime.Object.
func (in *RemoteFilterActionList) DeepCopyObject() runtime.Object {
	if c := in.DeepCopy(); c != nil {
		return c
	}
	return nil
}

// DeepCopyInto is an autogenerated deepcopy function, copying the receiver, writing into out. in must be non-nil.
func (in *RemoteFilterActionSpec) DeepCopyInto(out *RemoteFilterActionSpec) {
	*out = *in
	if in.SourceLabels != nil {
		in, out := &in.SourceLabels, &out.SourceLabels
		*out = make([]string, len(*in))
		copy(*out, *in)
	}
	return
}

// DeepCopy is an autogenerated deepcopy function, copying the receiver, creating a new RemoteFilterActionSpec.
func (in *RemoteFilterActionSpec) DeepCopy() *RemoteFilterActionSpec {
	if in == nil {
		return nil
	}
	out := new(RemoteFilterActionSpec)
	in.DeepCopyInto(out)
	return out
}

// DeepCopyInto is an autogenerated deepcopy function, copying the receiver, writing into out. in must be non-nil.
func (in *RemoteFilterActionStatus) DeepCopyInto(out *RemoteFilterActionStatus) {
	*out = *in
	return
}

// DeepCopy is an autogenerated deepcopy function, copying the receiver, creating a new RemoteFilterActionStatus.
func (in *RemoteFilterActionStatus) DeepCopy() *RemoteFilterActionStatus {
	if in == nil {
		return nil
	}
	out := new(RemoteFilterActionStatus)
	in.DeepCopyInto(out)
	return out
}
