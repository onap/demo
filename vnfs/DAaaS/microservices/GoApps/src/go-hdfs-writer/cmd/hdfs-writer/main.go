package main

import (
	"context"
	"fmt"
	"net/http"
	"os"
	"os/signal"
	"time"

	handler "hdfs-writer/pkg/handler"
	utils "hdfs-writer/pkg/utils"
)

func main() {

	slogger := utils.GetLoggerInstance()

	// Create the server
	httpServer := &http.Server{
		Handler: handler.CreateRouter(),
		Addr:    ":9393",
	}

	connectionsClose := make(chan struct{})
	go func() {
		c := make(chan os.Signal, 1)
		signal.Notify(c, os.Interrupt)
		<-c // function literal waiting to receive Interrupt signal
		fmt.Printf(":::Got the kill signal:::")
		slogger.Info(":::Got the kill signal:::")
		for eachWriter, eachChannel := range handler.ChannelMap {
			slogger.Infof("Closing writer goroutine :: %s", eachWriter)
			slogger.Infof("eachChannel:: %v", eachChannel)
			close(eachChannel)
			// This wait time ensures that the each of the channel is killed before
			// main routine finishes.
			time.Sleep(time.Second * 5)
		}
		//once all goroutines are signalled, send close to main thread
		httpServer.Shutdown(context.Background())
		close(connectionsClose)
	}()

	// Sever starts listening
	err := httpServer.ListenAndServe()
	if err != nil && err != http.ErrServerClosed {
		slogger.Fatal(err)
	}
	<-connectionsClose //main thread waiting to receive close signal
}
