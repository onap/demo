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

package org.onap.vnf.health;

import io.fd.honeycomb.translate.read.ReadFailedException;
import io.fd.honeycomb.translate.write.WriteFailedException;
import java.util.Collections;
import java.util.List;
import javax.annotation.Nonnull;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.health.vnf.onap.plugin.rev160918.health.vnf.onap.plugin.params.HealthCheckBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.health.vnf.onap.plugin.rev160918.HealthVnfOnapPluginState;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.health.vnf.onap.plugin.rev160918.health.vnf.onap.plugin.params.HealthCheck;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Simple example of class handling Crud operations for plugin.
 * <p/>
 * No real handling, serves just as an illustration.
 *
 * TODO update javadoc
 */
final class ElementCrudService implements CrudService<HealthCheck> {

    private static final Logger LOG = LoggerFactory.getLogger(ElementCrudService.class);

    @Override
    public void writeData(@Nonnull final InstanceIdentifier<HealthCheck> identifier, @Nonnull final HealthCheck data)
            throws WriteFailedException {
        if (data != null) {

            // identifier.firstKeyOf(SomeClassUpperInHierarchy.class) can be used to identify
            // relationships such as to which parent these data are related to

            // Performs any logic needed for persisting such data
            LOG.info("Writing path[{}] / data [{}]", identifier, data);
        } else {
            throw new WriteFailedException.CreateFailedException(identifier, data,
                    new NullPointerException("Provided data are null"));
        }
    }

    @Override
    public void deleteData(@Nonnull final InstanceIdentifier<HealthCheck> identifier, @Nonnull final HealthCheck data)
            throws WriteFailedException {
        if (data != null) {

            // identifier.firstKeyOf(SomeClassUpperInHierarchy.class) can be used to identify
            // relationships such as to which parent these data are related to

            // Performs any logic needed for persisting such data
            LOG.info("Removing path[{}] / data [{}]", identifier, data);
        } else {
            throw new WriteFailedException.DeleteFailedException(identifier,
                    new NullPointerException("Provided data are null"));
        }
    }

    @Override
    public void updateData(@Nonnull final InstanceIdentifier<HealthCheck> identifier, @Nonnull final HealthCheck dataOld,
                           @Nonnull final HealthCheck dataNew) throws WriteFailedException {
        if (dataOld != null && dataNew != null) {

            // identifier.firstKeyOf(SomeClassUpperInHierarchy.class) can be used to identify
            // relationships such as to which parent these data are related to

            // Performs any logic needed for persisting such data
            LOG.info("Update path[{}] from [{}] to [{}]", identifier, dataOld, dataNew);
        } else {
            throw new WriteFailedException.DeleteFailedException(identifier,
                    new NullPointerException("Provided data are null"));
        }
    }

    @Override
    public HealthCheck readSpecific(@Nonnull final InstanceIdentifier<HealthCheck> identifier) throws ReadFailedException {

        // Returns sample data
    	return new HealthCheckBuilder()
    			.setVnfName("scope represented")
    			.setState("sample status")
    			.setTime("01-01-1000:0000")
    			.build();
    }

    @Override
    public List<HealthCheck> readAll() throws ReadFailedException {
        // read all data under parent node,in this case {@link ModuleState}
        return Collections.singletonList(
                readSpecific(InstanceIdentifier.create(HealthVnfOnapPluginState.class).child(HealthCheck.class)));
    }
}
