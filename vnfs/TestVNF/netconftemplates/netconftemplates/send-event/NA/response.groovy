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
import groovy.sql.Sql;
import groovy.util.XmlParser
import java.util.logging.Logger;
Logger logger = Logger.getLogger("")

println "send-event!"
def RpcData data = binding.getVariable("RpcData")
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
			def post = new URL(targetURL).openConnection();
			def message = operationsData.content.text()
			post.setRequestMethod("POST")
			post.setDoOutput(true)
			post.setRequestProperty("Content-Type", "application/json")
			post.getOutputStream().write(message.getBytes("UTF-8"));
			def postRC = post.getResponseCode();
			println(postRC);
			if(postRC.equals(200)) {
				println(post.getInputStream().getText());
				result = "<rpc-reply message-id=\"<MID>\" xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\"> <ok/> </rpc-reply>";
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
