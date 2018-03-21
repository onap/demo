/*
 * Copyright (c) 2016 Cisco and/or its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 /*
 * Modifications copyright (c) 2018 AT&T Intellectual Property
 */

package org.onap.vnf.health.read;

import org.onap.vnf.health.CrudService;
import org.onap.vnf.health.RESTManager;
import org.onap.vnf.health.RESTManager.Pair;
import org.onap.vnf.vlb.write.DnsInstanceManager;

import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.vlb.business.vnf.onap.plugin.rev160918.vlb.business.vnf.onap.plugin.params.vdns.instances.VdnsInstance;

import io.fd.honeycomb.translate.read.ReadContext;
import io.fd.honeycomb.translate.read.ReadFailedException;
import io.fd.honeycomb.translate.spi.read.Initialized;
import io.fd.honeycomb.translate.spi.read.InitializingReaderCustomizer;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Properties;

import javax.annotation.Nonnull;

import org.onap.vnf.health.CrudService;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.health.vnf.onap.plugin.rev160918.HealthVnfOnapPluginStateBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.health.vnf.onap.plugin.rev160918.health.vnf.onap.plugin.params.HealthCheckBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.health.vnf.onap.plugin.rev160918.health.vnf.onap.plugin.params.health.check.FaultsBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.health.vnf.onap.plugin.rev160918.health.vnf.onap.plugin.params.health.check.faults.Fault;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.health.vnf.onap.plugin.rev160918.health.vnf.onap.plugin.params.health.check.faults.FaultBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.health.vnf.onap.plugin.rev160918.health.vnf.onap.plugin.params.health.check.faults.FaultKey;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.health.vnf.onap.plugin.rev160918.health.vnf.onap.plugin.params.HealthCheck;
import org.opendaylight.yangtools.concepts.Builder;
import org.opendaylight.yangtools.yang.binding.DataObject;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;


/**
 * Reader for {@link Element} list node from our YANG model.
 */
public final class ElementStateCustomizer implements InitializingReaderCustomizer<HealthCheck, HealthCheckBuilder> {

    private final CrudService<HealthCheck> crudService;
    private DnsInstanceManager dnsInstanceManager;
    private static final Logger LOG = LoggerFactory.getLogger(ElementStateCustomizer.class);
    private final String SCRIPT;
    private final String OUTPUT;
    private final String VNFC;
    private final Boolean PRIMARY;
    private static SimpleDateFormat SDF;
    private String vPktGenIp;

    public ElementStateCustomizer(final CrudService<HealthCheck> crudService) throws IOException {
        this.crudService = crudService;
        dnsInstanceManager = DnsInstanceManager.getInstance();

        // initialize data format
        SDF = new SimpleDateFormat("MM-dd-yyyy:HH.mm.ss");

        // read properties from file
        String path = "/opt/config/properties.conf";
		Properties prop = new Properties();
	   	InputStream prop_file = new FileInputStream(path);
	   	prop.load(prop_file);
	   	SCRIPT = prop.getProperty("script");
	   	OUTPUT = prop.getProperty("output");
	   	VNFC = prop.getProperty("vnfc");
	   	PRIMARY = Boolean.parseBoolean(prop.getProperty("primary"));
    	prop_file.close();

    	if(PRIMARY) {
    		vPktGenIp = readFromFile("/opt/config/oam_vpktgen_ip.txt");
    	}
    }
    
    @Override
    public void merge(@Nonnull final Builder<? extends DataObject> builder, @Nonnull final HealthCheck readData) {
        // merge children data to parent builder
        // used by infrastructure to merge data loaded in separated customizers
        ((HealthVnfOnapPluginStateBuilder) builder).setHealthCheck(readData);
    }

    @Nonnull
    @Override
    public HealthCheckBuilder getBuilder(@Nonnull final InstanceIdentifier<HealthCheck> id) {
        // return new builder for this data node
        return new HealthCheckBuilder();
    }

