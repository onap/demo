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

package org.onap.vnf.vlb;

import io.fd.honeycomb.translate.read.ReadFailedException;
import io.fd.honeycomb.translate.write.WriteFailedException;
import java.util.Collections;
import java.util.List;
import javax.annotation.Nonnull;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.vlb.business.vnf.onap.plugin.rev160918.VlbBusinessVnfOnapPluginState;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.vlb.business.vnf.onap.plugin.rev160918.vlb.business.vnf.onap.plugin.params.VdnsInstances;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.vlb.business.vnf.onap.plugin.rev160918.vlb.business.vnf.onap.plugin.params.vdns.instances.VdnsInstance;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.vlb.business.vnf.onap.plugin.rev160918.vlb.business.vnf.onap.plugin.params.vdns.instances.VdnsInstanceBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.vlb.business.vnf.onap.plugin.rev160918.vlb.business.vnf.onap.plugin.params.vdns.instances.VdnsInstanceKey;
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
final class ElementCrudService implements CrudService<VdnsInstance> {

    private static final Logger LOG = LoggerFactory.getLogger(ElementCrudService.class);

    @Override
    public void writeData(@Nonnull final InstanceIdentifier<VdnsInstance> identifier, @Nonnull final VdnsInstance data)
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
    public void deleteData(@Nonnull final InstanceIdentifier<VdnsInstance> identifier, @Nonnull final VdnsInstance data)
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
    public void updateData(@Nonnull final InstanceIdentifier<VdnsInstance> identifier, @Nonnull final VdnsInstance dataOld,
                           @Nonnull final VdnsInstance dataNew) throws WriteFailedException {
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
    public VdnsInstance readSpecific(@Nonnull final InstanceIdentifier<VdnsInstance> identifier) throws ReadFailedException {

        // read key specified in path identifier
        final VdnsInstanceKey key = identifier.firstKeyOf(VdnsInstance.class);

        // load data by this key
        // *Key class will always contain key of entity, in this case long value

        return new VdnsInstanceBuilder()
                .setIpAddr(key.getIpAddr())
                .setKey(key)
                .setIsEnabled(true)
                .build();
    }

    @Override
    public List<VdnsInstance> readAll() throws ReadFailedException {
        // read all data under parent node,in this case {@link ModuleState}
        return Collections.singletonList(
                readSpecific(InstanceIdentifier.create(VlbBusinessVnfOnapPluginState.class).child(VdnsInstances.class).child(VdnsInstance.class, new VdnsInstanceKey(""))));
    }
}
