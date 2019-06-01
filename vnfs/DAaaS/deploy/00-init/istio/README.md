/*
 * Copyright 2019 Intel Corporation, Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

# Instructions to Install Istio ServiceMesh

# Step 1 - Install Istio Operator's helm chart

helm install --name=istio-operator --namespace=istio-system istio-operator

# Step 2 - Add the helm chart to install Istio in sds configuration
helm install istio-instance --name istio --namespace istio-system
