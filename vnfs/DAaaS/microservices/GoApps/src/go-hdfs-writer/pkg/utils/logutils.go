package utils

import (
	"go.uber.org/zap"
	"fmt"
	"sync"
)



var logOnce sync.Once
var logger *zap.SugaredLogger

//GetLoggerInstance returns a singleton instance of logger
func GetLoggerInstance() (*zap.SugaredLogger){
	logOnce.Do(func(){
		logger = createLogger()
	})
	return logger
}


//createLogger returns a SugaredLogger, sugaredLogger can be directly used to generate logs
func createLogger() (*zap.SugaredLogger){
	logger, err := zap.NewDevelopment()
	if err != nil {
		fmt.Printf("can't initialize zap logger: %v", err)
	}
	defer logger.Sync()
	slogger := logger.Sugar()
	return slogger
}