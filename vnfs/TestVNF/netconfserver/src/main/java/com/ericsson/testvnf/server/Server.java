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

package com.ericsson.testvnf.server;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Properties;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import com.ericsson.testvnf.server.builder.ServerBuilder;

/**
 * Netconf server using Apache MINA SSHD and netconf4j
 * The SSHD server extends a netconf subsystem
 * 
 * @author Ajith Sreekumar
 * 
 */
public class Server {

	private static final int DEFAULT_PORT = 2052;

	private static final String DEFAULT_HOST = "0.0.0.0";

	private static final Log log = LogFactory.getLog(Server.class);

	private static String netconfTemplatesDirectory;

	private  static String timeDelayForSendingEvents; // time delay in milliseconds
	
	public static String getNetconfTemplatesDirectory() {
		return netconfTemplatesDirectory;
	}

	public static void setNetconfTemplatesDirectory(String netconfTemplatesDirectory) {
		Server.netconfTemplatesDirectory = netconfTemplatesDirectory;
	}

	public static String getTimeDelayForSendingEvents() {
		return timeDelayForSendingEvents;
	}

	public static void setTimeDelayForSendingEvents(String timeDelayForSendingEvents) {
		Server.timeDelayForSendingEvents = timeDelayForSendingEvents;
	}


	public static void main(String[] args) throws IOException{
		setNetconfTemplatesDirectory(args[0]); // the directory which contains the groovy files to perform actions based on netconf requests, and also contains other netconf templates to be sent back to the client
		setTimeDelayForSendingEvents(args[1]); // the delay between sending each event by the VNF
		ServerBuilder serverBuilder = new ServerBuilder();
		
		Properties prop = new Properties();
		String host = DEFAULT_HOST;
		int port = DEFAULT_PORT;
		try {
			InputStream input = new FileInputStream(getNetconfTemplatesDirectory()+ "/netconftemplates/server-config.properties");
			log.info("Setting host and port from the properties file.");
			prop.load(input);
			if(null!=prop.getProperty("host"))
				host = prop.getProperty("host");
			if(null!=prop.getProperty("port"))
				port = Integer.parseInt(prop.getProperty("port"));
		}catch(FileNotFoundException e) {
			log.info("server-config.properties file not found. " + e);
			log.info("Using default host and port");

		}

		serverBuilder.initializeServer(host,port);
		serverBuilder.startServer();
		
		// read lines from input
		BufferedReader buffer = new BufferedReader(new InputStreamReader(System.in));

		while (true) {
			if (buffer.readLine().equalsIgnoreCase("EXIT")) {
				break;
			}
		}

		log.info("Exiting");
		serverBuilder.stopServer();
		System.exit(0);
	}


}