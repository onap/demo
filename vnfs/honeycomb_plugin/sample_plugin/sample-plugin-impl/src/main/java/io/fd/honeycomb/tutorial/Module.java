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

package io.fd.honeycomb.tutorial;

import com.google.inject.AbstractModule;
import com.google.inject.multibindings.Multibinder;
import io.fd.honeycomb.data.init.DataTreeInitializer;
import io.fd.honeycomb.translate.read.ReaderFactory;
import io.fd.honeycomb.translate.v3po.util.NamingContext;
import io.fd.honeycomb.translate.write.WriterFactory;
import io.fd.honeycomb.tutorial.init.ConfigDataInitializer;
//import io.fd.honeycomb.tutorial.read.ModuleStateReaderFactory;
import io.fd.honeycomb.tutorial.write.ModuleWriterFactory;
import net.jmob.guice.conf.core.ConfigurationModule;

/**
 * Module class instantiating sample-plugin plugin components.
 */
public final class Module extends AbstractModule {

    @Override
    protected void configure() {
        // requests injection of properties
        install(ConfigurationModule.create());
        requestInjection(ModuleConfiguration.class);

        // bind naming context instance for reader and writer factories
        // the first parameter is artificial name prefix in cases a name needs to be reconstructed for a vxlan tunnel
        // that is present in VPP but not in Honeycomb (could be extracted into configuration)
        // the second parameter is just the naming context ID (could be extracted into configuration)
        binder().bind(NamingContext.class).toInstance(new NamingContext("pgstream", "pgstream-context"));

        // creates reader factory binding
        // can hold multiple binding for separate yang modules
      //  final Multibinder<ReaderFactory> readerFactoryBinder = Multibinder.newSetBinder(binder(), ReaderFactory.class);
      //  readerFactoryBinder.addBinding().to(ModuleStateReaderFactory.class);

        // create writer factory binding
        // can hold multiple binding for separate yang modules
        final Multibinder<WriterFactory> writerFactoryBinder = Multibinder.newSetBinder(binder(), WriterFactory.class);
        writerFactoryBinder.addBinding().to(ModuleWriterFactory.class);

        // create initializer binding
        // can hold multiple binding for separate yang modules
        final Multibinder<DataTreeInitializer> initializerBinder =
                Multibinder.newSetBinder(binder(), DataTreeInitializer.class);
        initializerBinder.addBinding().to(ConfigDataInitializer.class);

        // Disable notification producer for now
//        Multibinder.newSetBinder(binder(), ManagedNotificationProducer.class).addBinding()
//                .to(SampleNotificationProducer.class);
    }
}
