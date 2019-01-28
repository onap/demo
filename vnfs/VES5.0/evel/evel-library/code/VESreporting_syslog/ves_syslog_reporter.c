/*************************************************************************//**
 *
 * Copyright © 2018 AT&T Intellectual Property. All rights reserved.
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
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "jsmn.h"
#include "evel.h"

#define BUFSIZE 128
#define MAX_BUFFER_SIZE 4096
#define MAX_TOKENS 1000

void *SyslogThread(void *threadarg);

unsigned long long epoch_start = 0;

void report_syslog(char * js, jsmntok_t * tokens, int numToken, char * syslog_tag, char * eName, int srcTyp, char * syslog_msg)
{
  EVENT_HEARTBEAT_FIELD * event = NULL;
  EVENT_HEADER* syslog_header = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;
  char stringVal[BUFSIZE];

  char event_id1[10] = "syslog";
  char event_id2[15] = {0};
  int event_id3 = 0;
  char event_id[BUFSIZE] = {0};

  int ret = 0;
  /***************************************************************************/
  /* Syslog                                                               */
  /***************************************************************************/
  memset(event_id, 0, BUFSIZE);
  memset(event_id2, 0, 15);
  sprintf(event_id2, "%09d", event_id3++);
  strcat(event_id, event_id1);
  strcat(event_id, event_id2);

     
  event = evel_new_syslog(eName, event_id, srcTyp, syslog_msg, syslog_tag);
  
  if (event != NULL)
  {
       syslog_header = (EVENT_HEADER *)event;

       ret = getStringToken(js, tokens, numToken, "syslogProc", "tmp_directParameters", stringVal, BUFSIZE);
       if (ret == 0)
          evel_syslog_proc_set(event, stringVal);
       evel_syslog_facility_set(event, EVEL_SYSLOG_FACILITY_LOCAL0);
       ret = getStringToken(js, tokens, numToken, "eventType", "tmp_directParameters", stringVal, BUFSIZE);
       if (ret == 0)
         evel_header_type_set(&event->header, stringVal);

       struct timeval tv_now;
       gettimeofday(&tv_now, NULL);
       unsigned long long epoch_now = tv_now.tv_usec + 1000000 * tv_now.tv_sec;

       evel_start_epoch_set(&event->header, epoch_start);
       evel_last_epoch_set(&event->header, epoch_now);
       epoch_start = epoch_now;

       ret = getStringToken(js, tokens, numToken, "nfcNamingCode", "tmp_directParameters", stringVal, BUFSIZE);
       if (ret == 0)
         evel_nfcnamingcode_set(&event->header, stringVal);
 
       ret = getStringToken(js, tokens, numToken, "nfNamingCode", "tmp_directParameters", stringVal, BUFSIZE);
       if (ret == 0)
         evel_nfnamingcode_set(&event->header, stringVal);

       ret = getStringToken(js, tokens, numToken, "reportingEntityName", "tmp_directParameters", stringVal, BUFSIZE);
       if (ret == 0)
         evel_reporting_entity_name_set(&event->header, stringVal);
       else 
       {
          printf("Missing mandatory parameters - reportingEntityName is not there in tmp_directParameters\n");
          printf("Defaulting reportingEntityName to hostname\n");
       }

       ret = getStringToken(js, tokens, numToken, "reportingEntityId", "tmp_directParameters", stringVal, BUFSIZE);
       if (ret == 0)
         evel_reporting_entity_id_set(&event->header, stringVal);

       ret = getStringToken(js, tokens, numToken, "sourceId", "tmp_directParameters", stringVal, BUFSIZE);
       if (ret == 0)
         evel_source_id_set(&event->header, stringVal);

       ret = getStringToken(js, tokens, numToken, "sourceName", "tmp_directParameters", stringVal, BUFSIZE);
       if (ret == 0)
         evel_source_name_set(&event->header, stringVal);
       else 
       {
          printf("Missing mandatory parameters - sourceName is not there in tmp_directParameters\n");
          printf("Defaulting sourceName to hostname\n");
       }

       evel_rc = evel_post_event(syslog_header);
       if (evel_rc != EVEL_SUCCESS)
       {
         EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
       }
    }
    else
    {
      EVEL_ERROR("New Syslog failed");
    }
    printf("   Processed Syslog\n");
}

