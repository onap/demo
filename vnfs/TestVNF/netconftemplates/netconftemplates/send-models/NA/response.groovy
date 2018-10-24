/*
 * ============LICENSE_START=======================================================
 *  Copyright (C) 2016-2018 Ericsson. All rights reserved.
 * ================================================================================
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * ============LICENSE_END=========================================================
 */
 
import com.ericsson.testvnf.server.models.RpcData;

import java.util.concurrent.TimeUnit;
import groovy.sql.Sql;
import groovy.util.XmlParser
import java.util.logging.Logger;
import groovy.json.JsonSlurper 
import groovy.json.JsonOutput 

Logger logger = Logger.getLogger("")

println "send-models!"
def RpcData data = binding.getVariable("RpcData")
Object connectionResetMap = binding.getVariable("connectionResetMap")
println  "connectionResetMap:" + connectionResetMap.get(data.getTargetName())
result = "ignore"
try{
	def operationsData = new XmlParser().parseText(data.getOperationTagContent())
	println "operations data::::" + operationsData
	
	def targetName
	if (!operationsData.config.'target-name'.isEmpty()){
		targetName = operationsData.config.'target-name'.text()
		println "target-name ::::" + operationsData.config.'target-name'.text()
		
		logger.info("in the groovy file. going to open a database session.")
		def db = [url: "jdbc:mariadb://localhost:3306/netconf_db?useSSL=false",
				  user: "root", password: "root", driver: 'org.mariadb.jdbc.Driver']
	//	def db = [url: "jdbc:mariadb://mariadb:3306/netconf_db?useSSL=false",
	//	          user: "root", password: "root", driver: 'org.mariadb.jdbc.Driver']
		def sql = Sql.newInstance(db.url, db.user, db.password, db.driver)
		println "DB connection ready"
		logger.info("DB connection ready")
		def metadata = sql.connection.getMetaData()
		def tables = metadata.getTables(null, null, "AddressTable", null)
		def targetURL = ""
		if (!tables.next()) {
			logger.info("table not found");
			println "table not found"
			sql.execute("CREATE TABLE AddressTable (Name varchar(255), HTTPAddress varchar(255), PRIMARY KEY (Name))")
		}else{
			def query = "select HTTPAddress from AddressTable where Name=\"".concat(targetName).concat("\"")
			println "query" + query
			logger.info(query)
			sql.eachRow(query, { res ->
				println "sending JSON data to "+res.HTTPAddress
				targetURL = res.HTTPAddress
			})
		}
		println "targetURL:"+targetURL
		logger.info(targetURL)
		if(targetURL!="") {
			def url = url = new URL(targetURL)
			
			def message = operationsData.content.text()
			def jsonSlurper = new JsonSlurper()
			Object jsonData = jsonSlurper.parseText(message)
			for (i=0; i<jsonData.size(); i++){
				def post = url.openConnection();
				post.setRequestMethod("POST")
				post.setDoOutput(true)
				post.setRequestProperty("Content-Type", "application/json")
				
				println  "connectionResetMap:" + connectionResetMap.get(data.getTargetName())
				if (connectionResetMap.get(data.getTargetName())){
					println "ConfigUpdate received. Sending a final event."
					post.getOutputStream().write('{"event":{"commonEventHeader":{"startEpochMicrosec":1.53456715899E+12,"sourceId":"mmisVnfName","sequence":1910,"eventId":"ManagedElement=1","domain":"other","lastEpochMicrosec":1.53805815899E+12,"eventName":"PM_ManagedElement=1","internalHeaderFields":{"collectorTimeStamp":"Thu, 09 27 2018 02:22:48 GMT"},"sourceName":"000.111.222.333:4002","priority":"Low","version":2,"reportingEntityName":"ManagedElement=1"},"otherField":{"nameValuePairs":[{"name":"StartTime","value":"2018-1-26 8:49:50"},{"name":"WEB04NY_tej","value":"406"}],"otherFieldVersion":2}}}'.getBytes("UTF-8"))
					def postRC = post.getResponseCode();
					println(postRC);
					if(postRC.equals(200)) {
						println(post.getInputStream().getText());
						result = "<rpc-reply message-id=\"<MID>\" xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\"> <ok/> </rpc-reply>";
					} else{
						println "Final event not accepted. Respnse code "+ postRC+ " received."
					}
					break
				}
				
				def eventMessage = JsonOutput.toJson(jsonData[i])
				println "sending event " + i + ":" + eventMessage
				post.getOutputStream().write(eventMessage.getBytes("UTF-8"));
				def postRC = post.getResponseCode();
				println(postRC);
				if(postRC.equals(200)) {
					println(post.getInputStream().getText());
					result = "<rpc-reply message-id=\"<MID>\" xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\"> <ok/> </rpc-reply>";
				} else{
					println "Event not accepted. Response code "+ postRC+ " received."
				}
				if(i==jsonData.size()-1){
					i=-1
				}
				
				println "sleeping for "+Long.valueOf(data.getTimeDelayForSendingEvents())+" milliseconds.."
				sleep(Long.valueOf(data.getTimeDelayForSendingEvents()))
				
			}
		}else {
			println "No url found in database.."
			logger.info("no url found in database")
			result = "<rpc-reply message-id=\\\"<MID>\\\" xmlns=\\\"urn:ietf:params:xml:ns:netconf:base:1.0\\\"> <rpc-error><error-type>application</error-type><error-message xml:lang=\"en\">No url found in database</error-message></rpc-error></rpc-reply>"
		}
	} else{
		logger.info("targetName not found");
		println("targetName not found")
		result = "<rpc-reply message-id=\\\"<MID>\\\" xmlns=\\\"urn:ietf:params:xml:ns:netconf:base:1.0\\\"> <rpc-error><error-type>application</error-type><error-message xml:lang=\"en\">No target name in request</error-message></rpc-error></rpc-reply>"
	}
}
catch (Exception e)
{
 e.printStackTrace();
}
return result;