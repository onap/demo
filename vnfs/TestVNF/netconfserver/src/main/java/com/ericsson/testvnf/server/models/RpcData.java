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

package com.ericsson.testvnf.server.models;
/*
 * RPC operation message class
 */
public class RpcData extends NetconfMessage implements java.io.Serializable {

	private static final long serialVersionUID = -8318907964396287877L;

	private String operation;
	private String targetName;
	private String operationTagContent;
	private String configurationDatastore = "NA";
	private SchemaDetails schemaDetails; // Parameters for get-schema
	private String timeDelayForSendingEvents;// time delay in milliseconds
	
	public String getConfigurationDatastore() {
		return configurationDatastore;
	}

	public void setConfigurationDatastore(String configurationDatastore) {
		this.configurationDatastore = configurationDatastore;
	}

	public String getOperationTagContent() {
		return operationTagContent;
	}

	public void setOperationTagContent(String operationTagContent) {
		this.operationTagContent = operationTagContent;
	}

	public String getTimeDelayForSendingEvents() {
		return timeDelayForSendingEvents;
	}

	public void setTimeDelayForSendingEvents(String timeDelayForSendingEvents) {
		this.timeDelayForSendingEvents = timeDelayForSendingEvents;
	}

	public String getTargetName() {
		return targetName;
	}

	public void setTargetName(String targetName) {
		this.targetName = targetName;
	}

	public SchemaDetails getSchemaDetails() {
		return schemaDetails;
	}

	public void setSchemaDetails(SchemaDetails schemaDetails) {
		this.schemaDetails = schemaDetails;
	}

	public String getOperation() {
		return operation;
	}

	public void setOperation(String operation) {
		this.operation = operation;
	}

}