int get_source(char * inStr)
{
   int result = -1;

   if(strcmp(inStr, "other") == 0)
     result = EVEL_SOURCE_OTHER;
   else if(strcmp(inStr, "router") == 0)
     result = EVEL_SOURCE_ROUTER;
   else if(strcmp(inStr, "switch") == 0)
     result = EVEL_SOURCE_SWITCH;
   else if(strcmp(inStr, "host") == 0)
     result = EVEL_SOURCE_HOST;
   else if(strcmp(inStr, "card") == 0)
     result = EVEL_SOURCE_CARD;
   else if(strcmp(inStr, "port") == 0)
     result = EVEL_SOURCE_PORT;
   else if(strcmp(inStr, "slotThreshold") == 0)
     result = EVEL_SOURCE_SLOT_THRESHOLD;
   else if(strcmp(inStr, "portThreshold") == 0)
     result = EVEL_SOURCE_PORT_THRESHOLD;
   else if(strcmp(inStr, "virtualMachine") == 0)
     result = EVEL_SOURCE_VIRTUAL_MACHINE;
   else if(strcmp(inStr, "virtualNetworkFunction") == 0)
     result = EVEL_SOURCE_VIRTUAL_NETWORK_FUNCTION;

  return result;
}


int main(int argc, char** argv)
{
  char* fqdn = argv[1];
  int port = atoi(argv[2]);
  int i=0;
  int rc;
  pthread_attr_t attr;
  pthread_t syslog_thread;
  char* fqdn2 = NULL;
  int port2 = 0;

  if(argc == 5)
  {
     fqdn2 = argv[3];
     port2 = atoi(argv[4]);
  }

  if (!((argc == 3) || (argc == 5)))
  {
    fprintf(stderr, "Usage: %s <FQDN>|<IP address> <port> <FQDN>|<IP address> <port>  \n", argv[0]);
    fprintf(stderr, "OR\n");
    fprintf(stderr, "Usage: %s <FQDN>|<IP address> <port> \n", argv[0]);
    exit(-1);
  }

  /**************************************************************************/
  /* Initialize                                                             */
  /**************************************************************************/
  if(evel_initialize(fqdn,                         /* FQDN                  */
                     port, 	                   /* Port                  */
                     fqdn2, 	                   /* Backup FQDN           */
                     port2, 	                   /* Backup port           */
                     NULL,                         /* optional path         */
                     NULL,                         /* optional topic        */
                     100,                          /* Ring Buffer size      */
                     0,                            /* HTTPS?                */
                     NULL,                         /* cert file             */
                     NULL,                         /* key  file             */
                     NULL,                         /* ca   info             */
                     NULL,                         /* ca   file             */
                     0,                            /* verify peer           */
                     0,                            /* verify host           */
                     "sample1",                    /* Username              */
                     "sample1",                    /* Password              */
                     "sample1",                    /* Username2             */
                     "sample1",                    /* Password2             */
                     NULL,                         /* Source ip             */
                     NULL,                         /* Source ip2            */
                     EVEL_SOURCE_VIRTUAL_MACHINE,  /* Source type           */
                     "vSyslog",     	           /* Role                  */
                     1))                           /* Verbosity             */
  {
    fprintf(stderr, "\nFailed to initialize the EVEL library!!!\n");
    exit(-1);
  }
  else
  {
    printf("\nInitialization completed\n");
  }

  /* Initialize and set thread detached attribute */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  printf("Main:Creating thread \n");
  rc = pthread_create(&syslog_thread, NULL, SyslogThread, &i);
  if (rc)
  {
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    exit(-1);
  }
  printf("Main:Created Syslog thread \n");

  pthread_join(syslog_thread, NULL);

  evel_terminate();
  printf("Terminated\n");
  return 0;
}

