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

package io.fd.honeycomb.tutorial;

import net.jmob.guice.conf.core.BindConfig;
import net.jmob.guice.conf.core.InjectConfig;
import net.jmob.guice.conf.core.Syntax;

/**
 * Class containing static configuration for sample-plugin module,<br>
 * either loaded from property file sample-plugin.json from classpath.
 * <p/>
 * Further documentation for the configuration injection can be found at:
 * https://github.com/yyvess/gconf
 */
@BindConfig(value = "sample-plugin", syntax = Syntax.JSON)
public final class ModuleConfiguration {

    // TODO change the sample property to real plugin configuration
    // If there is no such configuration, remove this, sample-plugin.json resource and its wiring from Module class

    /**
     * Sample property that's injected from external json configuration file.
     */
    @InjectConfig("sample-prop")
    public String sampleProp;

    /**
     * Constant name used to identify sample-plugin plugin specific components during dependency injection.
     */
    public static final String ELEMENT_SERVICE_NAME = "element-service";
}
