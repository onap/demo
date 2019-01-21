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
 * Modifications copyright (c) 2019 AT&T Intellectual Property
 */

package io.fd.honeycomb.lcmapi;

import io.fd.honeycomb.translate.read.ReadFailedException;
import io.fd.honeycomb.translate.write.WriteFailedException;
import java.util.Collections;
import java.util.List;
import javax.annotation.Nonnull;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.stream.count.rev190118.StreamCountState;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.stream.count.rev190118.stream.count.params.Streams;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.stream.count.rev190118.stream.count.params.StreamsBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.stream.count.rev190118.StreamNum;
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
final class ElementCrudService implements CrudService<Streams> {

    private static final Logger LOG = LoggerFactory.getLogger(ElementCrudService.class);

    @Override
    public void writeData(@Nonnull final InstanceIdentifier<Streams> identifier, @Nonnull final Streams data)
            throws WriteFailedException {
        if (data != null) {

            // Performs any logic needed for persisting such data
            LOG.info("Writing path[{}] / data [{}]", identifier, data);
        } else {
            throw new WriteFailedException.CreateFailedException(identifier, data,
                    new NullPointerException("Provided data are null"));
        }
    }

    @Override
    public void deleteData(@Nonnull final InstanceIdentifier<Streams> identifier, @Nonnull final Streams data)
            throws WriteFailedException {
        if (data != null) {

            // Performs any logic needed for persisting such data
            LOG.info("Removing path[{}] / data [{}]", identifier, data);
        } else {
            throw new WriteFailedException.DeleteFailedException(identifier,
                    new NullPointerException("Provided data are null"));
        }
    }

    @Override
    public void updateData(@Nonnull final InstanceIdentifier<Streams> identifier, @Nonnull final Streams dataOld,
                           @Nonnull final Streams dataNew) throws WriteFailedException {
        if (dataOld != null && dataNew != null) {

            // Performs any logic needed for persisting such data
            LOG.info("Update path[{}] from [{}] to [{}]", identifier, dataOld, dataNew);
        } else {
            throw new WriteFailedException.DeleteFailedException(identifier,
                    new NullPointerException("Provided data are null"));
        }
    }

    @Override
    public Streams readSpecific(@Nonnull final InstanceIdentifier<Streams> identifier) throws ReadFailedException {

        // load data by this key
        // *Key class will always contain key of entity, in this case long value

        return new StreamsBuilder()
                .setActiveStreams(new StreamNum(1L))
                .build();
    }

    @Override
    public List<Streams> readAll() throws ReadFailedException {
        // read all data under parent node,in this case {@link ModuleState}
        return Collections.singletonList(
                readSpecific(InstanceIdentifier.create(StreamCountState.class).child(Streams.class)));
    }
}
