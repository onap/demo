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
	"encoding/json"
	"errors"
	"io"
	"io/ioutil"
	"net/http"

	logger "prom-kafka-adapter/pkg/config"
	kw "prom-kafka-adapter/pkg/kafkawriter"

	"github.com/golang/protobuf/proto"
	"github.com/golang/snappy"
	"github.com/gorilla/mux"
	"github.com/prometheus/prometheus/prompb"
)

type kwResponse struct {
	KWid       string       `json:"kwid,omitempty"`
	KWstatus   string       `json:"status,omitempty"`
	KWCRespMap kw.KWRespMap `json:"kafkaWriterConfigs,omitempty"`
}

var log = logger.GetLoggerInstance()

// CreateKWHandler - Creates and starts a Prometheus to Kafka writer
func CreateKWHandler(w http.ResponseWriter, r *http.Request) {
	log.Infow("Received request for Creating Kafka Writer")
	kwConfig := kw.NewKWConfig()
	dec := json.NewDecoder(r.Body)
	dec.DisallowUnknownFields()
	err := dec.Decode(kwConfig)
	switch {
	case err == io.EOF:
		http.Error(w, "Body empty", http.StatusBadRequest)
		return
	case err != nil:
		http.Error(w, err.Error(), http.StatusUnprocessableEntity)
		return
	}
	kwid := kw.AddKWC(kwConfig)

	//Send response back
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusCreated)
	kwResp := kwResponse{
		KWid:     kwid,
		KWstatus: http.StatusText(http.StatusCreated),
	}
	err = json.NewEncoder(w).Encode(kwResp)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
}

// ListKWHandler - Lists the KafkaWriters and its config
func ListKWHandler(w http.ResponseWriter, r *http.Request) {
	log.Infow("Received request for List Kafka Writers", "url", r.URL)
	res := kw.ListKWC()
	//Send response back
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	kwResp := kwResponse{
		KWstatus:   http.StatusText(http.StatusOK),
		KWCRespMap: res,
	}
	err := json.NewEncoder(w).Encode(kwResp)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
}

// DeleteKWHandler - Deletes a given Prometheus to Kafka writer
func DeleteKWHandler(w http.ResponseWriter, r *http.Request) {
	params := mux.Vars(r)
	log.Infow("Received request for Deleting Kafka Writer", "KWID", params["kwid"])
	kw.DeleteKWC(params["kwid"])

	//Send response back
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	kwResp := kwResponse{
		KWstatus: http.StatusText(http.StatusOK),
	}
	err := json.NewEncoder(w).Encode(kwResp)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
}

// ReceiveKWHandler - Publish metrics from Prometheus to Kafka
func ReceiveKWHandler(w http.ResponseWriter, r *http.Request) {
	params := mux.Vars(r)
	kwid := params["kwid"]
	if _, ok := kw.KWMap[kwid]; !ok {
		notRegisteredErr := errors.New("kafka writer not registered").Error()
		log.Error(notRegisteredErr)
		http.Error(w, notRegisteredErr, http.StatusNotFound)
		return
	}
	log.Infow("Produce message on Kafka Writer", "kwid", kwid)

	compressed, err := ioutil.ReadAll(r.Body)
	defer r.Body.Close()
	if err != nil {
		log.Error("error", err.Error())
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	metricBuf, err := snappy.Decode(nil, compressed)
	if err != nil {
		log.Error("error", err.Error())
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	var metrics prompb.WriteRequest
	if err := proto.Unmarshal(metricBuf, &metrics); err != nil {
		log.Error("error", err.Error())
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	err = kw.PublishTimeSeries(kwid, &metrics)
	if err != nil {
		log.Error("error", err.Error())
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
}
