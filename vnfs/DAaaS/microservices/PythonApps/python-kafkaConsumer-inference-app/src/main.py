#!/usr/bin/env python3
# """Django's command-line utility for administrative tasks."""

# from .consumer.CustomKafkaConsumer import CustomKafkaConsumer
# from .producer.CustomKafkaProducer import CustomKafkaProducer

import sys
import os
import traceback
import json

from consumer import CustomKafkaConsumer
from producer import CustomKafkaProducer


def main():


    # customKafkaProducer = CustomKafkaProducer.CustomKafkaProducer()
    # with open("./smallTopic10.json") as input_file:
    #     for each_line in input_file:
    #         python_obj = json.loads(each_line)
    #         # print(python_obj["labels"]["__name__"])
    #         customKafkaProducer.produce(each_line, python_obj["labels"]["__name__"])

    customKafkaConsumer = CustomKafkaConsumer.CustomKafkaConsumer()
    customKafkaConsumer.consume()


if __name__ == '__main__':
    main()