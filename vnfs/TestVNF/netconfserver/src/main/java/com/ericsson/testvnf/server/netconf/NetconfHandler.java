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

package com.ericsson.testvnf.server.netconf;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.StringReader;
import java.util.Map;
import java.util.stream.Stream;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.XMLReader;
import org.xml.sax.helpers.XMLReaderFactory;

import com.ericsson.testvnf.server.helper.ActionHelper;
import com.ericsson.testvnf.server.helper.CustomParser;
import com.ericsson.testvnf.server.models.Hello;
import com.ericsson.testvnf.server.models.NetconfMessage;
import com.ericsson.testvnf.server.models.RpcData;
import com.ericsson.testvnf.server.requestqueue.RequestQueueHandler;

/*
 * NetconfHandler class which handles the netconf communication with a client
 */
public class NetconfHandler implements Runnable {

	private InputStream in;
	private Map<String, Boolean> connectionResetMap;
	private XMLReader xmlReader;
	private boolean sessionClosed;
	private RequestQueueHandler requestQueueHandler;
	private ActionHelper actionHelper;
	private Thread requestHandlerThread;
	private static final Log log = LogFactory.getLog(NetconfHandler.class);

	public NetconfHandler(InputStream in, OutputStream out, Map<String, Boolean> connectionResetMap) {
		this.connectionResetMap = connectionResetMap;
		this.in = in;
		actionHelper = new ActionHelper(out);
		this.sessionClosed = false;
	}

	public void run() {
		// initialize rpc request handler and request queue
		try {
			requestQueueHandler = new RequestQueueHandler();
			CustomParser customParser = new CustomParser();
			customParser.setRequestQueueHandler(requestQueueHandler); // input requests once processed are added to the request queue.
			xmlReader = XMLReaderFactory.createXMLReader();
			xmlReader.setContentHandler(customParser);
			xmlReader.setErrorHandler(customParser);
		} catch (SAXException e) {
			log.error("Error creating custom rpc request parser.", e);
			return;
		}
		actionHelper.sendHelloMessage(); // server sends hello to the client as soon as the client initiates a connection with the server
		startRequestHandler();
		// start and wait for request handler
		try {
			Thread.sleep(1000);
		} catch (InterruptedException e) {
			Thread.currentThread().interrupt();
			log.warn("Error waiting for thread.", e);
		}
		readInputRequest();
		
	}
	
	// read input requests from the client
	private void readInputRequest(){
		
			StringBuilder netconfMessage = new StringBuilder();
			try (BufferedReader br = new BufferedReader(new InputStreamReader(in));
					Stream<String> lineStream = br.lines();){
				log.info("Parsing message.");
				// rpc requests from clients are parsed and processed
				lineStream.forEach(ele -> {
					ele = ele.trim();
					log.info("current element::" + ele);
					if (ele.contains("]]>]]>")) {
						ele = ele.replace("]]>]]>", "");
					}
					if (ele.contains("</hello>") || ele.contains("</rpc>")) {
						netconfMessage.append(ele + '\n');
						String messageAsString = netconfMessage.toString();
						try {
							log.info("Parsing message---:\n" + messageAsString);
							xmlReader.parse(new InputSource(new StringReader(messageAsString.trim()))); //xmlParser parses the rpc requests
							log.info("Parsing done..");
						} catch (Exception e) {
							log.error("Error parsing. Message---: \n" + messageAsString, e);
							sessionClosed = true;
						}
						netconfMessage.setLength(0);// reset the message as one whole request is complete
					}else {
						netconfMessage.append(ele + '\n');
					}
				});
			} catch (Exception e) {
				log.error("Exception caught in NetconfHandler readInputRequest: "+ e.getMessage());
			} finally {
				interruptThreads();
			}
	}

    // method that performs actions based on the message coming in from the client
	private void startRequestHandler() {
		log.info("start RequestHandler.");
		requestHandlerThread = new Thread("Request handler") {
			@Override
			public void run() {
				while (!sessionClosed) {
					
					NetconfMessage netconfMessage = null;
					try {
						netconfMessage = requestQueueHandler.waitAndGetMessageFromQueue(); // get the message received
					} catch (InterruptedException e) {
						log.warn("Interrupted exception");
						Thread.currentThread().interrupt();
						break;
					}

					if (netconfMessage instanceof Hello) { // if client sends a hello, send a hello message back
						actionHelper.sendHelloMessage();
					} else if (netconfMessage instanceof RpcData) { 
						sessionClosed = actionHelper.doActionForRPCRequest(netconfMessage, connectionResetMap, sessionClosed);
					}else {
						log.warn("Unknown message received.");
					}
				}
				log.info("Request handler ended");
			}

		};
		requestHandlerThread.start();
		log.info("Request handler thread started.");
	}

	public void interruptThreads() {
		actionHelper.interruptGroovyCallerThread();
		if (requestHandlerThread!=null && requestHandlerThread.isAlive()) {
			log.info("Killing request handler thread");
			requestHandlerThread.interrupt();
		}

	}

}
