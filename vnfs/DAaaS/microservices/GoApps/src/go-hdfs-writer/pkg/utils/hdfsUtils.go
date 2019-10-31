package utils

import (
	"fmt"
	"github.com/colinmarc/hdfs"
)


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