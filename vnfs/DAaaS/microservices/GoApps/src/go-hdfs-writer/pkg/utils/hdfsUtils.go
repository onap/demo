package utils

import (
	"fmt"
	"github.com/colinmarc/hdfs"
	//"sync"
	//"go.uber.org/zap"
)


//var clientOnce sync.Once
//var hdfsClient *hdfs.Client
//var slogger *zap.SugaredLogger


//GetHdfsClientInstance returns a singleton hdfsClient instance
// func GetHdfsClientInstance(hdfsURL string) (*hdfs.Client){
// 	clientOnce.Do(func(){
// 		hdfsClient = createHdfsClient(hdfsURL)
// 	})
// 	return hdfsClient
// }

//CreateHdfsClient creates a hdfs client and returns hdfs client
func CreateHdfsClient(hdfsURL string) (*hdfs.Client){
	slogger := GetLoggerInstance()
	hdfsClient, hdfsConnectError := hdfs.New(hdfsURL)
	if hdfsConnectError !=nil {
		slogger.Fatalf(":::Error in create hdfsClient::: %v", hdfsConnectError)
		fmt.Printf("::Unable to initialize hdfsURL, check logs")
	}
	return hdfsClient
}