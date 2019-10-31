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
	"gopkg.in/confluentinc/confluent-kafka-go.v1/kafka"
	"strconv"
	"sync"
)

//KWConfig - serialized type for config related to Kafka
type KWConfig struct {
	//Broker - Kafka Bootstrap servers (comma separated)
	Broker string `json:"bootstrap.servers"`
	//Topic - kafka topic name
	Topic string `json:"topic"`
	//UsePartition - Enforce use of partitions
	UsePartition bool   `json:"usePartition"`
	BatchMsgNum  int    `json:"batch.num.messages,omitempty"`
	Compression  string `json:"compression.codec,omitempty"`
}

//KWProducer - holds the Kafka Config and associated Kafka Producer
type KWProducer struct {
	Config   KWConfig
	Producer *kafka.Producer
}

//KWRespMap packs the KWConfig and kwid for List Api
type KWRespMap map[string]KWConfig

//KWMap - Stores the Kafka Writer to Kafka Producer Mapping
// 		  This is used to uniquely identify a Kafka Writer - Producer mapping.
var (
	KWMap   = make(map[string]KWProducer)
	kwMutex sync.Mutex
	id      int
)

// NewKafkaWriter - creates a new producer using kafka config.
// Handles the remote write from prometheus and send to kafka topic
func NewKafkaWriter(kwc *KWConfig) (*kafka.Producer, error) {
	producer, err := kafka.NewProducer(&kafka.ConfigMap{
		"bootstrap.servers":   kwc.Broker,
		"compression.codec":   kwc.Compression,
		"batch.num.messages":  kwc.BatchMsgNum,
		"go.batch.producer":   true,
		"go.delivery.reports": false,
	})
	if err != nil {
		return nil, err
	}
	return producer, nil
}

//NewKWConfig - creates a KWConfig object with default values
func NewKWConfig() *KWConfig {
	return &KWConfig{
		UsePartition: false,
		BatchMsgNum:  10000,
		Compression:  "none",
	}
}

//NewKWRespMap - packs the KWConfig and kwid for List Api
func newKWRespMap() KWRespMap {
	kwr := make(KWRespMap)
	return kwr
}

//AddKWC - Method to add KafkaWriterConfig request to KWMap
func AddKWC(kwc *KWConfig) (string, error) {
	kwMutex.Lock()
	defer kwMutex.Unlock()
	//TODO: Generate kwid
	kwid := "pkw" + strconv.Itoa(id)
	id++
	producer, err := NewKafkaWriter(kwc)
	if err != nil {
		log.Error("Error", err)
		id--
		return "", err
	}

	KWMap[kwid] = KWProducer{
		Config:   *kwc,
		Producer: producer,
	}
	return kwid, nil
}

//DeleteKWC - Method to add KafkaWriter request to KWMap
func DeleteKWC(kwid string) {
	kwMutex.Lock()
	defer kwMutex.Unlock()
	if _, ok := KWMap[kwid]; ok {
		KWMap[kwid].Producer.Close()
	}
	delete(KWMap, kwid)
}

//ListKWC - Method to add KafkaWriter request to KWMap
func ListKWC() KWRespMap {
	kwr := newKWRespMap()
	for k, v := range KWMap {
		kwr[k] = v.Config
	}
	return kwr
}

//Cleanup - Method to cleanup resources
func Cleanup() {
	for k := range KWMap {
		DeleteKWC(k)
	}
}
