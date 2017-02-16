
/*************************************************************************//**
 *
 * Copyright © 2017 AT&T Intellectual Property. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

package main.java;

import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

/*
 * Server-side Failure Detector (FD) implementation.
 * @param port: the port that the FD service listens for incoming UDP packets
 * @param timeout: how often the FD checks the status of client processes
 * @param threshold: number of missing ping messages before declaring a client dead
 * @param debug: debug mode on/off
 */

public class FDServer implements Runnable {
	// Input parameters
	private String IP_ADDR;
	private int PORT;
	private long TIMEOUT;
	private int THRESHOLD;
	private boolean DEBUG;
	
	// Data structures that store information about alive processes, processes alive/dead this round
	private Map<String, Integer> alive = new ConcurrentHashMap<String, Integer>(); // Key: process IP address; Value: # consecutive missed pings
	private Set<String> alive_this_round = ConcurrentHashMap.newKeySet(); // Needs to be synchronized because it is accessed by multiple threads
	private Set<String> dead = new HashSet<String>();
	
	public FDServer(String ip_addr, int port, long timeout, int threshold, boolean debug) throws IOException {
		IP_ADDR = ip_addr;
		PORT = port;
		TIMEOUT = timeout;
		THRESHOLD = threshold;
		DEBUG = debug;
		(new Thread(this)).start();
	}
	
	@Override
	public void run() {
		try {
			runFD();
		} catch (IOException e) {
			e.printStackTrace();
		}	
	}
	
	private void runFD() throws IOException {
		// Check the status of client processes periodically
		TimerTask timer = new TimerTask() {
			public void run() {
				checkClientStatus();
			}
		};
		new Timer().scheduleAtFixedRate(timer, 0, TIMEOUT);
		
		// Define a DatagramSocket object for receiving incoming UDP packets
		@SuppressWarnings("resource")
		DatagramSocket sock = new DatagramSocket(PORT);
		byte[] buffer = new byte[256];
		
		// Wait for incoming PING messages from remote clients
		while(true) {
			DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
            sock.receive(packet);
            String[] content = new String(packet.getData()).trim().split(":"); // Remove leading and trailing spaces
            String msg_type = content[0];
            String pid = content[1];
            // Process only PING UDP packets
            if(msg_type.equals("PING")) {
            	String ip = packet.getAddress().getHostAddress();
            	alive_this_round.add(pid);
				if(DEBUG) System.out.println("Keep-alive message received from process " + pid + " (sender IP Address: " + ip +")");
				sendReplyMessage(packet.getAddress());
            }
            else {
            	if(DEBUG) System.out.println("The received message is not a PING. Received content: " + content);
            }
		}
	}
	
	private void sendReplyMessage(InetAddress address) throws IOException {
		DatagramSocket sock = new DatagramSocket();
		// Allocate buffer for the PING message
		String content = "PONG:" + IP_ADDR;
		byte[] buffer = content.getBytes();
		// Sent a PONG message
		DatagramPacket packet = new DatagramPacket(buffer, buffer.length, address, PORT);
		sock.send(packet);
		sock.close();
	}

	// Update the list of processes that are alive
	private void checkClientStatus() {
		if(DEBUG) System.out.println("/================================/");
		if(DEBUG) System.out.println("Update status of remote processes");
		// Check if a process alive the previous round is still alive
		// Otherwise increment its counter
		Set<String> alive_processes = alive.keySet();
		Iterator<String> iter = alive_processes.iterator();
		while(iter.hasNext()) {
			String process = iter.next();
			if(!alive_this_round.contains(process)) {
				int counter = alive.get(process) + 1;
				alive.put(process, counter);
				if(DEBUG) System.out.println("Process " + process + " hasn't sent a message " + counter + " time(s) in a row");
				// If the number of consecutive missed ping messages reached the threshold, 
				// then assume the process to be dead
				if(counter == THRESHOLD) {
					dead.add(process);
					if(DEBUG) System.out.println("Process " + process + " is dead");
				}
			}
		}
		
		// Processes alive this round
		iter = alive_this_round.iterator();
		while(iter.hasNext()) {
			String process = iter.next();
			alive.put(process, 0);
			if(DEBUG) System.out.println("Process " + process + " is alive this round");
		}
			
		
		// Remove dead processes
		iter = dead.iterator();
		while(iter.hasNext()) {
			String process = iter.next();
			if(alive.containsKey(process))
				alive.remove(process);
			if(DEBUG) System.out.println("Process " + process + " is removed from the list of alive processes");
		}
		
		// Cleanup
		alive_this_round.clear();
		dead.clear();
		if(DEBUG) System.out.println();
	}
	
	// Return the set of alive processes to up-stream applications
	public Set<String> getAliveProcesses() {
		return alive.keySet();
	}
}
