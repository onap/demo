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

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.HashMap;
import java.util.Map;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.sshd.common.NamedFactory;
import org.apache.sshd.server.Environment;
import org.apache.sshd.server.ExitCallback;
import org.apache.sshd.server.command.Command;

/*
 * NetconfSubsystem class 
 */
public class NetconfSubsystem implements Command {

	private static final Log log = LogFactory.getLog(Factory.class);
	private InputStream in;
	private OutputStream out;
	private OutputStream error;
	private Thread netconfHandlerThread;
	private Map<String, Boolean> connectionResetMap;
	private NetconfHandler netconfHandler;
	
	public NetconfSubsystem(Map<String, Boolean> connectionResetMap) {
		this.connectionResetMap = connectionResetMap;
	}

	public void start(Environment env) throws IOException {
		// initialize netconf handler
		netconfHandler = new NetconfHandler(in, out, connectionResetMap);
		netconfHandlerThread = new Thread(netconfHandler, "netconfHandler thread");
		netconfHandlerThread.start();
	}

	public void destroy() {
		netconfHandler.interruptThreads();
		try {
			netconfHandlerThread.join(2000);
		} catch (InterruptedException e) {
			log.info("netconfHandler thread joining failed." + e.getMessage());
			Thread.currentThread().interrupt();
		}
		netconfHandlerThread.interrupt();
		log.info("Netconf Subsystem destroyed");
	}

	public static class Factory implements NamedFactory<Command> {

		// a connectionResetMap is maintained for each running instance of a netconf system.
		// this is a simple data structure to determine when sending events to a target needs to be terminated.
		private static Map<String, Boolean> connectionResetMap = new HashMap<>();

		public static Factory createFactory() {
			return new Factory();
		}

		public String getName() {
			return "netconf";
		}
		
		public Command create() {
			log.info("Creating subsystem for netconf");
			return new NetconfSubsystem(connectionResetMap);
		}
		
	}
	
	public InputStream getInputStream() {
		return in;
	}

	public void setInputStream(InputStream in) {
		this.in = in;
	}

	public OutputStream getOutputStream() {
		return out;
	}

	public void setOutputStream(OutputStream out) {
		this.out = out;
	}

	public OutputStream getErrorStream() {
		return error;
	}

	public void setErrorStream(OutputStream error) {
		this.error = error;
	}

	public void setExitCallback(ExitCallback callback) {
		//Set the callback that the shell has to call when it is closed.
	}
}