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
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"github.com/golang/protobuf/proto"
	"github.com/golang/snappy"
	"github.com/prometheus/prometheus/prompb"
	"github.com/stretchr/testify/assert"
	"io"
	"net/http"
	"net/http/httptest"
	"prom-kafka-adapter/pkg/kafkawriter"
	"testing"
)

type errReader int

func (errReader) Read(p []byte) (n int, err error) {
	return 0, errors.New("test error")
}

func TestCreateKWHandler(t *testing.T) {
	tests := []struct {
		name         string
		body         io.Reader
		expectStatus int
		expectResp   *kwResponse
	}{
		{
			name: "Test Create Kafka Writer",
			body: bytes.NewBuffer([]byte(`{
 				  "bootstrap.servers": "kafka-cluster-kafka-bootstrap.kafka.svc.cluster.local:9092",
				  "topic": "adatopic1",
				  "usePartition": false,
  				  "compression.codec": "snappy"
				  }`)),
			expectStatus: http.StatusCreated,
			expectResp: &kwResponse{
				KWid:     "pkw0",
				KWstatus: http.StatusText(http.StatusCreated),
			},
		},
		{
			name: "Test Create Kafka Writer Wrong parameters",
			body: bytes.NewBuffer([]byte(`{
 				  "servers": "kafka-cluster-kafka-bootstrap.kafka.svc.cluster.local:9092",
				  "kafkatopic": "adatopic1",
				  "usePartition": false,
  				  "compression.codec": "snappy"
				  }`)),
			expectStatus: http.StatusUnprocessableEntity,
			expectResp:   &kwResponse{},
		},
		{
			name:         "Test Create Kafka Writer Empty Body",
			body:         bytes.NewBuffer([]byte(nil)),
			expectStatus: http.StatusBadRequest,
			expectResp:   &kwResponse{},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			req := httptest.NewRequest("POST", "/pkw", tt.body)
			rec := httptest.NewRecorder()
			r := NewRouter()
			r.ServeHTTP(rec, req)
			resp := rec.Result()
			assert.Equal(t, tt.expectStatus, resp.StatusCode)
			kwResp := &kwResponse{}
			json.NewDecoder(resp.Body).Decode(&kwResp)
			assert.Equal(t, tt.expectResp, kwResp)
		})
	}
}

func TestListKWHandler(t *testing.T) {

	tests := []struct {
		name         string
		body         string
		expectStatus int
		expectResp   *kwResponse
	}{
		{
			name: "Test List Kafka Writers",
			body: `{
				"bootstrap.servers": "kafka-cluster-kafka-bootstrap.kafka.svc.cluster.local:9092",
				"topic": "adatopic1",
				"usePartition": false,
				"batch.num.messages": 10000,
				"compression.codec": "snappy"
			}`,
			expectStatus: http.StatusOK,
			expectResp: &kwResponse{
				KWstatus: http.StatusText(http.StatusOK),
				KWCRespMap: map[string]kafkawriter.KWConfig{
					"pkw0": {
						Broker:       "kafka-cluster-kafka-bootstrap.kafka.svc.cluster.local:9092",
						Topic:        "adatopic1",
						UsePartition: false,
						BatchMsgNum:  10000,
						Compression:  "snappy",
					},
				},
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			preCreateKW("pkw0", tt.body)
			req := httptest.NewRequest("GET", "/pkw", nil)
			rec := httptest.NewRecorder()
			r := NewRouter()
			r.ServeHTTP(rec, req)
			resp := rec.Result()
			assert.Equal(t, tt.expectStatus, resp.StatusCode)
			kwResp := &kwResponse{}
			json.NewDecoder(resp.Body).Decode(&kwResp)
			assert.Equal(t, tt.expectResp, kwResp)
		})
	}
}

func TestDeleteKWHandler(t *testing.T) {
	tests := []struct {
		name         string
		kwid         string
		expectStatus int
		expectResp   *kwResponse
	}{
		{
			name:         "Test Delete Kafka Writer",
			kwid:         "pkw777",
			expectStatus: http.StatusOK,
			expectResp: &kwResponse{
				KWstatus: http.StatusText(http.StatusOK),
			},
		},
	}
	body := `{
				"bootstrap.servers": "kafka-cluster-kafka-bootstrap.kafka.svc.cluster.local:9092",
				"topic": "adatopic1",
				"usePartition": false,
				"batch.num.messages": 10000,
				"compression.codec": "snappy"
			}`
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			preCreateKW(tt.kwid, body)
			target := fmt.Sprintf("/pkw/%s", tt.kwid)
			req := httptest.NewRequest("DELETE", target, nil)
			r := NewRouter()
			rec := httptest.NewRecorder()
			r.ServeHTTP(rec, req)
			resp := rec.Result()
			assert.Equal(t, tt.expectStatus, resp.StatusCode)
			kwResp := &kwResponse{}
			json.NewDecoder(resp.Body).Decode(&kwResp)
			assert.Equal(t, tt.expectResp, kwResp)
		})
	}
}

