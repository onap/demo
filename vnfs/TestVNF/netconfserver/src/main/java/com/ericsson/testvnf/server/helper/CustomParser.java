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

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;
import org.xml.sax.ext.DefaultHandler2;

import com.ericsson.testvnf.server.models.Hello;
import com.ericsson.testvnf.server.models.RpcData;
import com.ericsson.testvnf.server.models.SchemaDetails;
import com.ericsson.testvnf.server.requestqueue.RequestQueueHandler;

/*
 * Parses the xml requests and populates netconf related data to do operations
 */
public class CustomParser extends DefaultHandler2 {
	
	private Log log = LogFactory.getLog(CustomParser.class);
	private RequestQueueHandler requestQueueHandler;

	private Hello hello;
	private RpcData rpcData;
	private boolean nextIsOperationTag = false;
	private boolean insideOperationTag = false;
	private String operationTagValue = "";
	private StringBuilder operationTagContent = new StringBuilder();
	private boolean insideConfigurationDatastore = false;
	private boolean insideSchemaDetailsTag = false;
	private StringBuilder individualSchemaDetailsTagContent = new StringBuilder();
	private SchemaDetails schemaDetails;
	private boolean insideTargetNameTag = false;
	private StringBuilder targetNameContent = new StringBuilder();

	@Override
	public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
		super.startElement(uri, localName, qName, attributes);
		String messageId;
		if (nextIsOperationTag) {
			insideOperationTag = true;
			rpcData.setOperation(localName);
			operationTagValue = localName;
			nextIsOperationTag = false;
			if (localName.equalsIgnoreCase("get-schema")) {
				insideSchemaDetailsTag = true;
				schemaDetails = new SchemaDetails();
			}
		}
		
		if (insideOperationTag) {
			populateStartElementData(localName, attributes);
		}
		
		if (localName.equalsIgnoreCase("hello")) {
			hello = new Hello();
		} else if (localName.equalsIgnoreCase("rpc")) {
			rpcData = new RpcData();

			messageId = attributes.getValue("message-id");
			if (messageId == null)
				throw new SAXException("Received <rpc> message without a message ID");

			rpcData.setMessageId(messageId);
			nextIsOperationTag = true;
		}

	}
	
	@Override
	public void characters(char[] ch, int start, int length) throws SAXException {
		super.characters(ch, start, length);
		
		if (insideOperationTag) {
			operationTagContent.append(ch, start, length);
			if (insideSchemaDetailsTag) {
				individualSchemaDetailsTagContent.append(ch, start, length);
			}else if (insideTargetNameTag) {
				targetNameContent.append(ch, start, length);
			}
		}
	}
	@Override
	public void endElement(String uri, String localName, String qName) throws SAXException {
		super.endElement(uri, localName, qName);
		
		if (insideOperationTag) {
			populateEndElementData(localName);
		}
		if (localName.equalsIgnoreCase("hello")) {
			requestQueueHandler.addToQueue(hello);
			hello = null;
			/* Query tags and operations */
		} else if (localName.equalsIgnoreCase("rpc")) {
			requestQueueHandler.addToQueue(rpcData);
			rpcData = null;
		}else if(operationTagValue.equals(localName)) {
			rpcData.setOperationTagContent(operationTagContent.toString());
			insideOperationTag = false;
			operationTagContent = new StringBuilder();
			if (localName.equalsIgnoreCase("get-schema")) {
				insideSchemaDetailsTag = false;
				rpcData.setSchemaDetails(schemaDetails);
				schemaDetails = new SchemaDetails();
			}
		}
	}

	private void populateStartElementData(String localName, Attributes attributes) {
		if (insideConfigurationDatastore) {
			rpcData.setConfigurationDatastore(localName);
		}
		if (localName.equalsIgnoreCase("target") || localName.equalsIgnoreCase("source")) {
			insideConfigurationDatastore = true;
		} else if(localName.equalsIgnoreCase("target-name")) {
			insideTargetNameTag = true;
		}
		operationTagContent.append("<" + localName);
		for (int i = 0; i < attributes.getLength(); i++) {
			operationTagContent.append(" " + attributes.getQName(i) + "=\"" + attributes.getValue(i) + "\"");
		}
		operationTagContent.append(">");
	}
	
	private void populateEndElementData(String localName) {
		if (localName.equalsIgnoreCase("target") || localName.equalsIgnoreCase("source")) {
			insideConfigurationDatastore = false;
		} else if(localName.equalsIgnoreCase("target-name")) {
			insideTargetNameTag = false;
			rpcData.setTargetName(targetNameContent.toString());
			targetNameContent = new StringBuilder();
		} else if (insideSchemaDetailsTag) {
			if(localName.equalsIgnoreCase("identifier")) {
				schemaDetails.setIdentifier(individualSchemaDetailsTagContent.toString().trim());
				individualSchemaDetailsTagContent = new StringBuilder();
			} else if(localName.equalsIgnoreCase("version")) {
				schemaDetails.setVersion(individualSchemaDetailsTagContent.toString().trim());
				individualSchemaDetailsTagContent = new StringBuilder();
			}		
		}
		operationTagContent.append("</" + localName + ">");
	}
	
	public void setRequestQueueHandler(RequestQueueHandler queue) {
		this.requestQueueHandler = queue;
	}
	
	@Override
	public void error(SAXParseException e) throws SAXException {
		log.warn(e.getMessage());
	}

	@Override
	public void fatalError(SAXParseException e) throws SAXException {
		log.warn(e.getMessage());
	}
}