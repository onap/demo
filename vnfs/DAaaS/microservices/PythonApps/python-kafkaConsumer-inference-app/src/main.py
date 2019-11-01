#!/usr/bin/env python3

# from .consumer.CustomKafkaConsumer import CustomKafkaConsumer
# from .producer.CustomKafkaProducer import CustomKafkaProducer

import sys
import os, threading
import traceback
import json
import concurrent.futures
import logging

from consumer import CustomKafkaConsumer
from producer import CustomKafkaProducer

logging.basicConfig(format='%(asctime)s::%(process)d::%(levelname)s::%(message)s', level=logging.INFO, datefmt='%d-%b-%y %H:%M:%S')

def main():
    #Begin: Sample producer based on file
    customKafkaProducer = CustomKafkaProducer.CustomKafkaProducer()
    with open("./multithreading-metrics.json") as input_file:
        for each_line in input_file:
            python_obj = json.loads(each_line)
            # print(python_obj["labels"]["__name__"])
            customKafkaProducer.produce(each_line, python_obj["labels"]["__name__"])
    #END: Sample producer based on file

    customKafkaConsumer = CustomKafkaConsumer.CustomKafkaConsumer()

    #Form a data structure for query formation
    queries = []
    queries.append({"metric_name" : "go_gc_duration_seconds_count", "ip": "10.42.1.93:8686"})
    queries.append({"metric_name" : 'go_gc_duration_seconds_count', "ip": "10.42.1.92:8686"})

    executor = concurrent.futures.ThreadPoolExecutor(max_workers=1)
    executor.submit(customKafkaConsumer.consume)

    while(True):
        for each_record in queries:
            list_of_records = customKafkaConsumer.executeQuery(each_record["metric_name"], each_record["ip"])
            logging.info("The records collected :: {}".format(list_of_records))
            logging.info("The length of records collected: {}".format(len(list_of_records)))
            print("The records :: {}".format(list_of_records))


if __name__ == '__main__':
    main()