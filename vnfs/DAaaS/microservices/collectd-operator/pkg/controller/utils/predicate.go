// Copyright 2018 The Operator-SDK Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package utils

import (
	appsv1 "k8s.io/api/apps/v1"

	"sigs.k8s.io/controller-runtime/pkg/event"
	"sigs.k8s.io/controller-runtime/pkg/predicate"
	logf "sigs.k8s.io/controller-runtime/pkg/runtime/log"
)

var plog = logf.Log.WithName("predicate").WithName("eventFilters")

// DaemonSetStatusChangedPredicate implements a default update predicate function on status change for Daemonsets
// (adapted from sigs.k8s.io/controller-runtime/pkg/predicate/predicate.GenerationChangedPredicate)
type DaemonSetStatusChangedPredicate struct {
	predicate.Funcs
}

// Update implements default UpdateEvent filter for validating generation change
func (DaemonSetStatusChangedPredicate) Update(e event.UpdateEvent) bool {
	newDS := e.ObjectNew.DeepCopyObject().(*appsv1.DaemonSet)
	oldDS := e.ObjectOld.DeepCopyObject().(*appsv1.DaemonSet)
	plog.V(2).Info("newDS", "nUNS:=", newDS.Status.UpdatedNumberScheduled, "oUNS:=", oldDS.Status.UpdatedNumberScheduled, "nDNS:=", newDS.Status.DesiredNumberScheduled, "nNR:=", newDS.Status.NumberReady, "nNA:=", newDS.Status.NumberAvailable)
	if newDS.Status.UpdatedNumberScheduled >= oldDS.Status.UpdatedNumberScheduled {
		if (newDS.Status.UpdatedNumberScheduled == newDS.Status.NumberReady) &&
			(newDS.Status.UpdatedNumberScheduled == newDS.Status.NumberAvailable) {
			return true
		}
	}
	if e.MetaOld == nil {
		plog.Error(nil, "Update event has no old metadata", "event", e)
		return false
	}
	if e.ObjectOld == nil {
		plog.Error(nil, "Update event has no old runtime object to update", "event", e)
		return false
	}
	if e.ObjectNew == nil {
		plog.Error(nil, "Update event has no new runtime object for update", "event", e)
		return false
	}
	if e.MetaNew == nil {
		plog.Error(nil, "Update event has no new metadata", "event", e)
		return false
	}
	if e.MetaNew.GetGeneration() == e.MetaOld.GetGeneration() {
		return false
	}

	return true
}
