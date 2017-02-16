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

package io.fd.honeycomb.vpp.integration.distro;

import com.google.common.collect.Lists;
import com.google.inject.Module;
import io.fd.honeycomb.lisp.LispModule;
import io.fd.honeycomb.translate.v3po.V3poModule;
import io.fd.honeycomb.vpp.distro.VppCommonModule;
import io.fd.honeycomb.vppnsh.impl.VppNshModule;
import java.util.List;

public class Main {

    public static void main(String[] args) {
        final List<Module> sampleModules = Lists.newArrayList(io.fd.honeycomb.infra.distro.Main.BASE_MODULES);

        // All the plugins should be listed here
        sampleModules.add(new VppCommonModule());
        sampleModules.add(new V3poModule());
        sampleModules.add(new LispModule());
        sampleModules.add(new VppNshModule());
        sampleModules.add(new io.fd.honeycomb.tutorial.Module());

        io.fd.honeycomb.infra.distro.Main.init(sampleModules);
    }
}