    @Override
    public void readCurrentAttributes(@Nonnull final InstanceIdentifier<HealthCheck> id,
                                      @Nonnull final HealthCheckBuilder builder,
                                      @Nonnull final ReadContext ctx) throws ReadFailedException {

    	// assess the health status of the local service (try at most three times, otherwise return an error).
    	String healthStatus;
    	String [] cmdArgs = {"/bin/bash", "-c", SCRIPT};
    	int ret = -1;
    	int attempts = 0;
    	
    	do {
    		try {
    			Process child = Runtime.getRuntime().exec(cmdArgs);
    			// wait for child process to terminate
    			ret = child.waitFor();
    			attempts++;
    		}
    		catch (IOException e) {
    			LOG.error("Command: [" + SCRIPT + "] returned an error.");
    			e.printStackTrace();
    		}
    		catch (InterruptedException e) {
    			LOG.error("Child process: [" + SCRIPT + "] returned an error. Error code: " + ret);
    			e.printStackTrace();
    		}
    	} while(ret != 0 && attempts < 3);

    	if(ret == 0) {
    		healthStatus = readFromFile(OUTPUT);
    		if(healthStatus == null) {
    			healthStatus = "unhealthy";
    		}
    		LOG.info("Assessing the health status of the local component... Return status = \"" + healthStatus + "\"");
    	}
    	else {
    		healthStatus = "unhealthy";
    		LOG.info("Failed to assess the health status of the local component. Return status = \"unhealthy\"");
    	}

    	// check the status of other VNF components, if any
    	if(PRIMARY) {
    		// check the vPacketGenerator first
    		if(vPktGenIp != null) {
    			if(!getRemoteVnfcHealthStatus(vPktGenIp)) {
    				healthStatus = "unhealthy";
    			}
    		}

    		// check all the vDNS instances
    		Map<String, VdnsInstance> activeVdnsInstances = dnsInstanceManager.getDnsInstancesAsMap();
    		Iterator<String> iter = activeVdnsInstances.keySet().iterator();
    		while(iter.hasNext()) {
    			VdnsInstance vdns = activeVdnsInstances.get(iter.next());
    			if(vdns.isEnabled()) {
    				if(!getRemoteVnfcHealthStatus(vdns.getOamIpAddr())) {
        				healthStatus = "unhealthy";
        			}
    			}
    		}
    	}

        // and sets it to builder
        builder.setState(healthStatus);
        builder.setVnfName(VNFC);
        builder.setTime(SDF.format(new Date().getTime()));

        if(healthStatus.equals("unhealthy")) {
        	List<Fault> faultList = new ArrayList<Fault>();
            // build a FaultBuilder object in case of one or more VNF components are reporting faults
            FaultBuilder faultBuilder = new FaultBuilder();
            faultBuilder.setVnfComponent(VNFC);
            faultBuilder.setMessage("The VNF is not running correctly");
            faultBuilder.setKey(new FaultKey(faultBuilder.getVnfComponent()));
            faultList.add(faultBuilder.build());

            // build a FaultsBuilder object that contains a list of Fault instances
            FaultsBuilder faultsBuilder = new FaultsBuilder();
            faultsBuilder.setInfo("One or more VNF components are unreachable");
            faultsBuilder.setFault(faultList);

            // add the Faults object to HealthCheckBuilder
            builder.setFaults(faultsBuilder.build());
        }

        // create the final HealthCheck object
        builder.build();
    }
    /**
     *
     * Initialize configuration data based on operational data.
     * <p/>
     * Very useful when a plugin is initiated but the underlying layer already contains some operation state.
     * Deriving the configuration from existing operational state enables reconciliation in case when
     * Honeycomb's persistence is not available to do the work for us.
     */
    @Nonnull
    @Override
    public Initialized<? extends DataObject> init(@Nonnull final InstanceIdentifier<HealthCheck> id,
                                                  @Nonnull final HealthCheck readValue,
                                                  @Nonnull final ReadContext ctx) {
        return Initialized.create(id, readValue);
    }
    
    private String readFromFile(String path) {
    	// auto close the file reader
    	try (BufferedReader br = new BufferedReader(new FileReader(path))) {
    		String line;
			while ((line = br.readLine()) != null) {
				return line;
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
		return null;
    }

    private boolean getRemoteVnfcHealthStatus(String ipAddr) {
    	// set up the REST manager
        RESTManager mgr = new RESTManager();
		Map<String, String> headers = new HashMap<String, String>();
		headers.put("Content-Type", "application/json");
		headers.put("Accept", "application/json");

		// execute the request
		String URI = "http://" + ipAddr + ":8183/restconf/operational/health-vnf-onap-plugin:health-vnf-onap-plugin-state/health-check";
    	Pair<Integer, String> result = mgr.get(URI, "admin", "admin", headers);

    	return (!result.b.contains("unhealthy"));
    }
}
