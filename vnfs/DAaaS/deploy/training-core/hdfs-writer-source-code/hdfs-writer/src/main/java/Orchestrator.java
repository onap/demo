import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.dataformat.yaml.YAMLFactory;
import config.Configuration;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.InputStream;
import java.net.URL;
import java.util.HashMap;
import java.util.Map;


public class Orchestrator {

    private static Logger logger = LoggerFactory.getLogger(Orchestrator.class);

    public void init(String configYamlFile){

        parseConfigYaml(configYamlFile);
    }

    private void parseConfigYaml(String configYaml) {

        URL fileUrl = getClass().getResource(configYaml);
        if(fileUrl==null)
            System.out.println("::: Config file missing!!! :::");

        else{
            Configuration conf = new Configuration();
            ObjectMapper mapper = new ObjectMapper(new YAMLFactory());
            String realConfigYaml = configYaml;

            if (!realConfigYaml.startsWith("/")) {
                realConfigYaml = "/" + configYaml;
            }
            Map<String, Object> configs;
            try (InputStream is = getClass().getResourceAsStream(realConfigYaml)) {
                TypeReference<HashMap<String, Object>> typeRef
                        = new TypeReference<HashMap<String, Object>>() {
                };
                configs = mapper.readValue(is, typeRef);
                conf.init(configs);

            } catch (Exception e) {
                logger.error(e.getMessage());
            }
        }
    }
}

