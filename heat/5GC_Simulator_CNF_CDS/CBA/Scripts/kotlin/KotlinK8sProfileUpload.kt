/*
 * Copyright © 2019 Orange
 * Author: Malinconico Aniello Paolo <aniellopaolo.malinconico@guest.telecomitalia.it>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.onap.ccsdk.cds.blueprintsprocessor.services.execution.scripts

import com.fasterxml.jackson.annotation.JsonProperty
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.databind.node.ObjectNode
import java.io.File
import java.io.IOException
import java.nio.charset.Charset
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths
import java.util.ArrayList
import java.util.Base64
import java.util.LinkedHashMap
import org.apache.commons.io.IOUtils
import org.apache.http.client.ClientProtocolException
import org.apache.http.client.entity.EntityBuilder
import org.apache.http.client.methods.HttpPost
import org.apache.http.client.methods.HttpUriRequest
import org.apache.http.message.BasicHeader
import org.onap.ccsdk.cds.blueprintsprocessor.core.api.data.ExecutionServiceInput
import org.onap.ccsdk.cds.blueprintsprocessor.rest.BasicAuthRestClientProperties
import org.onap.ccsdk.cds.blueprintsprocessor.rest.service.BlueprintWebClientService
import org.onap.ccsdk.cds.blueprintsprocessor.rest.service.RestLoggerService
import org.onap.ccsdk.cds.blueprintsprocessor.services.execution.AbstractScriptComponentFunction
import org.onap.ccsdk.cds.controllerblueprints.core.BluePrintProcessorException
import org.onap.ccsdk.cds.controllerblueprints.core.utils.ArchiveType
import org.onap.ccsdk.cds.controllerblueprints.core.utils.BluePrintArchiveUtils
import org.onap.ccsdk.cds.controllerblueprints.core.utils.JacksonUtils
import org.slf4j.LoggerFactory
import org.springframework.http.HttpHeaders
import org.springframework.http.HttpMethod
import org.springframework.http.MediaType
import org.yaml.snakeyaml.Yaml

open class K8sProfileUpload : AbstractScriptComponentFunction() {

    private val log = LoggerFactory.getLogger(K8sProfileUpload::class.java)!!

    override fun getName(): String {
        return "K8sProfileUpload"
    }

    override suspend fun processNB(executionRequest: ExecutionServiceInput) {
        log.info("executing K8s Profile Upload script")

        val baseK8sApiUrl = getDynamicProperties("api-access").get("url").asText()
        val k8sApiUsername = getDynamicProperties("api-access").get("username").asText()
        val k8sApiPassword = getDynamicProperties("api-access").get("password").asText()
        val prefixList: ArrayList<String> = getTemplatePrefixList(executionRequest)
        for (prefix in prefixList) {
            if (prefix.toLowerCase().equals("vnf")) {
                log.info("For vnf-level resource-assignment, profile is not performed. Creating override_values")
                val assignmentParams = getDynamicProperties("assignment-params")
                val payloadObject = JacksonUtils.jsonNode(assignmentParams.get(prefix).asText()) as ObjectNode
                createOverrideVaues(payloadObject)
                continue
            }
            val assignmentParams = getDynamicProperties("assignment-params")
            val payloadObject = JacksonUtils.jsonNode(assignmentParams.get(prefix).asText()) as ObjectNode

            log.info("Uploading K8S profile for template prefix $prefix")

            val vfModuleModelInvariantUuid: String = getResolvedParameter(payloadObject, "vf-module-model-invariant-uuid")
            val vfModuleModelUuid: String = getResolvedParameter(payloadObject, "vf-module-model-version")
            val k8sRbProfileName: String = getResolvedParameter(payloadObject, "k8s-rb-profile-name")
            val k8sRbProfileNamespace: String = getResolvedParameter(payloadObject, "k8s-rb-profile-namespace")
            val vnfId: String = getResolvedParameter(payloadObject, "vnf-id")
            log.info("******vnfID************   $vnfId")
            log.info("k8sRbProfileName $k8sRbProfileName")

            // Extract supportedNssai
            val api = K8sApi(k8sApiUsername, k8sApiPassword, baseK8sApiUrl, vfModuleModelInvariantUuid, vfModuleModelUuid)

            if (!api.hasDefinition()) {
                throw BluePrintProcessorException("K8s RB Definition ($vfModuleModelInvariantUuid/$vfModuleModelUuid) not found ")
            }

            log.info("k8s-rb-profile-name: $k8sRbProfileName")
            if (k8sRbProfileName.equals("")) {
                throw BluePrintProcessorException("K8s rb profile name is empty! Either define profile name to use or choose default")
            }
            if (k8sRbProfileName.equals("default") and api.hasProfile(k8sRbProfileName)) {
                log.info("Using default profile - skipping upload")
            } else {
                if (api.hasProfile(k8sRbProfileName)) {
                    log.info("Profile Already Existing - skipping upload")
                } else {
                    val profileFilePath: Path = prepareProfileFile(k8sRbProfileName, vnfId)

                    var profile = K8sProfile()
                    profile.profileName = k8sRbProfileName
                    profile.rbName = vfModuleModelInvariantUuid
                    profile.rbVersion = vfModuleModelUuid
                    profile.namespace = k8sRbProfileNamespace
                    api.createProfile(profile)
                    api.uploadProfileContent(profile, profileFilePath)

                    log.info("K8s Profile Upload Completed")
                }
            }
        }
    }

    fun prepareProfileFile(k8sRbProfileName: String, vnfId: String): Path {
        val bluePrintContext = bluePrintRuntimeService.bluePrintContext()
        val bluePrintBasePath: String = bluePrintContext.rootPath
        var profileFilePath: Path = Paths.get(bluePrintBasePath.plus(File.separator).plus("Templates").plus(File.separator).plus("k8s-profiles").plus(File.separator).plus("template-profile.tar.gz"))
        log.info("Reading K8s profile file: $profileFilePath")

        val profileFile = profileFilePath.toFile()

        if (!profileFile.exists())
            throw BluePrintProcessorException("K8s Profile template file $profileFilePath does not exists")

        val tempMainPath: File = createTempDir("k8s-profile-", "")
        val tempProfilePath: File = createTempDir("$k8sRbProfileName-", "", tempMainPath)
        log.info("Decompressing profile to $tempProfilePath")

        val decompressedProfile: File = BluePrintArchiveUtils.deCompress(
            profileFilePath.toFile(),
            "$tempProfilePath",
            ArchiveType.TarGz
        )

        log.info("$profileFilePath decompression completed")
        val destPath: String = "/tmp/k8s-profile-" + vnfId

        // Here we update override.yaml file

        log.info("Modification of override.yaml file ")
        val manifestFileName = destPath.toString().plus(File.separator).plus("override_values.yaml")
        val destOverrideFile = tempProfilePath.toString().plus(File.separator).plus("override_values.yaml")
        log.info("destination override file $destOverrideFile")
        File(manifestFileName).copyTo(File(destOverrideFile), true)
        profileFilePath = Paths.get(tempMainPath.toString().plus(File.separator).plus("template-profile.tar.gz"))

        if (!BluePrintArchiveUtils.compress(decompressedProfile, profileFilePath.toFile(), ArchiveType.TarGz)) {
            throw BluePrintProcessorException("Profile compression has failed")
        }
        log.info("$profileFilePath compression completed")
        return profileFilePath
    }

    fun getTemplatePrefixList(executionRequest: ExecutionServiceInput): ArrayList<String> {
        val result = ArrayList<String>()
        for (prefix in executionRequest.payload.get("resource-assignment-request").get("template-prefix").elements())
            result.add(prefix.asText())
        return result
    }

    fun createOverrideVaues(payloadObject: ObjectNode): String {
        // Extract supportedNssai
        val supportedNssaiMap = LinkedHashMap<String, Any>()
        val snssai: String = getResolvedParameter(payloadObject, "config.supportedNssai.sNssai.snssai")
        log.info("snssa1 $snssai")
        supportedNssaiMap.put("snssai", snssai)

        val bluePrintContext = bluePrintRuntimeService.bluePrintContext()
        val bluePrintBasePath: String = bluePrintContext.rootPath
        val vnfId: String = getResolvedParameter(payloadObject, "vnf-id")
        val destPath: String = "/tmp/k8s-profile-" + vnfId
        log.info("*********vnfID***** $vnfId")

        var profileFilePath: Path = Paths.get(bluePrintBasePath.plus(File.separator).plus("Templates").plus(File.separator).plus("k8s-profiles").plus(File.separator).plus("template-profile.tar.gz"))
        log.info("Reading K8s profile file: $profileFilePath")
        val profileFile = profileFilePath.toFile()

        if (!profileFile.exists())
            throw BluePrintProcessorException("K8s Profile template file $profileFilePath does not exists")

        val success = File(destPath).mkdirs()
        log.info("Decompressing profile to $destPath")

        val decompressedProfile: File = BluePrintArchiveUtils.deCompress(
            profileFilePath.toFile(),
            "$destPath",
            ArchiveType.TarGz
        )

        log.info("$profileFilePath decompression completed")

        // Here we update override.yaml file
        val manifestFileName = destPath.plus(File.separator).plus("override_values.yaml")
        log.info("Modification of override.yaml file at $manifestFileName")
        var finalManifest = ""
        File(manifestFileName).bufferedReader().use { inr ->
            val manifestYaml = Yaml()
            val manifestObject: Map<String, Any> = manifestYaml.load(inr)

            for ((k, v) in manifestObject) {
                log.info("manifestObject: $k, $v")
            }

            log.info("Uploaded YAML object")

            val configFiles: MutableMap<String, Any> = manifestObject.get("config") as MutableMap<String, Any>
            log.info("Uploaded config YAML object")

            for ((k, v) in configFiles) {
                log.info("configFiles: $k, $v")
            }

            val supportedNssai: MutableMap<String, Any> = configFiles.get("supportedNssai") as MutableMap<String, Any>
            log.info("Uploaded  supportedNssai YAML object")

            for ((k, v) in supportedNssai) {
                log.info("supportedNssai: $k, $v")
            }

            val sNssai: MutableMap<String, Any> = supportedNssai.get("sNssai") as MutableMap<String, Any>
            log.info("Uploaded  sNssai YAML object")

            for ((k, v) in sNssai) {
                log.info("sNssai: $k, $v")
            }

            for ((k, v) in supportedNssaiMap) {
                log.info("supportedNssaiMap: $k, $v")
                sNssai.put(k, v)
            }

            finalManifest = manifestYaml.dump(manifestObject)
        }

        File(manifestFileName).bufferedWriter().use { out -> out.write(finalManifest) }
        log.info("Modified K8s profile manifest file")
        log.info(finalManifest)
        log.info("Modification of profile completed")
        return ""
    }

    fun getResolvedParameter(payload: ObjectNode, keyName: String): String {
        for (node in payload.get("resource-accumulator-resolved-data").elements()) {
            if (node.get("param-name").asText().equals(keyName)) {
                return node.get("param-value").asText()
            }
        }
        return ""
    }

    fun getResolvedParameterbyCapabilityData(payload: ObjectNode, keyName: String): String {
        for (node in payload.get("capability-data").elements()) {
            log.info("node: $node")
            if (node.get("capability-name").asText().equals("unresolved-composite-data")) {
                log.info("inside")
                for (d in node.get("key-mapping")) {
                    log.info("d: $d")
                    for (value in d.get("output-key-mapping")) {
                        if (value.get("resource-name").asText().equals(keyName)) {
                            log.info("value: $value")
                            return value.get("resource-value").asText()
                        }
                    }
                }
            }
        }
        return ""
    }

    override suspend fun recoverNB(runtimeException: RuntimeException, executionRequest: ExecutionServiceInput) {
        log.info("Executing Recovery")
        bluePrintRuntimeService.getBluePrintError().addError("${runtimeException.message}")
    }

    inner class K8sApi(
        val username: String,
        val password: String,
        val baseUrl: String,
        val definition: String,
        val definitionVersion: String
    ) {
        private val service: UploadFileRestClientService // BasicAuthRestClientService

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

            this.service = UploadFileRestClientService(basicAuthRestClientProperties)
        }

        fun hasDefinition(): Boolean {
            try {
                val result: BlueprintWebClientService.WebClientResponse<String> = service.exchangeResource(HttpMethod.GET.name, "", "")
                print(result)
                if (result.status >= 200 && result.status < 300)
                    return true
                else
                    return false
            } catch (e: Exception) {
                log.info("Caught exception trying to get k8s rb definition")
                throw BluePrintProcessorException("${e.message}")
            }
        }

        fun hasProfile(profileName: String): Boolean {
            try {
                val result: BlueprintWebClientService.WebClientResponse<String> = service.exchangeResource(
                    HttpMethod.GET.name,
                    "/profile/$profileName",
                    ""
                )
                if (result.status >= 200 && result.status < 300)
                    return true
                else {
                    print(result)
                    return false
                }
            } catch (e: Exception) {
                log.info("Caught exception trying to get k8s rb profile")
                throw BluePrintProcessorException("${e.message}")
            }
        }

        fun createProfile(profile: K8sProfile) {
            val objectMapper = ObjectMapper()
            val profileJsonString: String = objectMapper.writeValueAsString(profile)
            try {
                val result: BlueprintWebClientService.WebClientResponse<String> = service.exchangeResource(
                    HttpMethod.POST.name,
                    "/profile",
                    profileJsonString
                )
                if (result.status < 200 || result.status >= 300) {
                    throw Exception(result.body)
                }
            } catch (e: Exception) {
                log.info("Caught exception trying to create k8s rb profile ${profile.profileName}")
                throw BluePrintProcessorException("${e.message}")
            }
        }

        fun uploadProfileContent(profile: K8sProfile, filePath: Path) {
            try {
                val result: BlueprintWebClientService.WebClientResponse<String> = service.uploadBinaryFile(
                    "/profile/${profile.profileName}/content",
                    filePath
                )
                if (result.status < 200 || result.status >= 300) {
                    throw Exception(result.body)
                }
            } catch (e: Exception) {
                log.info("Caught exception trying to upload k8s rb profile ${profile.profileName}")
                throw BluePrintProcessorException("${e.message}")
            }
        }
    }
}

class UploadFileRestClientService(
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

class K8sProfile {
    @get:JsonProperty("rb-name")
    var rbName: String? = null
    @get:JsonProperty("rb-version")
    var rbVersion: String? = null
    @get:JsonProperty("profile-name")
    var profileName: String? = null
    @get:JsonProperty("namespace")
    var namespace: String? = "default"

    override fun toString(): String {
        return "$rbName:$rbVersion:$profileName"
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
