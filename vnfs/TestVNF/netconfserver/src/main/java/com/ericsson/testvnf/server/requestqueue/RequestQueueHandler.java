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

package com.ericsson.testvnf.server.requestqueue;

import java.util.Queue;
import java.util.concurrent.LinkedBlockingQueue;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import com.ericsson.testvnf.server.models.NetconfMessage;

/**
 * RequestQueueHandler manages incoming netconf requests from clients
 * 
 * @author Ajith Sreekumar
 *
 */
public class RequestQueueHandler {

	private static final Log log = LogFactory.getLog(RequestQueueHandler.class);

	private volatile Queue<NetconfMessage> requestQueue;
	
	public RequestQueueHandler() {
		requestQueue = new LinkedBlockingQueue<>();
	}

	// wait until an element is added to queue, once added return it
	public NetconfMessage waitAndGetMessageFromQueue() throws InterruptedException {
		NetconfMessage netconfMessage = null;
		synchronized (requestQueue) {
			while (true) {
				log.info("Waiting for message to be added to queue..");
				if (!requestQueue.isEmpty()) {
					netconfMessage = requestQueue.element();
					requestQueue.remove(requestQueue.element());
				}
				if (netconfMessage != null) {
					log.info("Message received in queue is taken for processing.");
					break;
				}
				requestQueue.wait();
			}
		}
		return netconfMessage;
	}
	
	//add message into the queue
	public void addToQueue(NetconfMessage netconfMessage) {
		synchronized (requestQueue) {
			log.info("Received a new message in queue.");
			requestQueue.add(netconfMessage);
			requestQueue.notifyAll();
		}
	}

}
