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

package kafkawriter

import (
	"github.com/stretchr/testify/assert"
	"reflect"
	"testing"
)

func TestNewKafkaWriter(t *testing.T) {
	type args struct {
		kwc *KWConfig
	}
	kwc := NewKWConfig()
	kwc.Broker = "localhost:9092"
	kwc.Topic = "metrics"

	kwc2 := NewKWConfig()
	kwc2.Broker = "localhost:9092"
	kwc2.Topic = "metrics"
	kwc2.BatchMsgNum = 0

	tests := []struct {
		name string
		args args
		want interface{}
	}{
		{
			name: "Test New Kafka Writer",
			args: args{kwc},
			want: "rdkafka#producer-1",
		},
		{
			name: "Test New Kafka Writer Wrong Config",
			args: args{kwc2},
			want: nil,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, _ := NewKafkaWriter(tt.args.kwc)
			if tt.want == nil {
				assert.Equal(t, tt.want, nil)
			} else {
				if !reflect.DeepEqual(got.String(), tt.want) {
					t.Errorf("NewKafkaWriter() = %v, want %v", got, tt.want)
				}
			}
		})
	}
}

func TestNewKWConfig(t *testing.T) {
	tests := []struct {
		name string
		want *KWConfig
	}{
		{
			name: "Test New Kafka Config",
			want: &KWConfig{
				UsePartition: false,
				BatchMsgNum:  10000,
				Compression:  "none",
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := NewKWConfig(); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("NewKWConfig() = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestAddKWC(t *testing.T) {
	type args struct {
		kwc *KWConfig
	}
	kwc := NewKWConfig()
	kwc.Broker = "localhost:9092"
	kwc.Topic = "metrics"
	tests := []struct {
		name string
		args args
		want string
	}{
		{
			name: "Test Add Kafka Writer 1",
			args: args{kwc},
			want: "pkw0",
		},
		{
			name: "Test Add Kafka Writer 2 ",
			args: args{kwc},
			want: "pkw1",
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, _ := AddKWC(tt.args.kwc)
			assert.Equal(t, tt.want, got)

		})
	}
	assert.Equal(t, 2, len(KWMap))
}

func TestDeleteKWC(t *testing.T) {
	type args struct {
		kwid string
	}
	tests := []struct {
		name     string
		args     args
		delcount int
	}{
		{
			name:     "Test Delete Kafka Writer 1",
			args:     args{"pkw0"},
			delcount: 1,
		},
		{
			name:     "Test Delete Kafka Writer Non existent",
			args:     args{"pkw3"},
			delcount: 0,
		},
		{
			name:     "Test Delete Kafka Writer 2",
			args:     args{"pkw1"},
			delcount: 1,
		},
	}
	for _, tt := range tests {
		l := len(KWMap)
		t.Run(tt.name, func(t *testing.T) {
			DeleteKWC(tt.args.kwid)
		})
		assert.Equal(t, l-tt.delcount, len(KWMap))
	}
	assert.Equal(t, 0, len(KWMap))
}

func TestListKWC(t *testing.T) {
	tests := []struct {
		name  string
		init  func() string
		want  KWRespMap
		clean func(string)
	}{
		{
			name: "Test List Kafka Writers Empty",
			want: KWRespMap{"pkw2": {
				Broker:       "localhost:9092",
				Topic:        "metrics",
				UsePartition: false,
				BatchMsgNum:  10000,
				Compression:  "none",
			}},
			init: func() string {
				kwc := NewKWConfig()
				kwc.Broker = "localhost:9092"
				kwc.Topic = "metrics"
				id, _ := AddKWC(kwc)
				return id
			},
			clean: func(id string) {
				DeleteKWC(id)
			},
		},
		{
			name: "Test List Kafka Writers Empty",
			want: KWRespMap{},
			init: func() string {
				return ""
			},
			clean: func(string) {},
		},
	}
	for _, tt := range tests {
		id := tt.init()
		t.Run(tt.name, func(t *testing.T) {
			if got := ListKWC(); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("ListKWC() = %v, want %v", got, tt.want)
			}
		})
		tt.clean(id)
	}
}

func TestCleanup(t *testing.T) {
	tests := []struct {
		name string
		init func()
	}{
		{
			name: "Test List Kafka Writers Empty",
			init: func() {
				kwc := NewKWConfig()
				kwc.Broker = "localhost:9092"
				kwc.Topic = "metrics"
				AddKWC(kwc)
				AddKWC(kwc)
				AddKWC(kwc)
			},
		},
	}
	for _, tt := range tests {
		tt.init()
		t.Run(tt.name, func(t *testing.T) {
			Cleanup()
		})
		assert.Equal(t, 0, len(KWMap))
	}
}
