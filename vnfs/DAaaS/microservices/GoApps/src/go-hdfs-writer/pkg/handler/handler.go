package handler


import (
	"fmt"
	"net/http"
	"io/ioutil"
	"encoding/json"
	"github.com/gorilla/mux"

	guuid "github.com/google/uuid"
	pipeline "hdfs-writer/pkg/pipeline"
	utils "hdfs-writer/pkg/utils"
)


var slogger = utils.GetLoggerInstance()
// ChannelMap is the global map to store writerNames as key and channels as values.
var ChannelMap =make(map[string]chan struct{})


// This is a sample test request handler
func testFunc(w http.ResponseWriter, r *http.Request){
	slogger.Info("Invoking testFunc ...")
	w.WriteHeader(http.StatusOK)
	fmt.Fprintln(w,"HTTP Test successful ")
}

// CreateRouter returns a http handler for the registered URLs
func CreateRouter() http.Handler{
	router := mux.NewRouter().StrictSlash(true)
	slogger.Info("Created router ...")
	router.HandleFunc("/test", testFunc).Methods("GET")
	router.HandleFunc("/createWriter", createWriter).Methods("POST")
	router.HandleFunc("/deleteWriter/{writerName}", deleteWriter).Methods("DELETE")
	return router
}


// CreateWriter creates a pipeline
func createWriter(w http.ResponseWriter, r *http.Request){
	reqBody, _ := ioutil.ReadAll(r.Body)
	slogger.Info(string(reqBody))
	var results map[string]interface{}
	json.Unmarshal(reqBody, &results)
	if len(results)==0{
		slogger.Fatalf("Unable to read from the config json file, unable to create configObject map")
	}
	writerStr := "writer"
	writer := results[writerStr].(map[string]interface{})
	kafkaConfigMapObj := writer["kafkaConfig"].(map[string]interface{})
	hdfsConfigObj := writer["hdfsConfig"].(map[string]interface{})

	kc := utils.SetKafkaParametersByObjectMap(kafkaConfigMapObj)
	hc := utils.SetHdfsParametersByObjectMap(hdfsConfigObj)

	//populate the channelMap
	pipelineChan := make(chan struct{})
	slogger.Infof("Channel created by post :: %v", pipelineChan)
	uuid := guuid.New().String()
	//slogger.Infof("guuid :: %s",uuid)
	slogger.Infof(":: Storing writerName and channel in ChannelMap :: ")
	writerName := writerStr+"-"+uuid[len(uuid)-4:]
	slogger.Infof("::writerName:: %s ",writerName)
	ChannelMap[writerName] = pipelineChan
	
	// envoke the go routine to build pipeline
	go pipeline.BuildWriterPipeline(kc,hc, writerName, ChannelMap[writerName])
	successMessage := fmt.Sprintf("Created the writer ::%s", writerName)
	w.WriteHeader(http.StatusOK)
	fmt.Fprintln(w,successMessage)
}


// deleteWriter deletes a given writer pipeline
func deleteWriter(w http.ResponseWriter, r *http.Request){
	vars := mux.Vars(r)
	writerName := vars["writerName"]
	if _, keyExists := ChannelMap[writerName]; keyExists{
		slogger.Infof("::Writer to be closed:: %s",writerName)
		toBeClosedChannel := ChannelMap[writerName]
		close(toBeClosedChannel)
		// deleting the channel from ChannelMap after closure to 
		// avoid closing the closed channel
		delete(ChannelMap, writerName)

		w.WriteHeader(http.StatusOK)
		deleteMessage := fmt.Sprintf("Deleted writer :: %s",writerName)
		fmt.Fprintln(w,deleteMessage)
		
	}else{
		notFoundMessage := fmt.Sprintf("Could not find writer :: %s",writerName)
		fmt.Fprintln(w,notFoundMessage)
	}
	
}