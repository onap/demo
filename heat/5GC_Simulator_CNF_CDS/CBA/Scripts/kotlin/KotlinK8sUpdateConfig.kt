/*
*  Copyright © 2019 TechMahindra
*  Author: Vamshi Namilikonda <vn00480215@techmahindra.com>
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*/

package org.onap.ccsdk.cds.blueprintsprocessor.services.execution.scripts

import org.onap.ccsdk.cds.blueprintsprocessor.core.api.data.ExecutionServiceInput
import org.onap.ccsdk.cds.blueprintsprocessor.services.execution.AbstractScriptComponentFunction
import org.slf4j.LoggerFactory
import com.fasterxml.jackson.databind.node.ObjectNode
import org.onap.ccsdk.cds.controllerblueprints.core.utils.JacksonUtils
import org.onap.ccsdk.cds.blueprintsprocessor.rest.service.BlueprintWebClientService
import org.onap.ccsdk.cds.controllerblueprints.core.BluePrintProcessorException
import java.nio.file.Path
import org.springframework.http.HttpMethod
import org.onap.ccsdk.cds.blueprintsprocessor.rest.BasicAuthRestClientProperties
import org.springframework.http.HttpHeaders
import org.springframework.http.MediaType
import org.apache.http.message.BasicHeader
import java.util.Base64
import java.nio.charset.Charset
import java.io.IOException
import org.apache.http.client.methods.HttpUriRequest
import com.fasterxml.jackson.annotation.JsonProperty
import org.apache.commons.io.IOUtils
import org.apache.http.client.methods.HttpPost
import org.apache.http.client.entity.EntityBuilder
import java.nio.file.Files
import org.onap.ccsdk.cds.blueprintsprocessor.rest.service.RestLoggerService
import org.apache.http.client.ClientProtocolException
import com.fasterxml.jackson.databind.ObjectMapper

import com.google.gson.Gson
import com.google.gson.reflect.TypeToken

import org.onap.ccsdk.cds.blueprintsprocessor.functions.resource.resolution.storedContentFromResolvedArtifactNB
import org.onap.ccsdk.cds.blueprintsprocessor.rest.service.BasicAuthRestClientService

open class KotlinK8sUpdateConfig : AbstractScriptComponentFunction() {

    private val log = LoggerFactory.getLogger(KotlinK8sUpdateConfig::class.java)!!

    override fun getName(): String {
        return "KotlinK8sUpdateConfig"
    }

