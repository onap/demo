/*************************************************************************//**
 *
 * Main Agent which spins up monitoring threads
 *
 *   Version 1.0:  Gokul Singaraju   gs244f   Tech Mahindra Inc.
 *
 * Copyright © 2017 AT&T Intellectual Property. All rights reserved.
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
#include <stdio.h>
#include <unistd.h>
#include <string.h> /* for strncpy */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "evel.h"
#include "afx_ves_reporter.h"


/**************************************************************************//**
 * Utility function to Create and send fault events.
 *****************************************************************************/
void report_fault( char* evname, char *evid, EVEL_SEVERITIES sevty, char *categ, char *intf, char *trapname, char *descr, char *rem_router, char *routername, char *router_ip, int status )
{
  EVENT_FAULT * fault = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  /***************************************************************************/
  /* Fault                                                                   */
  /***************************************************************************/
  if (status == 1)  /** ACTIVE **/
  {
    fault = evel_new_fault(evname,
			 evid,
                         trapname,
			 descr,
                         EVEL_PRIORITY_HIGH,
			 sevty,
			 EVEL_SOURCE_ROUTER,
			 EVEL_VF_STATUS_ACTIVE);
  }
  else
  {
    fault = evel_new_fault(evname,
			 evid,
                         trapname,
			 descr,
                         EVEL_PRIORITY_HIGH,
			 sevty,
			 EVEL_SOURCE_ROUTER,
			 EVEL_VF_STATUS_IDLE);
  }

  if (fault != NULL)
  {
    evel_header_type_set((EVENT_HEADER *)fault, "applicationVnf");
    evel_nfcnamingcode_set((EVENT_HEADER *)fault, "AFX");
    evel_nfnamingcode_set((EVENT_HEADER *)fault, "AFX");

    evel_fault_category_set(fault, categ);
    if( intf != NULL)
      evel_fault_interface_set(fault, intf );

    if( strstr(evname,"Fault_vAfx_Link") != NULL || strstr(evname,"Fault_vAfx_bgp") != NULL)
    {
      evel_fault_addl_info_add(fault, "remote_router", rem_router);
      evel_fault_addl_info_add(fault, "router_name", routername);
    }

    if( strstr(evname,"Fault_vAfx_bgp") != NULL )
      evel_fault_addl_info_add(fault, "router_ip", router_ip);

    evel_rc = evel_post_event((EVENT_HEADER *)fault);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }
  else
  {
    EVEL_ERROR("New Fault failed");
  }
  printf(" Generated Fault event\n");
}


int file_is_modified(const char *path, time_t *oldMTime) {
    struct stat file_stat;
    int err = stat(path, &file_stat);
    if (err != 0) {
        perror(" [file_is_modified] stat");
        exit(1);
    }
    if( file_stat.st_mtime > *oldMTime)
    {
      *oldMTime = file_stat.st_mtime;
      return 1;
    }
    else
      return 0;
}

void remove_spaces(char* source)
{
  char* i = source;
  char* j = source;
  while(*j != 0)
  {
    *i = *j++;
    if(*i != ' ')
      i++;
  }
  *i = 0;
}


char *get_oam_intfaddr()
{
 int fd;
 int ret;
 struct ifreq ifr;
 char oamaddr[64];

 fd = socket(AF_INET, SOCK_DGRAM, 0);

 /* I want to get an IPv4 IP address */
 ifr.ifr_addr.sa_family = AF_INET;

 /* I want IP address attached to "eth0" */
 strncpy(ifr.ifr_name, OAM_INTERFACE, IFNAMSIZ-1);

 ret = ioctl(fd, SIOCGIFADDR, &ifr);

 close(fd);

 if( ret == 0 )
 {
   /* display result */
   sprintf(oamaddr,"%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
   //printf("Loopback address %s\n", oamaddr);
 }
 else
   sprintf(oamaddr,"127.0.0.1");

 return strdup(oamaddr);
}


char *escape_json(char *in) {
    char out[2048];
    char *c;
    char *o;
    o = &out[0];
    for (c = in; *c != '\0'; c++) {
        switch (*c) {
        case '"': strcat(o,"\\\"");o+=2; break;
        case '\\': strcat(o, "\\\\");o+=2; break;
        case '\b': strcat(o,  "\\b");o+=2; break;
        case '\f': strcat(o, "\\f");o+=2; break;
        case '\n': strcat(o, "\\n");o+=2; break;
        case '\r': strcat(o, "\\r");o+=2; break;
        case '\t': strcat(o, "\\t");o+=2; break;
        default:
            if ('\x00' <= *c && *c <= '\x1f') {
                char tmp[8];
                strcat(o, "\\u");
                sprintf(tmp,"%04X",(int)*c);
                strcat(o,tmp); o+=6;
            } else {
                *o = *c; o++;
            }
        }
    }
    *o = '\0';
    //free(in);
    return strdup(out);
}

