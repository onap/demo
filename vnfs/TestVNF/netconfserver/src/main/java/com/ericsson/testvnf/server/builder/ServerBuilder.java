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

package com.ericsson.testvnf.server.builder;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.sshd.common.NamedFactory;
import org.apache.sshd.server.SshServer;
import org.apache.sshd.server.command.Command;
import org.apache.sshd.server.keyprovider.SimpleGeneratorHostKeyProvider;
import org.apache.sshd.server.session.ServerSession;

import com.ericsson.testvnf.server.netconf.NetconfSubsystem;

/**
 * Build the server by extending a netconf subsystem
 */
public class ServerBuilder {

	private static final Log log = LogFactory.getLog(ServerBuilder.class);
	private SshServer sshd;
	
	// initialize the server
	public void initializeServer(String host, int listeningPort) {
		log.info("Configuring server...");
		sshd = SshServer.setUpDefaultServer();
		sshd.setHost(host);
		sshd.setPort(listeningPort);

		log.info("Host: '" + host + "', listenig port: " + listeningPort);

		 // set the password authenticator, here the access is granted always.
		sshd.setPasswordAuthenticator((String username, String password, ServerSession session) -> true);
		 
		sshd.setKeyPairProvider(new SimpleGeneratorHostKeyProvider());

		List<NamedFactory<Command>> subsystemFactories = new ArrayList<>();
		subsystemFactories.add(NetconfSubsystem.Factory.createFactory()); 
		sshd.setSubsystemFactories(subsystemFactories); // add the netconf subystem to the server.

		log.info("Server configured.");
	}

	// start the server
	public void startServer(){
		log.info("Starting server...");
		try {
			sshd.start();
		} catch (IOException e) {
			log.error("Error starting server!", e);
		}
		log.info("Server started.");
	}

	// stop the server
	public void stopServer() throws IOException {
		log.info("Stopping server...");
		sshd.stop();
		log.info("Server stopped.");
	}
}
