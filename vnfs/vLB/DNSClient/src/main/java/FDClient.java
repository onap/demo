
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

/*
 * Client-side Failure Detector (FD) implementation.
 * @param server_addr: IP address of the remote server
 * @param port: the port that the FD service listens for incoming UDP packets
 * @param timeout: how often the FD checks the status of client processes
 * @param debug: debug mode on/off
 */

public class FDClient implements Runnable {
	private String PID;
	private String SERVER_ADDR;
	private int PORT;
	private long TIMEOUT;
	private boolean DEBUG;
	
	public FDClient(String pid, String server_addr, int port, long timeout, boolean debug) throws IOException {
		PID = pid;
		SERVER_ADDR = server_addr;
		PORT = port;
		TIMEOUT = timeout;
		DEBUG = debug;
		(new Thread(this)).start();
	}

	@SuppressWarnings("resource")
	@Override
	public void run() {
		// Define a DatagramSocket object to send packets to the server
		try {
			DatagramSocket sock = new DatagramSocket();
			InetAddress IPAddress = InetAddress.getByName(SERVER_ADDR);

			// Allocate buffer for the PING message
			String content = "PING:" + PID;
			byte[] buffer = content.getBytes();
		
			// Sent a PING message every TIMEOUT milliseconds
			while(true) {
				DatagramPacket packet = new DatagramPacket(buffer, buffer.length, IPAddress, PORT);
				sock.send(packet);

				if(DEBUG) System.out.println("Sent PING message to server " + SERVER_ADDR + ":" + PORT);
				try {
					Thread.sleep(TIMEOUT);
				}
				catch(InterruptedException e) {}
			} 
		} catch(IOException e) {}
	}
}
