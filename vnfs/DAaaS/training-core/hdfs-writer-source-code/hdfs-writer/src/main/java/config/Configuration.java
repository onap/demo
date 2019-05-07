package config;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;


public class Configuration{

    private static Logger log = LoggerFactory.getLogger(Configuration.class);
    private static Map<String, Map<String, Object>> settings;

    public void init(Map<String, Object> yamlConfigs){
        settings = new HashMap<>();

        if(yamlConfigs!=null){
            Iterator<String> keys = yamlConfigs.keySet().iterator();
            while(keys.hasNext()){
                String key = keys.next();

                Object value = yamlConfigs.get(key);

                if(value instanceof Map){
                    Map<String, Object> valueMap = (Map<String, Object>) value;
                    settings.put(key, valueMap);
                }
            }
        }
    log.info(":::Settings initiated :::");
    }

    public static Map<String, Map<String, Object>> getSettings() {
        return settings;
    }
}