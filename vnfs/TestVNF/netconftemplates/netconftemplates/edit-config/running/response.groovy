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
import groovy.sql.Sql
import groovy.util.XmlParser
import java.util.logging.Logger;
Logger logger = Logger.getLogger("")
println "edit config - running!"
def RpcData data = binding.getVariable("RpcData")
result = "ignore"
try{
	def operationsData = new XmlParser().parseText(data.getOperationTagContent())
	println "operations data::::" + operationsData
	
	def targetName
	def targetAddress
	
	if (!operationsData.config.'target-name'.isEmpty()){
		targetName = operationsData.config.'target-name'.text()
		println "target-name ::::" + operationsData.config.'target-name'.text()
	} else{
		logger.info("targetName not found");
		println("targetName not found")
	}
	if (!operationsData.config.'target-address'.isEmpty()){
		targetAddress = operationsData.config.'target-address'.text()
		println "target-address ::::" + operationsData.config.'target-address'.text()
	} else{
		logger.info("targetAddress not found");
		println ("targetAddress not found");
	}

	def db = [url: "jdbc:mariadb://localhost:3306/netconf_db?useSSL=false",
	          user: "root", password: "root", driver: 'org.mariadb.jdbc.Driver']
//	def db = [url: "jdbc:mariadb://mariadb:3306/netconf_db?useSSL=false",
//	          user: "root", password: "root", driver: 'org.mariadb.jdbc.Driver']
	def sql = Sql.newInstance(db.url, db.user, db.password, db.driver)
	println "DB connection ready"
	logger.info("DB connection ready")
	if (targetName != null && targetAddress != null){
		def metadata = sql.connection.getMetaData()
		def tables = metadata.getTables(null, null, "AddressTable", null)
		if (!tables.next()) {
			logger.info("table not found");
			println "table not found"
			sql.execute("CREATE TABLE AddressTable (Name varchar(255), HTTPAddress varchar(255), PRIMARY KEY (Name))")
		}
		def query = "INSERT INTO AddressTable (NAME, HTTPAddress) VALUES(\""+targetName+"\",\""+targetAddress+"\") ON DUPLICATE KEY UPDATE Name=\""+targetName+"\", HTTPAddress=\""+targetAddress+"\""
		println "query" + query
		logger.info(query)
		sql.execute(query)
		result = "<rpc-reply message-id=\"<MID>\" xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\"> <ok/> </rpc-reply>"
	}else{
	 result = "<rpc-reply message-id=\\\"<MID>\\\" xmlns=\\\"urn:ietf:params:xml:ns:netconf:base:1.0\\\"> <rpc-error><error-type>application</error-type><error-message xml:lang=\"en\">edit-conf failed. Target name or address not found in request.</error-message></rpc-error></rpc-reply>"
	}

} 
catch (Exception e)
{
 e.printStackTrace();
 result = "<rpc-reply message-id=\\\"<MID>\\\" xmlns=\\\"urn:ietf:params:xml:ns:netconf:base:1.0\\\"> <rpc-error><error-type>application</error-type><error-message xml:lang=\"en\">edit-conf failed</error-message></rpc-error></rpc-reply>"
}


return result;
