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

package org.onap.vnf.vfw;

import io.fd.honeycomb.translate.read.ReadFailedException;
import io.fd.honeycomb.translate.write.WriteFailedException;
import java.util.List;
import javax.annotation.Nonnull;
import org.opendaylight.yangtools.yang.binding.DataObject;
import org.opendaylight.yangtools.yang.binding.InstanceIdentifier;

/**
 * Example of an aggregated access interface.
 * <p/>
 * Shared by all the customizers hiding the ugly details of our data management.
 *
 * TODO update javadoc
 */
public interface CrudService<T extends DataObject> {

    /**
     * Perform write of provided data.
     */
    void writeData(@Nonnull final InstanceIdentifier<T> identifier, @Nonnull final T data)
            throws WriteFailedException;


    /**
     * Perform delete of existing data.
     */
    void deleteData(@Nonnull final InstanceIdentifier<T> identifier, @Nonnull final T data)
            throws WriteFailedException;

    /**
     * Perform update of existing data.
     */
    void updateData(@Nonnull final InstanceIdentifier<T> identifier, @Nonnull final T dataOld,
                           @Nonnull final T dataNew)
            throws WriteFailedException;

    /**
     * Read data identified by provided identifier.
     */
    T readSpecific(@Nonnull final InstanceIdentifier<T> identifier) throws ReadFailedException;

    /**
     * Read all nodes of type {@link T}.
     */
    List<T> readAll() throws ReadFailedException;
}
