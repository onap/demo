/*
 *
 * Copyright 2019 Intel Corporation.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

package main

import (
	"context"
	"net/http"
	"os"
	"os/signal"
	"time"

	"prom-kafka-writer/pkg/api"
	logger "prom-kafka-writer/pkg/config"
	kw "prom-kafka-writer/pkg/kafkawriter"
)

const defaultAddr = ":8686"

// main starts an http server on the $PORT environment variable.
func main() {
	log := logger.GetLoggerInstance()

	addr := defaultAddr
	// $PORT environment variable is provided in the Kubernetes deployment.
	if p := os.Getenv("PORT"); p != "" {
		addr = ":" + p
	}

	log.Infow("Starting Prometheus Kafka writer", "addr", addr)
	defer log.Infow("Prometheus Kafka writer Terminated")

	s := &http.Server{
		Handler: api.NewRouter(),
		Addr:    addr,
	}

	// shutdown hook. Wait for clean up if the pod/container is killed
	shutdownChannel := make(chan struct{})
	go func() {
		log.Debug("msg", "Creating shutdown hooks")
		sigChan := make(chan os.Signal, 1)
		signal.Notify(sigChan, os.Interrupt)
		<-sigChan
		log.Debug("msg", "Received os.Interrupt")
		log.Debug("msg", "Initiate cleanup")
		//TODO: Cleanup here
		kw.Cleanup()
		time.Sleep(time.Second * 3)
		_ = s.Shutdown(context.Background())
		close(shutdownChannel)
	}()

	err := s.ListenAndServe()
	if err != nil {
		log.Fatalw("Server Error - Shutting down", "error", err)
	}
	<-shutdownChannel
}
