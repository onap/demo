
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
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

public class DNSMembershipManager {
	/*
	 * Uses Failure Detector (FD) to keep track of the DNS servers currently active.
	 * @param port: the port that the FD service listens for incoming UDP packets
	 * @param timeout: how often the FD checks the status of client processes
	 * @param threshold: number of missing ping messages before declaring a client dead
	 * @param debug: debug mode on/off
	 */
	
	static Set<String> active = new HashSet<String>();
	
	@SuppressWarnings("static-access")
	public static void main(String[] args) throws IOException, InterruptedException {
		if(args.length != 5) {
			System.out.println("Missing input parameters");
			System.out.println("Usage:");
			System.out.print("\t- java FDServer [public IP address] [port] [timeout (sec)] [threshold] [debug]\n");
			System.exit(0);
		}
		
		// Input parameters: PORT, TIMEOUT, THRESHOLD
		String IPADDR = args[0];
		int PORT = Integer.parseInt(args[1]);
		long TIMEOUT = Long.parseLong(args[2]) * 1000; // convert the FD timeout to milliseconds
		int THRESHOLD = Integer.parseInt(args[3]);
		int debug = Integer.parseInt(args[4]);
		boolean DEBUG;
		if(debug <= 0)
			DEBUG = false;
		else
			DEBUG = true;
		
		// Start Failure Detector
		FDServer fd = new FDServer(IPADDR, PORT, TIMEOUT, THRESHOLD, DEBUG);
		
		// Check the status of client processes periodically. We use the same timeout value as FD
		Set<String> active = new HashSet<String>();
		while(true) {
			Set<String> alive_this_round = fd.getAliveProcesses();
			Iterator<String> iter = alive_this_round.iterator();
			String pid;
			
			// Check if there is some new DNS active
			while(iter.hasNext()) {
				pid = iter.next();
				if(!active.contains(pid)) {
					active.add(pid);
					// Add the new vDNS to the set of vDNS servers
					String script = new String("bash /opt/FDserver/add_dns.sh " + pid);
		            Runtime.getRuntime().exec(script);
					if(DEBUG) System.out.println("Adding process " + pid + " to the list of DNSs");
				}
			}
			
			// Remove possible dead DNSs
			iter = active.iterator();
			while(iter.hasNext()) {
				pid = iter.next();
				if(!alive_this_round.contains(pid)) {
					iter.remove(); // remove element from the iterator to avoid ConcurrentModificationException
					// Remove the new vDNS from the set of vDNS servers
					String script = new String("bash /opt/FDserver/remove_dns.sh " + pid);
		            Runtime.getRuntime().exec(script);
					if(DEBUG) System.out.println("Removing process " + pid + " from the list of DNSs");
				}
			}
			Thread.currentThread().sleep(TIMEOUT);
		}
	}
}
