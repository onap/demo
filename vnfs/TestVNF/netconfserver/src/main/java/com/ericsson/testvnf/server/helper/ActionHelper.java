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

package com.ericsson.testvnf.server.helper;

import java.io.OutputStream;
import java.util.Map;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import com.ericsson.testvnf.server.Server;
import com.ericsson.testvnf.server.models.NetconfMessage;
import com.ericsson.testvnf.server.models.RpcData;

/*
 * The helper class which performs actions based on netconf requests
 */
public class ActionHelper {
	
	private static final Log log = LogFactory.getLog(ActionHelper.class);
	private OutputStream					out;
	private Thread groovyCallerThread;
	
	public ActionHelper(OutputStream out){
		this.out = out;
	}
	
	public boolean doActionForRPCRequest(NetconfMessage netconfMessage, Map<String, Boolean> connectionResetMap, boolean sessionClosedStatus){
		// if the client is sending an rpc request for any protocol operation
		RpcData rpcData = (RpcData) netconfMessage;
		if (null != rpcData.getTargetName()) {
			// the connectionResetMap is a shared between multiple client connections coming in for a single netconf server instance
			// true value for a target indicates that sending events to that target needs to be stopped
			// edit-config operation with a target determines the termination of sending events to that target.
			if (rpcData.getOperation().equals("edit-config")) {
				connectionResetMap.put(rpcData.getTargetName(), true);
			} else {
				connectionResetMap.put(rpcData.getTargetName(), false);
			}
		}
		if (rpcData.getOperation().equals("\"close-session\"")) {
			sessionClosedStatus = true;
		}
		String operation = rpcData.getOperation();
		log.info("Received query");

		// on getting a get-schema request from client, the server sends back the yang
		// model of the particular functionality/capability asked for.
		if (operation.equals("get-schema")) {
			doActionForGetSchemaOperation(rpcData);
		} else {
			doActionForOperations(rpcData, operation, connectionResetMap);
		}
		return sessionClosedStatus;
	}
	
	public void doActionForOperations(RpcData rpcData, String operation, Map<String, Boolean> connectionResetMap) {
		groovyCallerThread = new Thread("Groovy caller") {
			@Override
			public void run() {
				rpcData.setTimeDelayForSendingEvents(Server.getTimeDelayForSendingEvents());
				String result ="";
				// pick up and execute the correct groovy file based on the operation and configuration datastore identified from the rpc request
				result = HelperUtils.executeGroovy(Server.getNetconfTemplatesDirectory()
						+ "/netconftemplates/"+operation+"/"+rpcData.getConfigurationDatastore()+"/response.groovy", rpcData, connectionResetMap);
				if (!result.equals("ignore")) {
					result = result.replaceAll("<MID>", rpcData.getMessageId());
					HelperUtils.sendAsBytesToOutput(result.getBytes(), out);
					log.info("Finished writing " + result);
				}
			}
		};
		groovyCallerThread.start();
		log.info("groovyCallerThread started");
	}
	
	public void doActionForGetSchemaOperation(RpcData rpcData){
		log.info("get-schema received.");
		if (null != rpcData.getSchemaDetails()
				&& null != rpcData.getSchemaDetails().getIdentifier()) {
			log.info("Sending schema for capability...");
			String schemaString = HelperUtils.readFile(Server.getNetconfTemplatesDirectory()
					+ "/netconftemplates/"+rpcData.getSchemaDetails().getIdentifier()+"-schema.yang"); // the yang model has to be in the netconftemplates directory with the proper naming format : '<operation>-schema.yang'
			String schemaStringValue = schemaString.replaceAll("<MID>", rpcData.getMessageId()); // Put the correct message id in the response as it is in the request.
			HelperUtils.sendAsBytesToOutput(schemaStringValue.getBytes(), out);
			log.info("Finished writing the schema information");
		}
	}
	
	// the method sends a hello message to the client, hello.xml in the netconftemplates directory is used to send the hello message
	public void sendHelloMessage(){
		log.info("Send hello message.");
		String helloMessage = HelperUtils.readFile(Server.getNetconfTemplatesDirectory()+"/netconftemplates/hello.xml");
    	HelperUtils.sendAsBytesToOutput(helloMessage.getBytes(), out);
    	log.info("Hello message sent.");
	}

	public void interruptGroovyCallerThread() {
		// kill thread if it don't finish naturally
		if (groovyCallerThread!=null && groovyCallerThread.isAlive()) {
			log.info("Killing groovy caller thread");
			groovyCallerThread.interrupt();
		}
		
	}
}
