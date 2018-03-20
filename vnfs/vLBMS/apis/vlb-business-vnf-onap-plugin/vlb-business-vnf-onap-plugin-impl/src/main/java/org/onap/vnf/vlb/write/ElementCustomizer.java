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

package org.onap.vnf.vlb.write;

import org.onap.vnf.vlb.CrudService;
import io.fd.honeycomb.translate.spi.write.ListWriterCustomizer;
import io.fd.honeycomb.translate.write.WriteContext;
import io.fd.honeycomb.translate.write.WriteFailedException;
import javax.annotation.Nonnull;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.vlb.business.vnf.onap.plugin.rev160918.vlb.business.vnf.onap.plugin.params.vdns.instances.VdnsInstance;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.vlb.business.vnf.onap.plugin.rev160918.vlb.business.vnf.onap.plugin.params.vdns.instances.VdnsInstanceKey;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;

/**
 * Writer for {@link Element} list node from our YANG model.
 */
public final class ElementCustomizer implements ListWriterCustomizer<VdnsInstance, VdnsInstanceKey> {

    private final CrudService<VdnsInstance> crudService;
    private DnsInstanceManager dnsInstanceManager;

    public ElementCustomizer(@Nonnull final CrudService<VdnsInstance> crudService) {
        this.crudService = crudService;
        dnsInstanceManager = DnsInstanceManager.getInstance();
    }

    @Override
    public void writeCurrentAttributes(@Nonnull final InstanceIdentifier<VdnsInstance> id, @Nonnull final VdnsInstance dataAfter,
                                       @Nonnull final WriteContext writeContext) throws WriteFailedException {
        //perform write of data,or throw exception
        //invoked by PUT operation,if provided data doesn't exist in Config data
        crudService.writeData(id, dataAfter);
        dnsInstanceManager.addDnsInstance(dataAfter.getIpAddr(), dataAfter);
    }

    @Override
    public void updateCurrentAttributes(@Nonnull final InstanceIdentifier<VdnsInstance> id,
                                        @Nonnull final VdnsInstance dataBefore,
                                        @Nonnull final VdnsInstance dataAfter, @Nonnull final WriteContext writeContext)
            throws WriteFailedException {
        //perform update of data,or throw exception
        //invoked by PUT operation,if provided data does exist in Config data
        crudService.updateData(id, dataBefore, dataAfter);
        dnsInstanceManager.updateDnsInstance(dataAfter.getIpAddr(), dataAfter);
    }

    @Override
    public void deleteCurrentAttributes(@Nonnull final InstanceIdentifier<VdnsInstance> id,
                                        @Nonnull final VdnsInstance dataBefore,
                                        @Nonnull final WriteContext writeContext) throws WriteFailedException {
        //perform delete of data,or throw exception
        //invoked by DELETE operation
        crudService.deleteData(id, dataBefore);
        dnsInstanceManager.deleteDnsInstance(dataBefore.getIpAddr());
    }
}
