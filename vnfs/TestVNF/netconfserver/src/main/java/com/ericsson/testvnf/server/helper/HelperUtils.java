/*
 * ============LICENSE_START=======================================================
 *  Copyright (C) 2016-2018 Ericsson. All rights reserved.
 * ================================================================================
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * ============LICENSE_END=========================================================
 */

package com.ericsson.testvnf.server.helper;

import java.io.File;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Map;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import com.ericsson.testvnf.server.models.NetconfMessage;

import groovy.lang.Binding;
import groovy.util.GroovyScriptEngine;

/*
 * The utils class
 */
public class HelperUtils{
	
	private static final Log log = LogFactory.getLog(HelperUtils.class);
	
	private HelperUtils() {
		super();
	}

	// executes the groovy file specified in the path
	public static String executeGroovy(String groovyFilePath, NetconfMessage data, Map<String, Boolean> connectionResetMap) {

		Object result = "";
		try {
			log.info("groovy path------" + groovyFilePath);
			File file = new File(groovyFilePath);
			GroovyScriptEngine engine = new GroovyScriptEngine(file.getParent());
			Binding binding = new Binding();
			binding.setVariable("RpcData", data);
			binding.setVariable("connectionResetMap", connectionResetMap);
			log.info("binding " + binding + "  " + file.getParent() + " " + file.getName());
			result = engine.run(file.getName(), binding);
		} catch (Exception e) {
			log.error("Exception while trying to execute groovy file", e);
		}
		return result.toString();
	}
	
	// send bytes to output stream
	public static void sendAsBytesToOutput(byte[] buffer, OutputStream out){
		try {
			log.info("Sending message as bytes..\n");
			int len = buffer.length;
			out.write(buffer, 0, len);
			String tail = "]]>]]>";
			out.write(tail.getBytes(), 0, tail.getBytes().length);
			out.flush();
		}catch(Exception e) {
			log.info("Error while sending response message as bytes: "+e);
		}
	}
	
	// the method is used to read the contents of the file specified
	public static String readFile(String filename) {
		String fileAsString= "";
		try {
			fileAsString = new String(Files.readAllBytes(Paths.get(filename)));
		} catch (IOException e) {
			log.error("Error reading file: "+ filename);
		}
		return fileAsString;
	}
}