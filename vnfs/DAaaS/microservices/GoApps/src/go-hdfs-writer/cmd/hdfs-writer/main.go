package main

import (
	"context"
	"fmt"
	"net/http"
	"os"
	"os/signal"

	pipeline "hdfs-writer/pkg/pipeline"
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

	connectionsClose := make(chan bool)
	go func() {
		c := make(chan os.Signal, 1)
		signal.Notify(c, os.Interrupt)
		<-c // function literal waiting to receive Interrupt signal
		fmt.Printf(":::Got the kill signal:::")
		slogger.Info(":::Got the kill signal:::")
		for eachWriter, eachChannel := range pipeline.ChannelMap {
			slogger.Info("Begin:: Closing writer goroutines::")
			slogger.Infof("Closing writer goroutine :: %s", eachWriter)
			eachChannel <- true


			// This wait time ensures that the each of the channel is killed before
			// main routine finishes.
			//time.Sleep(time.Second * 5)
		}

		httpServer.Shutdown(context.Background())
		/*once all goroutines are signalled and httpServer is shutdown,
		send close to main thread */
		connectionsClose <- true
		close(connectionsClose)
	}()

	// Sever starts listening
	err := httpServer.ListenAndServe()
	if err != nil && err != http.ErrServerClosed {
		slogger.Fatal(err)
	}
	pipeline.Wg.Wait()
	<-connectionsClose //main thread waiting to receive close signal
}
