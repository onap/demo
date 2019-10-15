package utils

import (
	"os"
	"io/ioutil"
)


//ReadJSON reads the content of a give file and returns as a string
// used for small config files only.
func ReadJSON(path string) string {
	slogger := GetLoggerInstance()
	jsonFile, err := os.Open(path)
	if err!=nil{
		//fmt.Print(err)
		slogger.Errorf("Unable to open file: %s", path)
		slogger.Errorf("Error::: %s", err)

	}else{
		slogger.Infof("Successfully opened config.json")
	}
	
	defer jsonFile.Close()
	byteValue, _ := ioutil.ReadAll(jsonFile)
	s := string(byteValue)
	return s
}