    override suspend fun processNB(executionRequest: ExecutionServiceInput) {

        println("Exeuting processNB")
        log.info("Executing processNB from Kotlin script: KotlinK8sUpdateConfig ...")

        // read the config  input
        val baseK8sApiUrl = getDynamicProperties("api-access").get("url").asText()
        val k8sApiUsername = getDynamicProperties("api-access").get("username").asText()
        val k8sApiPassword = getDynamicProperties("api-access").get("password").asText()

        val prefix = "baseconfigput"

        val aaiApiUrl = getDynamicProperties("aai-access").get("url").asText()
        val aaiApiUsername = getDynamicProperties("aai-access").get("username").asText()
        val aaiApiPassword = getDynamicProperties("aai-access").get("password").asText()

        log.info("AAI params $aaiApiUrl")

        val resolution_key = getDynamicProperties("resolution-key").asText()

        val payload = storedContentFromResolvedArtifactNB(resolution_key, prefix)

        val payloadObject = JacksonUtils.jsonNode(payload) as ObjectNode

        val serviceInstanceID: String = getResolvedParameter(payloadObject, "service-instance-id")
        val vnfID: String = getResolvedParameter(payloadObject, "vnf-id")

        log.info("Get serviceInstanceID $serviceInstanceID")
        log.info("Get vnfID $vnfID")

        val vnfUrl = aaiApiUrl + "/aai/v19/network/generic-vnfs/generic-vnf/" + vnfID + "/vf-modules";

        val mapOfHeaders = hashMapOf<String, String>()
        mapOfHeaders.put("Accept", "application/json")
        mapOfHeaders.put("Content-Type", "application/json")
        mapOfHeaders.put("x-FromAppId", "SO")
        mapOfHeaders.put("X-TransactionId", "get_aai_subscr")
        val basicAuthRestClientProperties: BasicAuthRestClientProperties = BasicAuthRestClientProperties()
        basicAuthRestClientProperties.username = aaiApiUsername
        basicAuthRestClientProperties.password = aaiApiPassword
        basicAuthRestClientProperties.url = vnfUrl
        basicAuthRestClientProperties.additionalHeaders =mapOfHeaders
        val basicAuthRestClientService: BasicAuthRestClientService= BasicAuthRestClientService(basicAuthRestClientProperties)
        try {
            val resultOfGet: BlueprintWebClientService.WebClientResponse<String> = basicAuthRestClientService.exchangeResource(HttpMethod.GET.name, "", "")

            val aaiBody = resultOfGet.body
            val aaiPayloadObject = JacksonUtils.jsonNode(aaiBody) as ObjectNode

            for (item in aaiPayloadObject.get("vf-module")) {

                log.info("item payload Deatils : $item")

                val isItBaseVfModule = item.get("is-base-vf-module").asText()

                if(isItBaseVfModule.toBoolean())
                    continue

                val vfModuleID: String = item.get("vf-module-id").asText()

                log.info("AAI Vf-module ID is : $vfModuleID")

                val vfModuleModelInvariantUuid: String = item.get("model-invariant-id").asText()

                log.info("AAI Vf-module Invariant ID is : $vfModuleModelInvariantUuid")

                val vfModuleModelUuid: String = item.get("model-version-id").asText()

                log.info("AAI Vf-module UUID is : $vfModuleModelUuid")

                val vfModuleInstance: String = item.get("heat-stack-id").asText()

                log.info("AAI Vf-module Heat Stack ID : $vfModuleInstance")

                val profileName: String = "profile-"+ vfModuleID
                val templateName: String = "template_" + vfModuleID

                val randomString = getRandomString(6)
                val configName: String = "config_"+ randomString

                var supportedNssai: String = getResolvedParameter(payloadObject, "supportedNssai")

                log.info("supportedNssai from SO -> "+ supportedNssai)
                log.info("configName ->"+ configName)
                log.info("profileName ->"+ profileName)
                log.info("templateName ->"+ templateName)


                executeK8sAPI(supportedNssai, k8sApiUsername, k8sApiPassword, baseK8sApiUrl, vfModuleModelInvariantUuid, vfModuleModelUuid, templateName, configName, profileName)

            }
        }
        catch (e: Exception) {
            log.info("Caught exception trying to get the vnf Details!!")
            throw BluePrintProcessorException("${e.message}")
        }
    }

    fun getRandomString(length: Int) : String {
        val charset = "0123456789"
        return (1..length)
                .map { charset.random() }
                .joinToString("")
    }

    fun executeK8sAPI(supportedNssai: String, k8sApiUsername:String, k8sApiPassword:String, baseK8sApiUrl:String, vfModuleModelInvariantUuid:String, vfModuleModelUuid: String, templateName: String, configName:String, profileName:String){

        println("Executing executeK8sAPI ...")

        // read and convert supportedNssai parameters from string to json
        val sNssaiAsJsonObj = parseSupportedNssai(supportedNssai)

        // contruct config api
        val api = K8sConfigApi(k8sApiUsername, k8sApiPassword, baseK8sApiUrl, vfModuleModelInvariantUuid, vfModuleModelUuid)


        // invoke config api
        var config = K8sConfigPayloadJson()
        config.templateName = templateName
        config.configName = configName
        config.values = Config()
        config.values.supportedNssai = SupportedNssai()
        config.values.supportedNssai.snssaiInitial = SnssaiInitial()
        config.values.supportedNssai.snssaiInitial.snssaiSecond = SnssaiSecond()
        config.values.supportedNssai.snssaiInitial.snssaiSecond.snssaiFinalArray = Array<SnssaiFinal>(sNssaiAsJsonObj.size){i-> SnssaiFinal()}

        val dest = buildSNssaiArray(config.values.supportedNssai.snssaiInitial.snssaiSecond.snssaiFinalArray, sNssaiAsJsonObj)
        api.createOrUpdateConfig(config, profileName)

        log.info("K8s Configurations create or update Completed")

    }

