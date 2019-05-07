import java.io.IOException;

public class kafka2hdfsApp {

    public static void main(String[] args) throws IOException {
        System.out.println("Begin::: kafka2hdfsApp");
        Orchestrator orchestrator = new Orchestrator();
        orchestrator.init(args[1]);

        CreateKafkaConsumer createKafkaConsumer = new CreateKafkaConsumer();
        createKafkaConsumer.processKafkaMessage();

    }
}
