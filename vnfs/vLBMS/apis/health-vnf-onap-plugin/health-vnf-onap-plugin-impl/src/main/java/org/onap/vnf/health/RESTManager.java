/*-
 * ============LICENSE_START=======================================================
 *
 * ================================================================================
 * Copyright (C) 2017 AT&T Intellectual Property. All rights reserved.
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
 * ============LICENSE_END=========================================================
 */

package org.onap.vnf.health;

import java.io.IOException;
import java.util.Map;
import java.util.Map.Entry;
import org.apache.http.HttpResponse;
import org.apache.http.auth.AuthScope;
import org.apache.http.auth.UsernamePasswordCredentials;
import org.apache.http.client.CredentialsProvider;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.conn.ssl.NoopHostnameVerifier;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.BasicCredentialsProvider;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClientBuilder;
import org.apache.http.util.EntityUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class RESTManager {

    private static final Logger logger = LoggerFactory.getLogger(RESTManager.class);

    public class Pair<A, B> {
        public final A a;
        public final B b;

        public Pair(A a, B b) {
            this.a = a;
            this.b = b;
        }
    }

    public Pair<Integer, String> post(String url, String username, String password,
            Map<String, String> headers, String contentType, String body) {
        CredentialsProvider credentials = new BasicCredentialsProvider();
        credentials.setCredentials(AuthScope.ANY,
                new UsernamePasswordCredentials(username, password));

        logger.debug("HTTP REQUEST: {} -> {} {} -> {}", url, username,
                ((password != null) ? password.length() : "-"), contentType);
        if (headers != null) {
            logger.debug("Headers: ");
            headers.forEach((name, value) -> logger.debug("{} -> {}", name, value));
        }
        logger.debug(body);

        try (CloseableHttpClient client =
                HttpClientBuilder
                        .create()
                        .setSSLHostnameVerifier(NoopHostnameVerifier.INSTANCE)
                        .setDefaultCredentialsProvider(credentials)
                        .build()) {

            HttpPost post = new HttpPost(url);
            if (headers != null) {
                for (Entry<String, String> entry : headers.entrySet()) {
                    post.addHeader(entry.getKey(), headers.get(entry.getKey()));
                }
            }
            post.addHeader("Content-Type", contentType);

            StringEntity input = new StringEntity(body);
            input.setContentType(contentType);
            post.setEntity(input);

            HttpResponse response = client.execute(post);
            if (response != null) {
                String returnBody = EntityUtils.toString(response.getEntity(), "UTF-8");
                logger.debug("HTTP POST Response Status Code: {}",
                        response.getStatusLine().getStatusCode());
                logger.debug("HTTP POST Response Body:");
                logger.debug(returnBody);

                return new Pair<>(response.getStatusLine().getStatusCode(),
                        returnBody);
            }
            else {
                logger.error("Response from {} is null", url);
                return null;
            }
        }
        catch (Exception e) {
            logger.error("Failed to POST to {}", url, e);
            return null;
        }
    }

    public Pair<Integer, String> get(String url, String username, String password,
            Map<String, String> headers) {

        CredentialsProvider credentials = new BasicCredentialsProvider();
        credentials.setCredentials(AuthScope.ANY,
                new UsernamePasswordCredentials(username, password));

        try (CloseableHttpClient client =
                HttpClientBuilder
                        .create()
                        .setSSLHostnameVerifier(NoopHostnameVerifier.INSTANCE)
                        .setDefaultCredentialsProvider(credentials)
                        .build()) {

            HttpGet get = new HttpGet(url);
            if (headers != null) {
                for (Entry<String, String> entry : headers.entrySet()) {
                    get.addHeader(entry.getKey(), headers.get(entry.getKey()));
                }
            }

            HttpResponse response = client.execute(get);

            String returnBody = EntityUtils.toString(response.getEntity(), "UTF-8");

            logger.debug("HTTP GET Response Status Code: {}",
                    response.getStatusLine().getStatusCode());
            logger.debug("HTTP GET Response Body:");
            logger.debug(returnBody);

            return new Pair<>(response.getStatusLine().getStatusCode(), returnBody);
        }
        catch (IOException e) {
            logger.error("Failed to GET to {}", url, e);
            return null;
        }
    }
}
