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
 * Modifications copyright (c) 2017 AT&T Intellectual Property
 */
package io.fd.honeycomb.tutorial.write;

import com.google.inject.Inject;
import io.fd.honeycomb.translate.impl.write.GenericWriter;
import io.fd.honeycomb.translate.v3po.util.NamingContext;
import io.fd.honeycomb.translate.write.WriterFactory;
import io.fd.honeycomb.translate.write.registry.ModifiableWriterRegistryBuilder;
import javax.annotation.Nonnull;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.sample.plugin.rev160918.SamplePlugin;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.sample.plugin.rev160918.sample.plugin.params.PgStreams;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.sample.plugin.rev160918.sample.plugin.params.pg.streams.PgStream;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.openvpp.jvpp.core.future.FutureJVppCore;

/**
 * Factory producing writers for sample-plugin plugin's data.
 */
public final class ModuleWriterFactory implements WriterFactory {

    private static final InstanceIdentifier<SamplePlugin> ROOT_CONTAINER_ID = InstanceIdentifier.create(SamplePlugin.class);

    /**
     * Injected vxlan naming context shared with writer, provided by this plugin
     */
    @Inject
    private NamingContext pgNamingContext;
    /**
     * Injected jvpp core APIs, provided by Honeycomb's infrastructure
     */
    @Inject
    private FutureJVppCore jvppCore;

    @Override
    public void init(@Nonnull final ModifiableWriterRegistryBuilder registry) {
        // Unlike ReaderFactory, there's no need to add structural writers, just the writers that actually do something

        // register writer for vxlan tunnel
        registry.add(new GenericWriter<>(
                // What part of subtree this writer handles is identified by an InstanceIdentifier
                ROOT_CONTAINER_ID.child(PgStreams.class).child(PgStream.class),
                // Customizer (the actual translation code to do the heavy lifting)
                new PgWriteCustomizer(jvppCore, pgNamingContext)));
    }
}
