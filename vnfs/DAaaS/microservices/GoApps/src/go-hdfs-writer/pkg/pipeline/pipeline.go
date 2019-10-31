package pipeline

import (
	"fmt"
	"github.com/colinmarc/hdfs"
	"github.com/confluentinc/confluent-kafka-go/kafka"
	guuid "github.com/google/uuid"
	"os"
	"sync"

	utils "hdfs-writer/pkg/utils"
)

var slogger = utils.GetLoggerInstance()

// ChannelMap is the global map to store writerNames as key and channels as values.
//var ChannelMap =make(map[string]chan struct{})
var ChannelMap = make(map[string]chan bool)

// Wg is of type WaitGroup ensures all the writers have enough time to cleanup a
var Wg sync.WaitGroup
var writerStr = "writer"

// CreatePipeline initiates the building of a pipeline
func CreatePipeline(kc utils.KafkaConfig, hc utils.HdfsConfig) string {
	//pipelineChan := make(chan struct{})
	pipelineChan := make(chan bool)
	uuid := guuid.New().String()
	slogger.Infof(":: Storing writerName and channel in ChannelMap :: ")
	writerName := writerStr + "-" + uuid[len(uuid)-4:]
	slogger.Infof("::writerName:: %s ", writerName)
	ChannelMap[writerName] = pipelineChan

	//Every create request shall add 1 to the WaitGroup
	Wg.Add(1)
	// envoke the go routine to build pipeline
	go buildWriterPipeline(kc, hc, writerName, ChannelMap[writerName])
	return writerName
}

// buildWriterPipeline builds a pipeline
func buildWriterPipeline(k utils.KafkaConfig, h utils.HdfsConfig, writerName string, sigchan chan bool) {

	topics := make([]string, 1)
	topics[0] = k.Topic

	c, err := kafka.NewConsumer(&kafka.ConfigMap{
		"bootstrap.servers":     k.Broker,
		"broker.address.family": "v4",
		"group.id":              k.Group,
		"session.timeout.ms":    6000,
		"auto.offset.reset":     "earliest"})

	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to create consumer: %s\n", err)
		slogger.Info("Failed to create consumer: %s", err.Error())
		delete(ChannelMap, writerName)
		Wg.Done()
		return
	}
	fmt.Printf("Created Consumer %v\n", c)
	err = c.SubscribeTopics(topics, nil)

	setUpPipeline := false

	var hdfsFileWriter *hdfs.FileWriter
	var hdfsFileWriterError error
	// HDFS CLIENT CREATION
	//client := utils.GetHdfsClientInstance(h.GetHdfsURL())
	client := utils.CreateHdfsClient(h.HdfsURL)

	for {
		select {
		case sig := <-sigchan:
			defer Wg.Done()
			client.Close()
			if hdfsFileWriter != nil {
				cleanup(hdfsFileWriter)
			}
			slogger.Infof("\nCaught signal %v: terminating the go-routine of writer :: %s\n", sig, writerName)
			close(sigchan)
			return
		default:
			//slogger.Info("Running default option ....")
			ev := c.Poll(100)
			if ev == nil {
				continue
			}
			//:: BEGIN : Switch between different types of messages that come out of kafka
			switch e := ev.(type) {
			case *kafka.Message:
				slogger.Infof("::: Message on %s\n%s\n", e.TopicPartition, e.Value)
				dataStr := string(e.Value)
				slogger.Infof("byte array ::: %s", []byte(dataStr))
				fileInfo, fileInfoError := client.Stat("/" + k.Topic)
				// create file if it doesnt exists already
				if fileInfoError != nil {
					slogger.Infof("Error::: %s", fileInfoError)
					slogger.Infof("Creating file::: %s", "/"+k.Topic)
					hdfsFileWriterError = client.CreateEmptyFile("/" + k.Topic)
					if hdfsFileWriterError != nil {
						slogger.Infof("Creation of empty file ::: %s failed\n Error:: %s",
							"/"+k.Topic, hdfsFileWriterError.Error())
						continue
					}
					_ = client.Chmod("/"+k.Topic, 0777)
				}
				newDataStr := dataStr + "\n"
				// file exists case, so just append
				hdfsFileWriter, hdfsFileWriterError = client.Append("/" + fileInfo.Name())

				if hdfsFileWriterError != nil {
					if hdfsFileWriter == nil {
						slogger.Infof("hdfsFileWriter is NULL !!")
					}
					slogger.Infof(":::Appending to file : %s failed:::\nError occured:::%s\n",
						"/"+k.Topic, hdfsFileWriterError)
					continue
				} else {
					bytesWritten, error := hdfsFileWriter.Write([]byte(newDataStr))
					if bytesWritten > 0 && error == nil {
						slogger.Infof("::: Wrote %s to HDFS:::", newDataStr)
						slogger.Infof("::: Wrote %d bytes to HDFS:::", bytesWritten)

						if setUpPipeline == false {
							slogger.Infof(" The pipeline with topic: %s and hdfs url %s is setup,"+
								"watching for more messages.. ", k.Topic, h.HdfsURL)
							setUpPipeline = true
						}
						hdfsFileWriter.Close()
					} else {
						slogger.Info("::: Unable to write to HDFS\n :::Error:: %s", error)
					}
				}

			case kafka.Error:
				// Errors should generally be considered
				// informational, the client will try to
				// automatically recover.
				// But in this example we choose to terminate
				// the application if all brokers are down.
				fmt.Fprintf(os.Stderr, "%% Error: %v: %v\n", e.Code(), e)
				if e.Code() == kafka.ErrAllBrokersDown {
					return
				}

			default:
				fmt.Printf("Ignored %v\n", e)
			} //:: END : Switch between different types of messages that come out of kafka
		} // END: select channel
	} // END : infinite loop
}

func cleanup(h *hdfs.FileWriter) {
	if h != nil {
		err := h.Close()
		if err != nil {
			fmt.Printf(":::Error occured while closing hdfs writer::: \n%s", err.Error())
		}

	}
	fmt.Printf("\n:::Clean up executed ::: \n")
}

// DeletePipeline deletes a writer pipeline
func DeletePipeline(writerName string) {
	slogger.Infof("::Writer to be closed:: %s", writerName)
	toBeClosedChannel := ChannelMap[writerName]
	toBeClosedChannel <- true
	// deleting the channel from ChannelMap after closure to
	// avoid closing the closed channel
	delete(ChannelMap, writerName)
}
