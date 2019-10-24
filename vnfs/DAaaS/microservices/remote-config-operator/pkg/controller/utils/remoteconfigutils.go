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

package utils

import (
	"context"
	"fmt"
	"os"
	onapv1alpha1 "remote-config-operator/pkg/apis/onap/v1alpha1"
	"sigs.k8s.io/controller-runtime/pkg/client"
)

const (
	defaultWatchLabel     = "remote=m3db1"
	WatchLabelsEnvVar     = "WATCH_LABELS"
	RemoteConfigFinalizer = "finalizer.remoteConfig.onap.org"
)

// GetWatchLabels returns the labels the operator should be watching for changes
func GetWatchLabels() (string, error) {
	labelSelector, found := os.LookupEnv(WatchLabelsEnvVar)
	if !found {
		return defaultWatchLabel, fmt.Errorf("%s must be set", WatchLabelsEnvVar)
	}
	return labelSelector, nil
}

// GetPrometheusRemoteEndpoint returns the prometheusRemoteEndpoint instance in the namespace ns
func GetPrometheusRemoteEndpoint(rc client.Client, ns string) (*onapv1alpha1.PrometheusRemoteEndpoint, error) {
	// Get the PrometheusRemoteEndpoint instance in current namespace to rebuild conf.
	preList := &onapv1alpha1.PrometheusRemoteEndpointList{}
	preOpts := []client.ListOption{
		client.InNamespace("edge1"),
		client.MatchingLabels{"remote": "m3db1"},
	}

	err := rc.List(context.TODO(), preList, preOpts...)
	if err != nil {
		return nil, err
	}
	if preList.Items == nil || len(preList.Items) == 0 {
		return nil, err
	}
	prometheusRemoteEndpoint := &preList.Items[0]
	return prometheusRemoteEndpoint, nil
}

// Contains checks if a string is contained in a list of strings
func Contains(list []string, s string) bool {
	for _, v := range list {
		if v == s {
			return true
		}
	}
	return false
}

// Remove checks and removes a string from a list of strings
func Remove(list []string, s string) []string {
	for i, v := range list {
		if v == s {
			list = append(list[:i], list[i+1:]...)
		}
	}
	return list
}
