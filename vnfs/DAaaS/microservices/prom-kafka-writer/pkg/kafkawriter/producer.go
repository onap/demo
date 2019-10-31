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
	"encoding/json"
	"github.com/prometheus/common/model"
	"github.com/prometheus/prometheus/prompb"
	"gopkg.in/confluentinc/confluent-kafka-go.v1/kafka"
	logger "prom-kafka-writer/pkg/config"
)

var log = logger.GetLoggerInstance()

func PublishTimeSeries(kwid string, metrics *prompb.WriteRequest) error {
	log.Debugw("Remote write Time Series", "length", len(metrics.Timeseries), "TimeSeries", metrics.Timeseries)
	for _, ts := range metrics.Timeseries {
		m := make(model.Metric, len(ts.Labels))
		for _, l := range ts.Labels {
			m[model.LabelName(l.Name)] = model.LabelValue(l.Value)
		}
		log.Debugw("Labels", "Labelset", m)

		for _, s := range ts.Samples {
			log.Debugf("  %f %d\n", s.Value, s.Timestamp)
			metric := map[string]interface{}{
				"name":      m["__name__"],
				"labels":    m,
				"timestamp": s.Timestamp,
				"value":     s.Value,
			}
			key := string(m["__name__"])
			jsonMetric, err := json.Marshal(metric)
			if err != nil {
				log.Errorw("Marshal error", "error", err.Error())
				continue
			}
			err = publish(kwid, key, jsonMetric)
			if err != nil {
				log.Error("Failed to produce message")
				return err
			}
		}
	}
	return nil
}

func publish(kwid string, key string, jsonMetric []byte) error {
	var (
		kwp = KWMap[kwid].Producer
		kwc = KWMap[kwid].Config
	)

	tp := getTopicPartition(kwc)
	kwMsg := kafka.Message{TopicPartition: tp, Key: []byte(key), Value: jsonMetric}
	err := kwp.Produce(&kwMsg, nil)
	if err != nil {
		log.Errorw("Kafka Producer Error", "error", err.Error())
	}
	return err
}

func getTopicPartition(kwc KWConfig) kafka.TopicPartition {
	p := kafka.PartitionAny
	if kwc.UsePartition {
		// TODO: Implement partition strategy
		p = kafka.PartitionAny
	}
	return kafka.TopicPartition{Topic: &kwc.Topic, Partition: p}
}
