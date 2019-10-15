package utils


import (
	"os"
)

// SetKafkaParametersByObjectMap sets the  value of the kafka parameters
// and sets the KafkaConfig object 
func SetKafkaParametersByObjectMap(m map[string]interface{}) KafkaConfig {
	kc := KafkaConfig{}
	kc.broker = m["broker"].(string)
	kc.group = m["group"].(string)
	kc.topic = m["topic"].(string)

	return kc
}

// SetKafkaParametersByEnvVariables sets the kafka parameters
func SetKafkaParametersByEnvVariables() KafkaConfig {
	slogger := GetLoggerInstance()
	
	kafkaConfigObject := KafkaConfig{
		broker: os.Getenv("BROKER"),
		group: os.Getenv("GROUP"),
		topic: os.Getenv("TOPIC"),
	}
	slogger.Infof("::broker:: %s", kafkaConfigObject.broker)
	slogger.Infof("::group:: %s", kafkaConfigObject.group)
	slogger.Infof("::topic:: %s", kafkaConfigObject.topic)

	return kafkaConfigObject
}

// KafkaConfig contains all the config parameters needed for kafka. This can be extended over time
type KafkaConfig struct {
	broker string
	group string
	topic string
}

// GetBroker returns kafka broker configured
func (k KafkaConfig) GetBroker() string {
	return k.broker
}

// GetGroup returns kafka group configured
func (k KafkaConfig) GetGroup() string {
	return k.group
}

// GetTopic returns kafka topic configured
func (k KafkaConfig) GetTopic() string {
	return k.topic
}