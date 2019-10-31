package utils

// import (
// 	"os"
// )

// // SetHdfsParametersByObjectMap set the value of the hdfs config parameters
// // and return HdfsConfig object
// func SetHdfsParametersByObjectMap(m map[string]interface{}) HdfsConfig{

// 	hc := HdfsConfig{}
// 	hc.hdfsURL = m["hdfs_url"].(string)
// 	return hc

// }

// // SetHdfsParametersByEnvVariables sets the hdfs parameters
// func SetHdfsParametersByEnvVariables() HdfsConfig {

// 	slogger := GetLoggerInstance()
// 	hdfsConfigObject := HdfsConfig{
// 		hdfsURL: os.Getenv("HDFS_URL"),
// 	}
// 	slogger.Infof("::hdfsURL:: %s", hdfsConfigObject.hdfsURL)
// 	return hdfsConfigObject

// }

// // HdfsConfig contains hdfs related config items
// type HdfsConfig struct {
// 	hdfsURL string
// }

// // GetHdfsURL returns HdfsURL
// func (h HdfsConfig) GetHdfsURL() string {
// 	return h.hdfsURL
// }