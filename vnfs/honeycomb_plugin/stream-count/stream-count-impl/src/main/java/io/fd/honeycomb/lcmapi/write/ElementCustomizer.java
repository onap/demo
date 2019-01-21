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

package io.fd.honeycomb.lcmapi.write;

import io.fd.honeycomb.lcmapi.CrudService;
import io.fd.honeycomb.translate.spi.write.WriterCustomizer;
import io.fd.honeycomb.translate.write.WriteContext;
import io.fd.honeycomb.translate.write.WriteFailedException;

import java.io.IOException;

import javax.annotation.Nonnull;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.stream.count.rev190118.stream.count.params.Streams;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Writer for {@link Element} list node from our YANG model.
 */

public final class ElementCustomizer implements WriterCustomizer<Streams> {

	private static final Logger LOG = LoggerFactory.getLogger(ElementCustomizer.class);

    private final CrudService<Streams> crudService;

    public ElementCustomizer(@Nonnull final CrudService<Streams> crudService) {
        this.crudService = crudService;
    }

    @Override
    public void writeCurrentAttributes(@Nonnull final InstanceIdentifier<Streams> id, @Nonnull final Streams dataAfter,
                                       @Nonnull final WriteContext writeContext) throws WriteFailedException {
        //perform write of data,or throw exception
        //invoked by PUT operation,if provided data doesn't exist in Config data
        crudService.writeData(id, dataAfter);
        try {
        	runScript(dataAfter.getActiveStreams().getValue());
        }
        catch (IOException e) {
            LOG.error("Write operation failed: " + e);
        }
    }

    @Override
    public void updateCurrentAttributes(@Nonnull final InstanceIdentifier<Streams> id,
                                        @Nonnull final Streams dataBefore,
                                        @Nonnull final Streams dataAfter, @Nonnull final WriteContext writeContext)
            throws WriteFailedException {
    	//invoked by PUT operation,if provided data does exist in Config data
    	crudService.updateData(id, dataBefore, dataAfter);
    	try {
        	runScript(dataAfter.getActiveStreams().getValue());
        }
        catch (IOException e) {
            LOG.error("Update operation failed: " + e);
        }
    }

    @Override
    public void deleteCurrentAttributes(@Nonnull final InstanceIdentifier<Streams> id,
                                        @Nonnull final Streams dataBefore,
                                        @Nonnull final WriteContext writeContext) throws WriteFailedException {
        //perform delete of data,or throw exception
        // Not supported, throw exception
        throw new WriteFailedException.DeleteFailedException(id,
                new UnsupportedOperationException("Delete operation not supported"));
    }

    //Update the number of running streams running a custom script that uses the old vPacketGen REST APIs
    private void runScript(long streams) throws IOException {

    	String script = new String("bash /opt/update_running_streams.sh " + streams);
        Runtime.getRuntime().exec(script);
        String message = "Number of running streams updated to " + streams;
        LOG.info(message);
    }
}
