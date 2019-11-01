import logging
from confluent_kafka import Producer
import traceback

logging.basicConfig(format='%(asctime)s::%(process)d::%(levelname)s::%(message)s', level=logging.INFO, datefmt='%d-%b-%y %H:%M:%S')


class CustomKafkaProducer:
    def __init__(self):
        self.topic_name = "metrics3"
        #self.topic_name = "adatopic1"
        conf = {'bootstrap.servers': 'kafka-cluster-kafka-bootstrap:9092'
                }
        self.producer = Producer(**conf)


    def produce(self, kafka_msg, kafka_key):
        try:
            self.producer.produce(topic=self.topic_name,
                              value=kafka_msg,
                              key=kafka_key,
                              callback=lambda err, msg: self.on_delivery(err, msg)
            )
            self.producer.flush()

        except Exception as e:
            #print("Error during producing to kafka topic. Stacktrace is %s",e)
            logging.error("Error during producing to kafka topic.")
            traceback.print_exc()


    def on_delivery(self, err, msg):
        if err:
            print("Message failed delivery, error: %s", err)
            logging.error('%s raised an error', err)
        else:
            logging.info("Message delivered to %s on partition %s",
                        msg.topic(), msg.partition())