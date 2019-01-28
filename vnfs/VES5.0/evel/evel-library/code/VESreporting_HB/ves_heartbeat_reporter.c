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

void *HeartbeatThread(void *threadarg);

unsigned long long epoch_start = 0;

int main(int argc, char** argv)
{
  char* fqdn = argv[1];
  int port = atoi(argv[2]);
  int i=0;
  int rc;
  pthread_attr_t attr;
  pthread_t hb_thread;
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
                     "vHeartbeat",     	           /* Role                  */
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
  rc = pthread_create(&hb_thread, NULL, HeartbeatThread, &i);
  if (rc)
  {
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    exit(-1);
  }
  printf("Main:Created HB thread \n");

  pthread_join(hb_thread, NULL);

  evel_terminate();
  printf("Terminated\n");
  return 0;
}

void *HeartbeatThread(void *threadarg)
{

  EVENT_HEARTBEAT_FIELD * event = NULL;
  EVENT_HEADER* hb_header = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  int numToken;
  jsmntok_t tokens[MAX_TOKENS];
  char js[MAX_BUFFER_SIZE]; 

  jsmn_parser p;
  char ch[BUFSIZE];
  int ret = 0;
  char stringVal[BUFSIZE];
  char eName[BUFSIZE];
  int hb_interval;

  char event_id1[10] = "heartbeat";
  char event_id2[15] = {0};
  int event_id3 = 0;
  char event_id[BUFSIZE] = {0};

  sleep(1);
  printf("Running HB thread \n");
  fflush(stdout);

  while(1)
  {

     FILE * file = fopen("hb_config.json", "r");

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
     printf("Token count-%d\n", numToken);

//   printToken(js,tokens, numToken);

     ret = getStringToken(js, tokens, numToken, "eventName", "tmp_directParameters", eName, BUFSIZE);
     if (ret != 0)
     {
        printf("Missing mandatory parameters - eventName is not there in tmp_directParameters. Exiting...\n");
        exit(1);
     }

     ret = getIntToken(js, tokens, numToken, "heartbeatInterval", "tmp_directParameters", &hb_interval);
     if (ret != 0)
     {
        printf("The parameter heartbeatInterval is not defined, defaulted to 60 seconds\n");
        hb_interval = 60;
     }

     /***************************************************************************/
     /* Heartbeat                                                               */
     /***************************************************************************/
     memset(event_id, 0, BUFSIZE);
     memset(event_id2, 0, 15);
     sprintf(event_id2, "%09d", event_id3++);
     strcat(event_id, event_id1);
     strcat(event_id, event_id2);

     event = evel_new_heartbeat_field(hb_interval, eName, event_id);
     if (event != NULL)
     {
       hb_header = (EVENT_HEADER *)event;

       ret = getStringToken(js, tokens, numToken, "eventType", "tmp_directParameters", stringVal, BUFSIZE);
       if (ret == 0)
       {
         evel_header_type_set(&event->header, stringVal);
       }

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

       evel_rc = evel_post_event(hb_header);
       if (evel_rc != EVEL_SUCCESS)
       {
         EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
       }
    }
    else
    {
      EVEL_ERROR("New Heartbeat failed");
    }
    printf("   Processed Heartbeat\n");

    sleep(hb_interval);
  }
}
