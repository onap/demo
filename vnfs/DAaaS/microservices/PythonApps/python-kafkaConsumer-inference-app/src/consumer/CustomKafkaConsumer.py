import logging
from confluent_kafka import Consumer
import json

logging.basicConfig(format='%(asctime)s::%(process)d::%(levelname)s::%(message)s', level=logging.INFO, datefmt='%d-%b-%y %H:%M:%S')


class CustomKafkaConsumer:
    def __init__(self):
        self.output_map = dict()
        #self.topic_name = "metrics3"
        self.topic_name = "adatopic1"
        self.consumer = Consumer({
            #'bootstrap.servers': 'kafka-cluster-kafka-bootstrap:9092',
            'bootstrap.servers': '172.25.103.6:31610',
            'group.id': 'grp1',
            'auto.offset.reset': 'earliest'
        })
        self.duration = 31536000 #50
        self.time_format = 'timestamp' #or 'iso'


    def processMessage(self, msg_key, msg_val):
        python_obj = {}
        try:
            python_obj = json.loads(msg_key)
        except ValueError:
            pass
        try:
            python_obj = json.loads(msg_val)
        except ValueError:
            pass
        print(python_obj["labels"]["__name__"])
        metric_name = python_obj["labels"]["__name__"]
        ip = python_obj["labels"]["instance"]
        if self.time_format == 'iso':
            logging.info("Time_format is ISO-FORMAT")
            iso_time = python_obj["timestamp"]
            logging.info("iso_time:: {}".format(iso_time))
            import dateutil.parser as dp
            parsed_datetime_obj = dp.parse(iso_time)
            from datetime import datetime
            now_datetime_obj = datetime.now()
            st_datetime_obj = now_datetime_obj - datetime.timedelta(seconds= self.duration)
            en_datetime_obj = now_datetime_obj
            if st_datetime_obj <= parsed_datetime_obj and parsed_datetime_obj <= en_datetime_obj:
                logging.info("Parsed a relevant record")
                if metric_name in self.output_map:
                    if ip in self.output_map[metric_name]:
                        self.output_map[metric_name][ip].append(python_obj)
                        logging.info("::Appended a record to existing time series data::")
                    else:
                        self.output_map[metric_name][ip] = list()
                        self.output_map[metric_name][ip].append(python_obj)
                        logging.info("::Appended a recorded to existing time series data with a new ip::")
                else:
                    self.output_map[metric_name] = dict()
                    self.output_map[metric_name][ip] = list()
                    self.output_map[metric_name][ip].append(python_obj)
                    logging.info("::Inserted the first record to a new time series::")
        else:
            logging.info("Time_format is timestamp")
            parsed_timestamp = python_obj["timestamp"]
            logging.info("parsed_timestamp:: {}".format(parsed_timestamp))
            from datetime import datetime, timedelta
            now_datetime_obj = datetime.now()
            st_datetime_obj = now_datetime_obj - timedelta(seconds=self.duration)
            en_datetime_obj = now_datetime_obj
            st_timestamp = int(st_datetime_obj.timestamp()*1000)
            en_timestamp = int(en_datetime_obj.timestamp()*1000)

            logging.info("st_timestamp:: {}".format(st_timestamp))
            logging.info("en_timestamp:: {}".format(en_timestamp))
            if st_timestamp <= parsed_timestamp and en_timestamp>=parsed_timestamp:
                if metric_name in self.output_map:
                    if ip in self.output_map[metric_name]:
                        self.output_map[metric_name][ip].append(python_obj)
                        logging.info("::Appended a record to existing time series data::")
                    else:
                        self.output_map[metric_name][ip] = list()
                        self.output_map[metric_name][ip].append(python_obj)
                        logging.info("::Appended a recorded to existing time series data with a new ip::")
                else:
                    self.output_map[metric_name] = dict()
                    self.output_map[metric_name][ip] = list()
                    self.output_map[metric_name][ip].append(python_obj)
                    logging.info("::Inserted the first record to a new time series::")
        logging.info("The size of the o/p map :: {}".format(len(self.output_map[metric_name][ip])))


    def consume(self):
        self.consumer.subscribe([self.topic_name])
        while True:
            msg = self.consumer.poll(1.0)
            if msg is None:
                logging.info('Looking for message on topic:: {}'.format(self.topic_name))
                continue
            if msg.error():
                print("Consumer error: {}".format(msg.error()))
                continue
            print("msg type:: {} and msg:: {}".format(type(msg), msg))
            print('Received message key from producer: {}'.format(msg.key().decode('utf-8')))
            print('Received message val from producer: {}'.format(msg.value().decode('utf-8')))
            print("mes-key-type:: {}".format(type(msg.key().decode('utf-8'))))
            print("msg-value-type:: {}".format(type(msg.value().decode('utf-8'))))

            self.processMessage(msg.key(), msg.value())
        self.consumer.close()