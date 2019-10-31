package utils


// import (
// 	"os"
// )

// // SetKafkaParametersByObjectMap sets the  value of the kafka parameters
// // and sets the KafkaConfig object
// func SetKafkaParametersByObjectMap(m map[string]interface{}) KafkaConfig {
// 	slogger := GetLoggerInstance()
// 	kc := KafkaConfig{}
// 	if broker, ok := m["broker"].(string); ok{
// 		kc.broker = broker
// 		slogger.Infof("Set the kafka broker::%s", kc.broker)
// 	}
// 	if group, ok := m["group"].(string); ok{
// 		kc.group = group
// 		slogger.Infof("Set the kafka consumer group::%s", kc.group)
// 	}
// 	if topic, ok := m["topic"].(string); ok{
// 		kc.topic = topic
// 		slogger.Infof("Set the kafka topic::%s", kc.topic)
// 	}
// 	return kc
// }

// // SetKafkaParametersByEnvVariables sets the kafka parameters
// func SetKafkaParametersByEnvVariables() KafkaConfig {
// 	slogger := GetLoggerInstance()

// 	kafkaConfigObject := KafkaConfig{
// 		broker: os.Getenv("BROKER"),
// 		group: os.Getenv("GROUP"),
// 		topic: os.Getenv("TOPIC"),
// 	}
// 	slogger.Infof("::broker:: %s", kafkaConfigObject.broker)
// 	slogger.Infof("::group:: %s", kafkaConfigObject.group)
// 	slogger.Infof("::topic:: %s", kafkaConfigObject.topic)

// 	return kafkaConfigObject
// }

// // KafkaConfig contains all the config parameters needed for kafka. This can be extended over time
// // type KafkaConfig struct {
// // 	broker string `json:"broker"`
// // 	group string `json:"group"`
// // 	topic string `json:"topic"`
// // }

// // Broker returns kafka broker configured
// func (k KafkaConfig) Broker() string {
// 	return k.broker
// }

// // Group returns kafka group configured
// func (k KafkaConfig) Group() string {
// 	return k.group
// }

// // Topic returns kafka topic configured
// func (k KafkaConfig) Topic() string {
// 	return k.topic
// }