void *SyslogThread(void *threadarg)
{

  int numToken;
  jsmntok_t tokens[MAX_TOKENS];
  char js[MAX_BUFFER_SIZE]; 
  FILE * fp;
  jsmn_parser p;
  char ch[BUFSIZE];
  int ret = 0;
  char eName[BUFSIZE];
  char eventSrcTyp[BUFSIZE];
  char syslog_tag[BUFSIZE];
  char syslog_msg[8*BUFSIZE];
  char str[8*BUFSIZE];
  char syslog_file[BUFSIZE];
  int srcTyp = 0;
  int count = 0;
  unsigned long long pos;
  unsigned long long prevpos = 0;

  sleep(1);
  printf("Running Syslog thread \n");
  fflush(stdout);

  while(1)
  {

     FILE * file = fopen("syslog_config.json", "r");

     memset(js, 0, MAX_BUFFER_SIZE);
     memset(ch, 0, BUFSIZE);

     while((fgets(ch, (BUFSIZE-1), file)) !=NULL)
     {
        strcat(js, ch);
        memset(ch, 0, BUFSIZE);
     }
//   printf("the file content is \n %s \n", js);

     jsmn_init(&p);
     numToken = jsmn_parse(&p, js, strlen(js), tokens, MAX_TOKENS);
//     printf("Token count-%d\n", numToken);

//   printToken(js,tokens, numToken);

     ret = getStringToken(js, tokens, numToken, "tmp_syslogFile", "tmp_indirectParameters", syslog_file, BUFSIZE);
     if (ret != 0)
     {
        printf("Missing mandatory parameters - tmp_syslogFile is not there in tmp_indirectParameters. Exiting...\n");
        exit(1);
     }

     ret = getStringToken(js, tokens, numToken, "syslogTag", "tmp_directParameters", syslog_tag, BUFSIZE);
     if (ret != 0)
     {
        printf("Missing mandatory parameters - syslogTag is not there in tmp_directParameters, exiting..\n");
        exit(1);
     }

     ret = getStringToken(js, tokens, numToken, "eventName", "tmp_directParameters", eName, BUFSIZE);
     if (ret != 0)
     {
        printf("Missing mandatory parameters - eventName is not there in tmp_directParameters. Exiting...\n");
        exit(1);
     }

     ret = getStringToken(js, tokens, numToken, "eventSourceType", "tmp_directParameters", eventSrcTyp, BUFSIZE);
     if (ret != 0)
     {
        printf("Missing mandatory parameters - eventSourceType is not there in tmp_directParameters, exiting..\n");
        exit(1);
     }
     srcTyp = get_source(eventSrcTyp);
     if(srcTyp == -1)
     {
        printf("Fault eventSourceType value is not matching, eventSourceType-%s \n", eventSrcTyp);
        exit(1);
     }

    // Open file in read mode
    fp = fopen(syslog_file, "r");
    if (fp == NULL)
    {
        printf("Error while opening file");
        exit(EXIT_FAILURE);
    }

     memset(str, 0, 8*BUFSIZE);
     if (fseek(fp, 0, SEEK_END))
        perror("fseek() failed");
     else
     {
        // pos will contain no. of chars in input file.
        int n = 8;
        pos = ftell(fp);

        // search for '\n' characters
        while (pos>prevpos)
        {
            // Move 'pos' away from end of file.
            if (!fseek(fp, --pos, SEEK_SET))
            {
                if (fgetc(fp) == '\n')

                    // stop reading when n newlines is found
                    if (count++ == n)
                        break;
            }
            else
                perror("fseek() failed");
        }
        //printf("pos %d prevpos %d\n",pos,prevpos);

        // print last n lines
        prevpos = pos;

        while (fgets(str, sizeof(str), fp))
        {
            char * position;
           // printf("str is - %s\n", str);
            if ((position=strchr(str, '\n')) != NULL)
               *position = '\0';
            if(strstr(str, syslog_tag) && !strstr(str,"EVEL") && !strstr(str,"commonEventHeader") && !strstr(str,"syslogMsg") && !strstr(str,"syslogTag"))
            {
               memset(syslog_msg, 0, 8*BUFSIZE);
               memcpy(syslog_msg, str, 8*BUFSIZE);
               report_syslog(js, tokens, numToken, syslog_tag, eName, srcTyp, syslog_msg);
            }
       
            prevpos += strlen(str);
            //printf("new prevpos is %d, size of str is %d\n", prevpos, strlen(str));
        }
    }
    sleep(3);
  }
}

