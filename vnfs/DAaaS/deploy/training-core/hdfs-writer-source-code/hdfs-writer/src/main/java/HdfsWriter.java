import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FSDataOutputStream;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.net.URI;

public class HdfsWriter {

    private static Logger log = LoggerFactory.getLogger(CreateKafkaConsumer.class);


    public FileSystem createHdfsFileSystem(String hdfsDestination) throws IOException {
        Configuration hdfsConfiguration = new Configuration();
        FileSystem hdfsFileSystem = FileSystem.get(URI.create(hdfsDestination), hdfsConfiguration);
        log.info(":::Created hdfsFileSystem:::");
        return hdfsFileSystem;
    }


    public void writeMessageToHdfs(FSDataOutputStream fsDataOutputStream, String bytesFromKafka) throws IOException {
        fsDataOutputStream.writeBytes(bytesFromKafka);
        log.info(":::Wrote to HDFS:::");
    }


    public FSDataOutputStream  invokeHdfsWriter(FileSystem hdfsFileSystem, String hdfsFile) throws IOException {
        FSDataOutputStream fsDataOutputStream;
        if(!hdfsFileSystem.exists(new Path("/"+hdfsFile)))
            fsDataOutputStream = hdfsFileSystem.create(new Path("/"+hdfsFile));
        else
            fsDataOutputStream = hdfsFileSystem.append(new Path("/"+hdfsFile));
        log.info(":::HDFSWriter invoked:::");
        return fsDataOutputStream;
    }

}
