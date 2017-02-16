
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

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;

/*
 * Client-side Failure Detector (FD) implementation.
 * @param server_addr: IP address of the remote server
 * @param port: the port that the FD service listens for incoming UDP packets
 * @param timeout: how often the FD checks the status of client processes
 * @param debug: debug mode on/off
 */

public class DNSServer {

	public static void main(String[] args) throws IOException {
		if(args.length != 5) {
			System.out.println("Missing input parameters");
			System.out.println("Usage:");
			System.out.print("\t- java FDClient [process ID] [server IP address] [server port] [timeout (sec)] [debug]\n");
			System.exit(0);
		}
		// IP address and port of the remote server
		String PID = args[0];
		String SERVER_ADDR = args[1];
		int PORT = Integer.parseInt(args[2]);
		long TIMEOUT = Long.parseLong(args[3]) * 1000; // convert the FD timeout to milliseconds
		int debug = Integer.parseInt(args[4]);
		boolean DEBUG;
		if(debug <= 0)
			DEBUG = false;
		else
			DEBUG = true;
		
		// Run FD client
		new FDClient(PID, SERVER_ADDR, PORT, TIMEOUT, DEBUG);
		
		// Listen for reply messages
		@SuppressWarnings("resource")
		DatagramSocket sock = new DatagramSocket(PORT);
		byte[] buffer = new byte[256];
		boolean gre_tunnel_enabled = false;
		
		while(true) {
			DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
            sock.receive(packet);
            String[] content = new String(packet.getData()).trim().split(":"); // Remove leading and trailing spaces
            String msg_type = content[0];
            String ip = content[1];
            // Process only PING UDP packets
            if(msg_type.equals("PONG")) {
				if(DEBUG) System.out.println("Reply message received from process " + ip);
				if(!gre_tunnel_enabled) {
					// Set GRE tunnel towards load balancer
					String script = new String("bash /opt/FDclient/set_gre_tunnel.sh " + ip);
		            Runtime.getRuntime().exec(script);
					if(DEBUG) System.out.println("GRE tunnel towards load balancer " + ip + " done");
					gre_tunnel_enabled = true;
				}
            }
            else {
            	if(DEBUG) System.out.println("The received message is not a PONG. Received content: " + content);
            }
		}
	}
}
