
/*************************************************************************//**
 *
 * Copyright (c) 2018 AT&T Intellectual Property. All rights reserved.
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

package org.onap.vnf.vlb.write;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.vlb.business.vnf.onap.plugin.rev160918.vlb.business.vnf.onap.plugin.params.vdns.instances.VdnsInstance;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * 
 * Class that maintains a map of all the active vDNS instances.
 * A vDNS instance is represented by its IP address and a flag
 * that represents its status, i.e. enabled or disabled.
 * 
 * Clients can update the vDNS map via NETCONF or RESTconf.
 * Operations are translated by the Honeycomb agent into a JSON
 * object that is persisted on disk (see Honeycomb distribution
 * configuration for more information). The ElementCustomizer
 * class then updates this map. Each operation results in a vLB
 * setup change, e.g. GRE tunnels that connect the vLB to the
 * vDNS instances are created or deleted, depending on the kind
 * of operation that is invoked. Only vDNS instances for which
 * the isEnabled flag is true are currently served by the vLB.
 *
 * The vLB setup and configuration is executed via shell scripts.
 *
 */

public class DnsInstanceManager {

	private static DnsInstanceManager thisInstance = null;
	private static final Logger LOG = LoggerFactory.getLogger(DnsInstanceManager.class);
	private Map<String, VdnsInstance> dnsInstances = new HashMap<String, VdnsInstance>();

	/*
	 * Add a new DNS instance to the map and create a GRE tunnel 
	 * towards that instance if isEnabled is set to true.
	 */

	public static DnsInstanceManager getInstance() {
		if(thisInstance == null) {
			thisInstance = new DnsInstanceManager();
		}

		return thisInstance;
	}

	private DnsInstanceManager() {

	}

	void addDnsInstance(String ipAddr, VdnsInstance data) {
		// Call updateDnsInstance in case the DNS instance already exists.
		// This is somewhat redundant because Honeycomb already runs this check.
		if(dnsInstances.containsKey(ipAddr)) {
			updateDnsInstance(ipAddr, data);
			return;
		}

		dnsInstances.put(ipAddr, data);
		LOG.debug("DNS instance " + ipAddr + " with status isEnabled=" + data.isEnabled() + " has been added.");

		// Create a GRE tunnel towards the new DNS instance if isEnabled is true.
		if(data.isEnabled()) {
			addGreTunnel(ipAddr);
		}
	}

	/*
	 * Update an existing DNS instance and create or remove a GRE
	 * tunnel based on the current value of the isEnabled flag.
	 */
	void updateDnsInstance(String ipAddr, VdnsInstance data) {
		// This is somewhat redundant because Honeycomb already runs this check.
		boolean isEnabled = data.isEnabled();
		if(dnsInstances.get(ipAddr).isEnabled() == isEnabled) {
			LOG.debug("DNS instance " + ipAddr + " with status isEnabled=" + isEnabled + " already exists. No update is necessary.");
			return;
		}

		dnsInstances.put(ipAddr, data);
		LOG.debug("DNS instance " + ipAddr + " has been updated with status isEnabled=" + isEnabled + ".");

		if(isEnabled) {
			addGreTunnel(ipAddr);
		}
		else {
			deleteGreTunnel(ipAddr);
		}
	}

	/*
	 * Delete an existing DNS instance from the map and remove the 
	 * GRE tunnel if isEnabled was set to true.
	 */
	void deleteDnsInstance(String ipAddr) {
		// This is somewhat redundant because Honeycomb already runs this check.
		if(!dnsInstances.containsKey(ipAddr)) {
			LOG.debug("DNS instance " + ipAddr + " doesn't exist.");
			return;
		}

		// Remove a GRE tunnel towards this DNS instance if it exists.
		if(dnsInstances.get(ipAddr).isEnabled()) {
			deleteGreTunnel(ipAddr);
		}

		dnsInstances.remove(ipAddr);
	}

	/*
	 * Auxiliary function that calls a shell script to create a GRE tunnel towards a new vDNS instance.
	 */
	private void addGreTunnel(String ipAddr) {
		String script = new String("bash /opt/add_dns.sh " + ipAddr);
		try {
			Runtime.getRuntime().exec(script);
			LOG.debug("New GRE tunnel towards DNS instance " + ipAddr + " created.");
		} catch (IOException e) {
			LOG.error("add_dns.sh returned an error.");
			e.printStackTrace();
		}
	}

	/*
	 * Auxiliary function that calls a shell script to delete a GRE tunnel towards a vDNS instance.
	 */
	private void deleteGreTunnel(String ipAddr) {
		String script = new String("bash /opt/remove_dns.sh " + ipAddr);
		try {
			Runtime.getRuntime().exec(script);
			LOG.debug("GRE tunnel towards DNS instance " + ipAddr + " removed.");
		} catch (IOException e) {
			LOG.error("remove_dns.sh returned an error.");
			e.printStackTrace();
		}
	}

	/*
	 * Auxiliary function that returns vDNS instances as map.
	 */
	public Map<String, VdnsInstance> getDnsInstancesAsMap() {
		return dnsInstances;
	}
}