/*
 *
 * Copyright 2019 Intel Corporation.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

package api

import (
	"github.com/gorilla/mux"
	"github.com/prometheus/client_golang/prometheus/promhttp"
)

func NewRouter() *mux.Router {
	r := mux.NewRouter()
	r.HandleFunc("/pkw", ListKWHandler).Methods("GET")
	r.HandleFunc("/pkw", CreateKWHandler).Methods("POST")
	r.HandleFunc("/pkw/{kwid}", DeleteKWHandler).Methods("DELETE")
	r.HandleFunc("/pkw/{kwid}/receive", ReceiveKWHandler).Methods("POST")

	// Metrics Handler for prom-kafka-writer
	r.Handle("/metrics", promhttp.Handler())
	return r
}
