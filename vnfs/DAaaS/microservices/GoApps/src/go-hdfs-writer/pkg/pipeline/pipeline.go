package pipeline

import (
	"fmt"
	"os"
	"github.com/colinmarc/hdfs"
	"github.com/confluentinc/confluent-kafka-go/kafka"
	utils "hdfs-writer/pkg/utils"
	
)

// BuildWriterPipeline builds a pipeline
func BuildWriterPipeline(k utils.KafkaConfig, h utils.HdfsConfig, writerName string, sigchan chan struct{}) {
	slogger := utils.GetLoggerInstance()
	topics := make([]string, 1)
	topics[0] = k.GetTopic()
	
	c,err := kafka.NewConsumer(&kafka.ConfigMap{
		"bootstrap.servers": k.GetBroker(),
		"broker.address.family": "v4",
		"group.id":              k.GetGroup(),
		"session.timeout.ms":    6000,
		"auto.offset.reset":     "earliest"})

	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to create consumer: %s\n", err)
		os.Exit(1)
	}
	fmt.Printf("Created Consumer %v\n", c)
	err = c.SubscribeTopics(topics, nil)

	run := true
	setUpPipeline := false

	var hdfsFileWriter *hdfs.FileWriter
	var hdfsFileWriterError error
	// HDFS CLIENT CREATION
	//client := utils.GetHdfsClientInstance(h.GetHdfsURL())
	client := utils.CreateHdfsClient(h.GetHdfsURL())
	

	for run==true {
		select {
		case sig := <-sigchan:
			client.Close()
			if hdfsFileWriter!=nil{
				cleanup(hdfsFileWriter)
			}
			slogger.Infof("\nCaught signal %v: terminating the go-routine of writer :: %s\n", sig, writerName)
			run = false
		default:
			//slogger.Info("Running default option ....")
			ev := c.Poll(100)
			if ev == nil {
				continue
			}
			//:: BEGIN : Switch between different types of messages that come out of kafka
			switch e := ev.(type){
			case *kafka.Message:
				slogger.Infof("::: Message on %s\n%s\n", e.TopicPartition, e.Value)
				dataStr := string(e.Value)
				slogger.Infof("byte array ::: %s", []byte(dataStr))
				fileInfo, fileInfoError := client.Stat("/" + k.GetTopic())
				// create file if it doesnt exists already
				if fileInfoError != nil {
					slogger.Infof("Error::: %s",fileInfoError)
					slogger.Infof("Creating file::: %s", "/"+k.GetTopic())
					hdfsFileWriterError = client.CreateEmptyFile("/"+k.GetTopic())
					if hdfsFileWriterError !=nil {
						slogger.Infof("Creation of empty file ::: %s failed\n Error:: %s",
						 "/"+k.GetTopic(), hdfsFileWriterError.Error())
						panic(fmt.Sprintf("Creation of empty file ::: %s failed", k.GetTopic()))
					}
					_= client.Chmod("/"+k.GetTopic(), 0777);
				}
				newDataStr := dataStr + "\n"
				// file exists case, so just append
				hdfsFileWriter, hdfsFileWriterError = client.Append("/"+fileInfo.Name())
				
				if hdfsFileWriterError != nil || hdfsFileWriter==nil{
					if(hdfsFileWriter==nil){
						slogger.Infof("hdfsFileWriter is NULL !!")
					}
					slogger.Infof(":::Appending to file : %s failed:::\nError occured:::%s\n",
					 "/"+k.GetTopic(),hdfsFileWriterError)
					panic(fmt.Sprintf("Appending to file : %s failed", k.GetTopic()))
				}
				bytesWritten, error := hdfsFileWriter.Write([]byte(newDataStr))
				if bytesWritten > 0 && error == nil {
					slogger.Infof("::: Wrote %s to HDFS:::", newDataStr)
					slogger.Infof("::: Wrote %d bytes to HDFS:::", bytesWritten)
					
					if setUpPipeline==false{
						slogger.Infof(" The pipeline with topic: %s and hdfs url %s is setup,"+
						"watching for more messages.. ",k.GetTopic(), h.GetHdfsURL())
						setUpPipeline = true
					}
				} else {
					slogger.Info("::: Unable to write to HDFS\n :::Error:: %s",error)
				}
				hdfsFileWriter.Close()
			
			case kafka.Error:
				// Errors should generally be considered
				// informational, the client will try to
				// automatically recover.
				// But in this example we choose to terminate
				// the application if all brokers are down.
				fmt.Fprintf(os.Stderr, "%% Error: %v: %v\n", e.Code(), e)
				if e.Code() == kafka.ErrAllBrokersDown {
					run = false
				}
			
			default:
				fmt.Printf("Ignored %v\n", e)
			} //:: END : Switch between different types of messages that come out of kafka
		} // END: select channel
	} // END : infinite loop

	fmt.Printf("Closing the consumer")
}

func cleanup(h *hdfs.FileWriter){
	if h!=nil{
		err := h.Close()
		if err!=nil{
			fmt.Printf(":::Error occured while closing the hdfs writer::: \n%s", err.Error())
		}
	}
}