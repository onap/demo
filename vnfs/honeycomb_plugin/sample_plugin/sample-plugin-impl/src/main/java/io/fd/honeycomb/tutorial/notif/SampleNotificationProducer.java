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

package io.fd.honeycomb.tutorial.notif;

import io.fd.honeycomb.notification.ManagedNotificationProducer;
import io.fd.honeycomb.notification.NotificationCollector;
import java.util.Collection;
import java.util.Collections;
import javax.annotation.Nonnull;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.sample.plugin.rev160918.SampleNotification;
import org.opendaylight.yang.gen.v1.urn.opendaylight.params.xml.ns.yang.sample.plugin.rev160918.SampleNotificationBuilder;
import org.opendaylight.yangtools.yang.binding.Notification;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Notification producer for sample plugin
 */
public class SampleNotificationProducer implements ManagedNotificationProducer {

    private static final Logger LOG = LoggerFactory.getLogger(SampleNotificationProducer.class);

    private Thread thread;

    @Override
    public void start(@Nonnull final NotificationCollector collector) {
        LOG.info("Starting notification stream for interfaces");

        // Simulating notification producer
        thread = new Thread(() -> {
            while(true) {
                if (Thread.currentThread().isInterrupted()) {
                    return;
                }

                try {
                    Thread.sleep(2000);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    break;
                }

                final SampleNotification notification = new SampleNotificationBuilder()
                        .setContent("Hello world " + System.currentTimeMillis())
                        .build();
                LOG.info("Emitting notification: {}", notification);
                collector.onNotification(notification);
            }
        }, "NotificationProducer");
        thread.setDaemon(true);
        thread.start();
    }

    @Override
    public void stop() {
        if(thread != null) {
            thread.interrupt();
        }
    }

    @Nonnull
    @Override
    public Collection<Class<? extends Notification>> getNotificationTypes() {
        // Producing only this single type of notification
        return Collections.singleton(SampleNotification.class);
    }

    @Override
    public void close() throws Exception {
        stop();
    }
}
