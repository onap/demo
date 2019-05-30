import config.Configuration;
import org.apache.hadoop.fs.FSDataOutputStream;
import org.apache.hadoop.fs.FileSystem;
import org.apache.kafka.clients.consumer.ConsumerConfig;
import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.clients.consumer.KafkaConsumer;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.*;

public class CreateKafkaConsumer {


    private static Logger log = LoggerFactory.getLogger(CreateKafkaConsumer.class);

    private final String BOOTSTRAP_SERVERS = (String) Configuration.getSettings().get("kafka").get("bootStrapServers");
    private final String GROUP_ID_CONFIG = (String) Configuration.getSettings().get("kafka").get("group_id");
    private final String KEY_DESERIALIZER = (String) Configuration.getSettings().get("kafka").get("key_deserialize_class");
    private final String VAL_DESERIALIZER = (String) Configuration.getSettings().get("kafka").get("value_deserialize_class");
    private final String KAFKA_TOPIC = (String) Configuration.getSettings().get("kafka").get("topic");

    private final String HDFS_URL= (String) Configuration.getSettings().get("hdfs").get("hdfsURL");
    private final String HDFS_REMOTE_FILE = (String) Configuration.getSettings().get("hdfs").get("hdfs_remote_file");

    private KafkaConsumer<String, String> kafkaConsumer;
    private Properties properties = new Properties();
    private HdfsWriter hdfsWriter;
    private FileSystem hdfsFileSystem;



    public CreateKafkaConsumer() throws IOException{
        setKafkaProperties();
        kafkaConsumer = new KafkaConsumer<>(properties);
        kafkaConsumer.subscribe(Collections.singletonList(KAFKA_TOPIC));
        hdfsWriter = new HdfsWriter();
        hdfsFileSystem = hdfsWriter.createHdfsFileSystem(HDFS_URL);
        log.info(":::Created kafkaConsumer:::");
    }

    private void setKafkaProperties(){

        properties.put(ConsumerConfig.BOOTSTRAP_SERVERS_CONFIG, BOOTSTRAP_SERVERS);
        properties.put(ConsumerConfig.GROUP_ID_CONFIG, GROUP_ID_CONFIG);
        properties.put(ConsumerConfig.KEY_DESERIALIZER_CLASS_CONFIG, KEY_DESERIALIZER);
        properties.put(ConsumerConfig.VALUE_DESERIALIZER_CLASS_CONFIG, VAL_DESERIALIZER);
        log.info(":::Set kafka properties:::");
    }


    public void processKafkaMessage() throws IOException{
        try{
            while(true){
                ConsumerRecords<String, String> recordsPerPartition = kafkaConsumer.poll(100000);
                if(recordsPerPartition.isEmpty())
                    log.info(":::recordsPerPartition is NULL:::");
                else
                    log.info(":::size of recordsPerPartition: "+recordsPerPartition.count()+" :::");

                for(ConsumerRecord<String, String> record:recordsPerPartition){
                    log.info("Topic: "+record.topic());
                    log.info("partition: "+record.partition());
                    log.info("ReceivedKey: "+record.key()+" ReceivedValue: "+record.value());
                    FSDataOutputStream fsDataOutputStream = hdfsWriter.invokeHdfsWriter(hdfsFileSystem, HDFS_REMOTE_FILE);
                    hdfsWriter.writeMessageToHdfs(fsDataOutputStream, record.value());
                    fsDataOutputStream.close();
                }

                }
        }

        finally {
                log.info(":::Closing kafkaConsumer:::");
                kafkaConsumer.close();
        }
    }
}
