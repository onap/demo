/*****************************************************************************
* @file
* main function to process ipv4 packets
* -------
*
* Copyright (C) 2017 AT&T Intellectual Property. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*        http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
****************************************************************************/

#include <dhcp/pkt4.h>
#include <log/logger.h>
#include <log/macros.h>
#include <log/message_initializer.h>
#include "library_common.h"
#include <curl/curl.h>
#include <sstream>

using namespace isc::dhcp;
using namespace isc::hooks;
using namespace std;
using namespace isc::log;
namespace pt = boost::property_tree;


isc::log::Logger logger("sdnc-notify-logger");
const char* log_messages[] = {
    "SNL_BASE", "message: %1",
    "SNL_PKT_SEND", "Outgoing packet: \n%1",
    "SNL_CURL_RECEIVED", "Received: \n%1",
    "SNL_CURL_FAILED", "Unable to receive data from %1",
    NULL
};

/// Initializer for log messages.
const MessageInitializer message_initializer(log_messages);

extern "C" {
int pkt4_send(CalloutHandle& handle) {

    // Curl variables 
    CURL *curl;
    CURLcode res;
    struct curl_slist *list=NULL;
    bool perform_updates = 0;
    int curl_opt_res = 0;

    // "C" variables.
    char *bp;
    size_t size;
    FILE *response_memfile;

    // String variables
    string hwaddr;
    string final_url;
    string msg_name;
    string yiaddr;
    string post_data;
    

    // Pkt4 variables.
    Pkt4Ptr response4_ptr;
    HWAddrPtr hwaddr_ptr;
    
    // Boost / json related variables.
    pt::ptree root;
    std::stringstream ss;
    boost::optional<std::string> siaddr_json_field;

    /* Begin Code */
    handle.getArgument("response4", response4_ptr);
    hwaddr_ptr = response4_ptr->getHWAddr();
    isc::asiolink::IOAddress new_yiaddr(response4_ptr->getYiaddr());
    hwaddr = hwaddr_ptr->toText(false);
    yiaddr = new_yiaddr.toText();
    msg_name = response4_ptr->getName();
    /* POST string for DMaaP */
    post_data = "{\n\"macaddr\":\"" + hwaddr + "\",\n\"yiaddr\":\"" + yiaddr + "\",\n\"msg_name\":\"" + msg_name + "\"\n}";
    final_url = json_params[0] ;
    LOG_DEBUG(logger, 0, "SNL_BASE").arg(final_url);
    LOG_DEBUG(logger, 0, "SNL_BASE").arg(yiaddr);
    LOG_DEBUG(logger, 0, "SNL_BASE").arg(post_data);


    if ( msg_name == "DHCPACK")
        perform_updates = 1;

    if (!perform_updates) {
        LOG_WARN(logger, "SNL_BASE").arg("Nothing to update.");
        return(0);
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(!curl) {
        curl_global_cleanup();
        LOG_ERROR(logger, "SNL_BASE").arg("Could not initialize curl");
        return(1);
    }

    list = curl_slist_append(list, "Content-type: application/json");
    list = curl_slist_append(list, "Accept: application/json");
    if (list == NULL) {
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        LOG_ERROR(logger, "SNL_BASE").arg("Could not create curl slist.");
        return(1);
    }

    response_memfile = open_memstream (&bp, &size);
    if (response_memfile == NULL) {
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        LOG_ERROR(logger, "SNL_BASE").arg("Could not create memfile.");
        return(1);
    }
	
    // DEBUGGING
    curl_opt_res += curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_opt_res += curl_easy_setopt(curl, CURLOPT_HEADER, 1L);

    // Default curl timeout is 300 seconds 
    curl_opt_res += curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1L);
    curl_opt_res += curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
    curl_opt_res += curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response_memfile);
    curl_opt_res += curl_easy_setopt(curl, CURLOPT_URL, (final_url).c_str());
    curl_opt_res += curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1L );
    curl_opt_res += curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str() );
    curl_opt_res += curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    #ifdef SKIP_PEER_VERIFICATION
    curl_opt_res += curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    #endif

    #ifdef SKIP_HOSTNAME_VERIFICATION
    curl_opt_res += curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    #endif
    if (curl_opt_res > 0) {
      fclose(response_memfile);
      free(bp);
      curl_easy_cleanup(curl);
      curl_global_cleanup();
      LOG_ERROR(logger, "SNL_BASE").arg("Error setting curl options.");
      return(1);
    }
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
        fclose(response_memfile);
        free(bp);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        LOG_ERROR(logger, "SNL_CURL_FAILED").arg(ss.str());
        return(1);
    }
    // make bp available for reading.
    fclose(response_memfile);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    ss << bp;
    free(bp);
    LOG_DEBUG(logger, 0, "SNL_CURL_RECEIVED").arg(ss.str());
    // Load the json file in this ptree


    LOG_DEBUG(logger, 0, "SNL_PKT_SEND").arg(response4_ptr->toText());

    return(0);
}
}