    fun buildSNssaiArray(payloadSnssai: Array<SnssaiFinal>, requestSnssai: Array<SnssaiFinal>): Array<SnssaiFinal>{

        System.arraycopy(requestSnssai, 0, payloadSnssai, 0, requestSnssai.size)

        return payloadSnssai

    }

    fun parseSupportedNssai(supportedNssai: String): Array<SnssaiFinal>{

        log.info("parsing supportedNssai string..")

        log.info("sNssai value from input..  $supportedNssai")

        val trimmed_supportedNssai = supportedNssai.replace("\\s".toRegex(), "").replace("\\r\\n","").replace("\\","")

        val gson = Gson()

        val startInd = trimmed_supportedNssai.indexOf('[')
        val endInd = trimmed_supportedNssai.indexOf(']')

        val subStr = trimmed_supportedNssai.substring(startInd, endInd+1)

        val snType = object : TypeToken<Array<SnssaiFinal>>() {}.type

        var snList: Array<SnssaiFinal> = gson.fromJson(subStr, snType)

        log.info("parsing is done.")

        return snList

    }

    fun getResolvedParameter(payload: ObjectNode, keyName: String): String {
        for (node in payload.get("resource-accumulator-resolved-data").elements()) {
            if (node.get("param-name").asText().equals(keyName)) {
                return node.get("param-value").asText()
            }
        }
        return ""
    }

    override suspend fun recoverNB(runtimeException: RuntimeException, executionRequest: ExecutionServiceInput) {
        log.info("Executing Recovery")
    }

    inner class K8sConfigApi(
            val username: String,
            val password: String,
            val baseUrl: String,
            val definition: String,
            val definitionVersion: String
    ) {
        private val service: UploadFileConfigClientService // BasicAuthRestClientService

        init {
            var mapOfHeaders = hashMapOf<String, String>()
            mapOfHeaders.put("Accept", "application/json")
            mapOfHeaders.put("Content-Type", "application/json")
            mapOfHeaders.put("cache-control", " no-cache")
            mapOfHeaders.put("Accept", "application/json")
            var basicAuthRestClientProperties: BasicAuthRestClientProperties = BasicAuthRestClientProperties()
            basicAuthRestClientProperties.username = username
            basicAuthRestClientProperties.password = password
            basicAuthRestClientProperties.url = "$baseUrl/v1/rb/definition/$definition/$definitionVersion"
            basicAuthRestClientProperties.additionalHeaders = mapOfHeaders

            this.service = UploadFileConfigClientService(basicAuthRestClientProperties)
        }

        fun createOrUpdateConfig(configJson: K8sConfigPayloadJson, profileName: String) {
            val objectMapper = ObjectMapper()

            for(snssai in configJson.values.supportedNssai.snssaiInitial.snssaiSecond.snssaiFinalArray){
                println("snssai->" +snssai.snssai)
                println("status->"+snssai.status)

            }

            val configJsonString: String = objectMapper.writeValueAsString(configJson)

            log.info("payload generated -> "+ configJsonString)

            val startInd = configJsonString.indexOf('[')
            val endInd = configJsonString.indexOf(']')

            val snssaiArray: String = configJsonString.substring(startInd, endInd+1).replace("\"","\\\"").replace("[","\"[").replace("]","]\"")

            val finalPayload: String = configJsonString.replaceRange(startInd..endInd, snssaiArray)

            log.info("payload restructured -> "+ finalPayload)

            try {
                val result: BlueprintWebClientService.WebClientResponse<String> = service.exchangeResource(HttpMethod.POST.name,
                        "/profile/${profileName}/config", finalPayload)
                if (result.status < 200 || result.status >= 300) {
                    throw Exception(result.body)
                }
            } catch (e: Exception) {
                log.info("Caught exception trying to create or update configuration ")
                throw BluePrintProcessorException("${e.message}")
            }
        }

    }
}

