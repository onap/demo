package utils

// Pipeline type represents a stucture of a general pipeline
type Pipeline struct{
	KafkaConfiguration KafkaConfig `json:"kafkaConfig"`
	HdfsConfiguration HdfsConfig `json:"hdfsConfig"`
}

// HdfsConfig type represents the config items of HDFS
type HdfsConfig struct {
	HdfsURL string `json:"hdfs_url"`
}

// KafkaConfig type represents the config items of Kafka
type KafkaConfig struct {
	Broker string `json:"broker"`
	Group string `json:"group"`
	Topic string `json:"topic"`
}