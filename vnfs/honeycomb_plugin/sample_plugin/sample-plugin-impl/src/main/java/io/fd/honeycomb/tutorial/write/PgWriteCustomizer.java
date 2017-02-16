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

import io.fd.honeycomb.translate.spi.write.ListWriterCustomizer;
import io.fd.honeycomb.translate.v3po.util.NamingContext;
import io.fd.honeycomb.translate.v3po.util.TranslateUtils;
import io.fd.honeycomb.translate.write.WriteContext;
import io.fd.honeycomb.translate.write.WriteFailedException;
import javax.annotation.Nonnull;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.sample.plugin.rev160918.sample.plugin.params.pg.streams.PgStream;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.sample.plugin.rev160918.sample.plugin.params.pg.streams.PgStreamKey;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;
import org.openvpp.jvpp.VppBaseCallException;
import org.openvpp.jvpp.core.dto.PgEnableDisable;
import org.openvpp.jvpp.core.dto.PgEnableDisableReply;
import org.openvpp.jvpp.core.future.FutureJVppCore;

import java.util.Arrays;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;


/**
 * Writer for {@link VxlanTunnel} list node from our YANG model.
 */
public final class PgWriteCustomizer implements ListWriterCustomizer<PgStream, PgStreamKey> {


    private static final Logger LOG = LoggerFactory.getLogger(PgWriteCustomizer.class);

    /**
     * JVpp APIs
     */
    private final FutureJVppCore jvppCore;
    /**
     * Shared vxlan tunnel naming context
     */
    private final NamingContext pgStreamNamingContext;

    public PgWriteCustomizer(final FutureJVppCore jvppCore, final NamingContext pgStreamNamingContext) {
        this.jvppCore = jvppCore;
        this.pgStreamNamingContext = pgStreamNamingContext;
    }

    @Override
    public void writeCurrentAttributes(@Nonnull final InstanceIdentifier<PgStream> id,
                                       @Nonnull final PgStream dataAfter,
                                       @Nonnull final WriteContext writeContext) throws WriteFailedException {
        // Create and set vxlan tunnel add request
        final PgEnableDisable pgEnableDisable = new PgEnableDisable();
        // 1 for add, 0 for delete
        //look into this file: ~/vpp/build-root/build-vpp-native/vpp-api/java/jvpp-core/org/openvpp/jvpp/core/dto/PgEnableDisable.java
        pgEnableDisable.isEnabled = 1;//public byte
        String sName = dataAfter.getId();
        //pgEnableDisable.streamName = sName.getBytes();//public byte[]
        byte[] tempArray = sName.getBytes();
        LOG.info("Going to copy array!!!");
        String tempMsg = "";
        pgEnableDisable.streamName = new byte[tempArray.length+1];
        for(int i = 0; i < tempArray.length; i++){
             tempMsg = "copying: i= "+i+" value: "+tempArray[i];
             LOG.info(tempMsg);
             pgEnableDisable.streamName[i] = tempArray[i];
        }

        //System.arraycopy( sName.getBytes(), 0, pgEnableDisable.streamName, 0, sName.length());
        pgEnableDisable.streamNameLength = sName.length() + 1;//public int
        String logMsg = "######***** Enabling: "+sName+" len: "+sName.length()+" getBytes:" + Arrays.toString(pgEnableDisable.streamName);
        LOG.info(logMsg);
        // dataAfter is the new vxlanTunnel configuration
        //final boolean isIpv6 = dataAfter.getSrc().getIpv6Address() != null;
        //vxlanAddDelTunnel.isIpv6 = TranslateUtils.booleanToByte(isIpv6);
        //vxlanAddDelTunnel.srcAddress = TranslateUtils.ipAddressToArray(isIpv6, dataAfter.getSrc());
        //vxlanAddDelTunnel.dstAddress = TranslateUtils.ipAddressToArray(isIpv6, dataAfter.getDst());
        // There are other input parameters that are not exposed by our YANG model, default values will be used

        try {
            final PgEnableDisableReply replyForWrite = TranslateUtils
                    .getReplyForWrite(jvppCore.pgEnableDisable(pgEnableDisable).toCompletableFuture(), id);

            // VPP returns the index of new vxlan tunnel
            //final int newVxlanTunnelIndex = replyForWrite.swIfIndex;
            // It's important to store it in context so that reader knows to which name a vxlan tunnel is mapped
            pgStreamNamingContext.addName(1, dataAfter.getId(), writeContext.getMappingContext());
        } catch (VppBaseCallException e) {
            throw new WriteFailedException.CreateFailedException(id, dataAfter, e);
        }
    }

    @Override
    public void updateCurrentAttributes(@Nonnull final InstanceIdentifier<PgStream> id,
                                        @Nonnull final PgStream dataBefore,
                                        @Nonnull final PgStream dataAfter, @Nonnull final WriteContext writeContext)
            throws WriteFailedException {
        // Not supported at VPP API level, throw exception
        throw new WriteFailedException.UpdateFailedException(id, dataBefore, dataAfter,
                new UnsupportedOperationException("Vxlan tunnel update is not supported by VPP"));
    }

    @Override
    public void deleteCurrentAttributes(@Nonnull final InstanceIdentifier<PgStream> id,
                                        @Nonnull final PgStream dataBefore,
                                        @Nonnull final WriteContext writeContext) throws WriteFailedException {
        // Create and set vxlan tunnel add request
        //final VxlanAddDelTunnel vxlanAddDelTunnel = new VxlanAddDelTunnel();
        final PgEnableDisable pgEnableDisable = new PgEnableDisable();
        // 1 for add, 0 for delete
        //vxlanAddDelTunnel.isAdd = 0;
        pgEnableDisable.isEnabled = 0;//public byte

        String sName = dataBefore.getId();
        pgEnableDisable.streamName = sName.getBytes();//public byte[]
        pgEnableDisable.streamNameLength = sName.length()+1;//public int

        String logMsg1 = "***** Disabling: "+sName+" len: "+sName.length()+" getBytes:" + Arrays.toString(pgEnableDisable.streamName);
        LOG.info(logMsg1);
        // Vxlan tunnel is identified by its attributes when deleting, not index, so set all attributes
        // dataBefore is the vxlan tunnel that's being deleted
        //final boolean isIpv6 = dataBefore.getSrc().getIpv6Address() != null;
        //vxlanAddDelTunnel.isIpv6 = TranslateUtils.booleanToByte(isIpv6);
        //vxlanAddDelTunnel.srcAddress = TranslateUtils.ipAddressToArray(isIpv6, dataBefore.getSrc());
        //vxlanAddDelTunnel.dstAddress = TranslateUtils.ipAddressToArray(isIpv6, dataBefore.getDst());
        // There are other input parameters that are not exposed by our YANG model, default values will be used

        try {
           // final VxlanAddDelTunnelReply replyForWrite = TranslateUtils
           //         .getReplyForWrite(jvppCore.vxlanAddDelTunnel(vxlanAddDelTunnel).toCompletableFuture(), id);
          final PgEnableDisableReply replyForWrite = TranslateUtils
                    .getReplyForWrite(jvppCore.pgEnableDisable(pgEnableDisable).toCompletableFuture(), id);
           // It's important to remove the mapping from context
           pgStreamNamingContext.removeName(dataBefore.getId(), writeContext.getMappingContext());
        } catch (VppBaseCallException e) {
            throw new WriteFailedException.DeleteFailedException(id, e);
        }
    }
}