class UploadFileConfigClientService(
        private val restClientProperties:
        BasicAuthRestClientProperties
) : BlueprintWebClientService {

    override fun defaultHeaders(): Map<String, String> {

        val encodedCredentials = setBasicAuth(
                restClientProperties.username,
                restClientProperties.password
        )
        return mapOf(
                HttpHeaders.CONTENT_TYPE to MediaType.APPLICATION_JSON_VALUE,
                HttpHeaders.ACCEPT to MediaType.APPLICATION_JSON_VALUE,
                HttpHeaders.AUTHORIZATION to "Basic $encodedCredentials"
        )
    }

    override fun host(uri: String): String {
        return restClientProperties.url + uri
    }

    override fun convertToBasicHeaders(headers: Map<String, String>):
            Array<BasicHeader> {
        val customHeaders: MutableMap<String, String> = headers.toMutableMap()
        // inject additionalHeaders
        customHeaders.putAll(verifyAdditionalHeaders(restClientProperties))

        if (!headers.containsKey(HttpHeaders.AUTHORIZATION)) {
            val encodedCredentials = setBasicAuth(
                    restClientProperties.username,
                    restClientProperties.password
            )
            customHeaders[HttpHeaders.AUTHORIZATION] =
                    "Basic $encodedCredentials"
        }
        return super.convertToBasicHeaders(customHeaders)
    }

    private fun setBasicAuth(username: String, password: String): String {
        val credentialsString = "$username:$password"
        return Base64.getEncoder().encodeToString(
                credentialsString.toByteArray(Charset.defaultCharset())
        )
    }

    @Throws(IOException::class, ClientProtocolException::class)
    private fun performHttpCall(httpUriRequest: HttpUriRequest): BlueprintWebClientService.WebClientResponse<String> {
        val httpResponse = httpClient().execute(httpUriRequest)
        val statusCode = httpResponse.statusLine.statusCode
        httpResponse.entity.content.use {
            val body = IOUtils.toString(it, Charset.defaultCharset())
            return BlueprintWebClientService.WebClientResponse(statusCode, body)
        }
    }

    fun uploadBinaryFile(path: String, filePath: Path): BlueprintWebClientService.WebClientResponse<String> {
        val convertedHeaders: Array<BasicHeader> = convertToBasicHeaders(defaultHeaders())
        val httpPost = HttpPost(host(path))
        val entity = EntityBuilder.create().setBinary(Files.readAllBytes(filePath)).build()
        httpPost.setEntity(entity)
        RestLoggerService.httpInvoking(convertedHeaders)
        httpPost.setHeaders(convertedHeaders)
        return performHttpCall(httpPost)
    }
}


class K8sConfigPayloadJson {
    @get:JsonProperty("template-name")
    var templateName: String? = null
    @get:JsonProperty("config-name")
    var configName: String? = null
    @get:JsonProperty("values")
    lateinit var values: Config

    override fun toString(): String {
        return "$templateName:$configName:$values"
    }

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false
        return true
    }

    override fun hashCode(): Int {
        return javaClass.hashCode()
    }
}

class Config{
    @get:JsonProperty("config")
    lateinit var supportedNssai: SupportedNssai
}

class SupportedNssai{
    @get:JsonProperty("supportedNssai")
    lateinit var snssaiInitial: SnssaiInitial
}

class SnssaiInitial{

    @get:JsonProperty("sNssai")
    lateinit var snssaiSecond: SnssaiSecond
}

class SnssaiSecond{

    @get:JsonProperty("snssai")
    lateinit var snssaiFinalArray: Array<SnssaiFinal>
}


class SnssaiFinal{
    @get:JsonProperty("snssai")
    var snssai: String? = null

    @get:JsonProperty("status")
    var status: String? = null
}


fun main(args: Array<String>) {

    val supportedNssai = """

        {\r\n                                     \"sNssai\":[\r\n                                        {\r\n                                           \"snssai\":\"001-100001\",\r\n                                           \"status\":\"created-modified\"\r\n                                        },\r\n                                        {\r\n                                           \"snssai\":\"002-100001\",\r\n                                           \"status\":\"activated\"\r\n                                        },\r\n                                        {\r\n                                           \"snssai\":\"003-100001\",\r\n                                           \"status\":\"de-activated\"\r\n                                        }\r\n                                     ]\r\n                                  }\r\n

"""

    val kotlin = KotlinK8sUpdateConfig()

    /* supportedNssai
     k8sApiUsername
     k8sApiPassword
     baseK8sApiUrl
     vfModuleModelInvariantUuid
     vfModuleModelUuid
     templateName
     configName
     profileName*/

    kotlin.executeK8sAPI(supportedNssai, "admin", "admin", "http://0.0.0.0:9015", "rb_test", "1", "template_test", "config_test", "profile_test")

}
