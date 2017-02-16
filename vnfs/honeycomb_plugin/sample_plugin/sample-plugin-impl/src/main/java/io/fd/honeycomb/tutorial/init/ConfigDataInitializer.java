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

package io.fd.honeycomb.tutorial.init;

import io.fd.honeycomb.data.init.AbstractDataTreeConverter;
import com.google.inject.Inject;
import com.google.inject.name.Named;
import javax.annotation.Nonnull;
import org.opendaylight.controller.md.sal.binding.api.DataBroker;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.sample.plugin.rev160918.SamplePlugin;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.sample.plugin.rev160918.SamplePluginBuilder;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.sample.plugin.rev160918.SamplePluginState;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;

/**
 * Initialize configuration data based on operational data.
 * <p/>
 * Very useful when a plugin is initiated but the underlying layer already contains some operation state.
 * Deriving the configuration from existing operational state enables reconciliation in case when Honeycomb's persistence
 * is not available to do the work for us.
 */
public final class ConfigDataInitializer extends AbstractDataTreeConverter<SamplePluginState, SamplePlugin> {

    @Inject
    public ConfigDataInitializer(@Named("honeycomb-initializer") @Nonnull final DataBroker bindingDataBroker) {
        super(bindingDataBroker, InstanceIdentifier.create(SamplePluginState.class), InstanceIdentifier.create(SamplePlugin.class));
    }

    @Override
    public SamplePlugin convert(final SamplePluginState operationalData) {
        // Transfer all the operational data into configuration
        return new SamplePluginBuilder()
                .setPgStreams(operationalData.getPgStreams())
                .build();
    }
}
