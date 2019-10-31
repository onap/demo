package handler


import (
	"fmt"
	"net/http"
	"io/ioutil"
	"encoding/json"
	"github.com/gorilla/mux"
	"strings"


	pipeline "hdfs-writer/pkg/pipeline"
	utils "hdfs-writer/pkg/utils"

)


var slogger = utils.GetLoggerInstance()

// This is a sample test request handler
func testFunc(w http.ResponseWriter, r *http.Request){
	slogger.Info("Invoking testFunc  ...")
	w.WriteHeader(http.StatusOK)
	fmt.Fprintln(w,"HTTP Test successful ")
}

// CreateRouter returns a http handler for the registered URLs
func CreateRouter() http.Handler{
	router := mux.NewRouter().StrictSlash(true)
	slogger.Info("Created router ...")
	router.HandleFunc("/test", testFunc).Methods("GET")
	router.HandleFunc("/v1/writer", createWriter).Methods("POST")
	router.HandleFunc("/v1/writer/{writerName}", deleteWriter).Methods("DELETE")
	router.HandleFunc("/v1/writers", getAllWriters).Methods("GET")
	return router
}




// CreateWriter creates a pipeline
func createWriter(w http.ResponseWriter, r *http.Request){
	if r.Body == nil {
		http.Error(w, "Empty body", http.StatusBadRequest)
		return
	}
	reqBody, err := ioutil.ReadAll(r.Body)
	if err != nil {
		http.Error(w, err.Error(), http.StatusUnprocessableEntity)
		return

	}
	slogger.Info(string(reqBody))
	var results utils.Pipeline
	json.Unmarshal(reqBody, &results)
	if validateKafkaConfig(results.KafkaConfiguration) == false {
		http.Error(w, "Validation failed for kafka config items, check logs ..", http.StatusBadRequest)
		return
	}
	if validateHdfsConfig(results.HdfsConfiguration) == false {
		http.Error(w, "Validation failed for hdfs config items, check logs ..", http.StatusBadRequest)
		return
	}
	writerName := pipeline.CreatePipeline(results.KafkaConfiguration, results.HdfsConfiguration)
	successMessage := fmt.Sprintf("Created the writer ::%s", writerName)
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusCreated)
	fmt.Fprintln(w,successMessage)
}


// deleteWriter deletes a given writer pipeline
func deleteWriter(w http.ResponseWriter, r *http.Request){
	vars := mux.Vars(r)
	writerName := vars["writerName"]
	if _, keyExists := pipeline.ChannelMap[writerName]; keyExists{
		pipeline.DeletePipeline(writerName)
		w.WriteHeader(http.StatusOK)
		deleteMessage := fmt.Sprintf("Deleted writer :: %s",writerName)
		fmt.Fprintln(w,deleteMessage)
	}else{
		notFoundMessage := fmt.Sprintf("Could not find writer :: %s",writerName)
		fmt.Fprintln(w,notFoundMessage)

	}
}


// validateKafkaConfig validates the kafka config items and returns true if they are valid.
func validateKafkaConfig(k utils.KafkaConfig) bool{
	if strings.TrimSpace(k.Broker) == "" {
		fmt.Println("Broker is empty!")
		slogger.Infof("Broker is empty!")
		return false
	}
	if strings.TrimSpace(k.Group) == "" {
		fmt.Println("Group is empty!")
		slogger.Infof("Group is empty!")
		return false
	}
	if strings.TrimSpace(k.Topic) == "" {
		fmt.Println("Topic is empty!")
		slogger.Infof("Topic is empty!")
		return false
	}
	return true
}


// validateHdfsConfig validates the kafka config items and returns true if they are valid.
func validateHdfsConfig(h utils.HdfsConfig) bool {
	if strings.TrimSpace(h.HdfsURL) == "" {
		fmt.Println("HdfsURL is empty!")
		return false
	}
	return true
}

func getAllWriters(w http.ResponseWriter, r *http.Request){
	slogger.Info("Listing all the writers  ...")
	var listOfWriters []string
	for k := range pipeline.ChannelMap {
		listOfWriters = append(listOfWriters, k)
	}
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	w.Write([]byte(fmt.Sprintf(`{"Writers" : "%v"}`, listOfWriters)))
	//fmt.Fprintln(w,listOfWriters)
	// err := json.NewEncoder(w).Encode(listOfWriters)
	// if err != nil {
	// 	http.Error(w, err.Error(), http.StatusInternalServerError)
	// 	return
	// }
}