func preCreateKW(kwid string, body string) {
	kafkawriter.Cleanup()
	k := []byte(body)
	kwc := &kafkawriter.KWConfig{}
	_ = json.Unmarshal(k, kwc)
	kafkawriter.KWMap[kwid] = kafkawriter.KWProducer{Config: *kwc, Producer: kafkawriter.NewKafkaWriter(kwc)}
}

func TestReceiveKWHandler(t *testing.T) {
	f, err := buildRemoteWriteRequest()
	if err != nil {
		t.Fatal("Could not build prompb.WriteRequest")
	}
	tests := []struct {
		name         string
		kwid         string
		body         io.Reader
		preCreate    bool
		expectStatus int
	}{
		{
			name:         "Test Receive Messages Empty Message",
			kwid:         "pkw111",
			preCreate:    true,
			expectStatus: http.StatusBadRequest,
		},
		{
			name:         "Test Receive Messages",
			kwid:         "pkw111",
			preCreate:    true,
			body:         bytes.NewReader(f),
			expectStatus: http.StatusOK,
		},
		{
			name:         "Test Receive Messages Kafka Writer Not registed",
			kwid:         "pkw222",
			preCreate:    false,
			expectStatus: http.StatusNotFound,
		},
		{
			name:         "Test Receive Messages Kafka Writer Not registed",
			kwid:         "pkw111",
			preCreate:    true,
			body:         errReader(0),
			expectStatus: http.StatusInternalServerError,
		},
	}
	body := `{
				"bootstrap.servers": "kafka-cluster-kafka-bootstrap.kafka.svc.cluster.local:9092",
				"topic": "adatopic1",
				"usePartition": false,
				"batch.num.messages": 10000,
				"compression.codec": "snappy"
			}`
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if tt.preCreate {
				preCreateKW(tt.kwid, body)
			}
			target := fmt.Sprintf("/pkw/%s/receive", tt.kwid)
			req := httptest.NewRequest("POST", target, tt.body)
			r := NewRouter()
			rec := httptest.NewRecorder()
			r.ServeHTTP(rec, req)
			resp := rec.Result()
			assert.Equal(t, tt.expectStatus, resp.StatusCode)
		})
	}
}

func buildRemoteWriteRequest() ([]byte, error) {
	var buf []byte
	samples := []*prompb.TimeSeries{
		&prompb.TimeSeries{
			Labels: []*prompb.Label{
				&prompb.Label{Name: "__name__", Value: "go_gc_duration_seconds_count"},
				&prompb.Label{Name: "endpoint", Value: "http"},
				&prompb.Label{Name: "instance", Value: "10.42.1.101:8686"},
				&prompb.Label{Name: "job", Value: "prom-kafka-adapter"},
				&prompb.Label{Name: "metrics_storage", Value: "kafka_remote"},
				&prompb.Label{Name: "namespace", Value: "edge1"},
				&prompb.Label{Name: "pod", Value: "prom-kafka-adapter-696898f47f-bc5fs"},
				&prompb.Label{Name: "prometheus", Value: "edge1/cp-prometheus-prometheus"},
				&prompb.Label{Name: "prometheus_replica", Value: "prometheus-cp-prometheus-prometheus-0"},
				&prompb.Label{Name: "service", Value: "prom-kafka-adapter"},
			},
			Samples: []prompb.Sample{
				prompb.Sample{
					Value:     17,
					Timestamp: 1572479934007,
				},
				prompb.Sample{
					Value:     19,
					Timestamp: 1572480144007,
				},
			},
		},
	}
	req := &prompb.WriteRequest{
		Timeseries: samples,
	}

	data, err := proto.Marshal(req)
	if err != nil {
		return nil, err
	}

	// snappy uses len() to see if it needs to allocate a new slice. Make the
	// buffer as long as possible.
	if buf != nil {
		buf = buf[0:cap(buf)]
	}
	compressed := snappy.Encode(buf, data)
	return compressed, nil
}
