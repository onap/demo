package main

import (
	"os"
	"fmt"
	"log"
	"github.com/colinmarc/hdfs/v2"
)

func main() {
	log.Println("Starting the HDFS writer")
	localSourceFile := os.Args[1]
	hdfsDestination := os.Args[2]

	log.Println("localSourceFile:: "+localSourceFile)
	log.Println("hdfsDestination:: "+hdfsDestination)

	client, _ := hdfs.New("hdfs://hdfs-1-namenode-1.hdfs-1-namenode.hdfs1.svc.cluster.local:8020")
	file, _ := client.Open("/kafka.txt")

	buf := make([]byte, 59)
	file.ReadAt(buf, 48847)
	fmt.Println(string(buf))

}
