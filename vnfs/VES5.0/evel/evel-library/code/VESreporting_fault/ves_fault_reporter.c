/*************************************************************************//**
 *
 * Copyright © 2019 AT&T Intellectual Property. All rights reserved.
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
#define MAX_INTERFACES 40

void *FaultThread(void *threadarg);
void *FaultThread01(void *threadarg);
void *FaultThread02(void *threadarg);
void *FaultThread03(void *threadarg);

typedef struct dummy_vpp_metrics_struct {
  int curr_bytes_in;
  int curr_bytes_out;
  int curr_packets_in;
  int curr_packets_out;
  int last_bytes_in;
  int last_bytes_out;
  int last_packets_in;
  int last_packets_out;
} vpp_metrics_struct;

typedef struct linkstat {

  char linkname[32];
  char linkdescr[64];
  char linkmode[64];
  int  speedmbps;
  int  fault_raised;
  unsigned long long last_epoch;

}LINKSTAT;

//vpp_metrics_struct meas_intfstat[MAX_INTERFACES];
//LINKSTAT meas_linkstat[MAX_INTERFACES];
//int request_rate=0;
vpp_metrics_struct fault_intfstat[MAX_INTERFACES];
LINKSTAT fault_linkstat[MAX_INTERFACES];

unsigned long long epoch_start = 0;

int format_val_params(KEYVALRESULT * keyValArray, int numElements, const char *replace, const char *search)
{
     char *sp;
     int i =0;
     int search_len;
     int replace_len;
     int tail_len;

     for (i=0; i<numElements; i++)
     {
        if ((sp = strstr(keyValArray[i].valStr, search)) == NULL) {
           printf("\n String not found\n");
           return 1; //Error search string not found
        }
        
        search_len = strlen(search);
        replace_len = strlen(replace);
        tail_len = strlen(sp+search_len);
        memmove(sp+replace_len,sp+search_len,tail_len+1);
        memcpy(sp, replace, replace_len);
//       printf("\n Changed value for i=%d is %s", i, keyValArray[i].valStr);
     }
     return 0; //search and replace is successful
}

void runCommands(KEYVALRESULT * commandArray, int numCommands)
{

  char buf[BUFSIZE];            /* buffer used to store VPP metrics     */
  FILE *fp;                     /* file descriptor to pipe cmd to shell */
  int i;

  for(i = 0; i < numCommands; i++)
  {
      memset(buf, 0, BUFSIZE);

      // Open a pipe and read VPP values
      if ((fp = popen(commandArray[i].valStr, "r")) == NULL) {
          printf("Error opening pipe!\n");
          return;
      }

      while (fgets(buf, BUFSIZE, fp) != NULL);
      // printf("\n the vpp value is %d\n", atoi(buf));
      strcpy(commandArray[i].resultStr, buf);

      if(pclose(fp))  {
          printf("Command not found or exited with error status\n");
          return;
      }
   }
   //for(i = 0; i < numCommands; i++)
   //   printf("\n commandArray[%d].resultStr is- %s\n", i, commandArray[i].resultStr);
}


void copy_vpp_metic_data(vpp_metrics_struct *intfstats, KEYVALRESULT * cmdArray, int numCmds, int linkNum)
{
    int i;

    // Store the current metric in the last metric
    intfstats[linkNum].last_bytes_in = intfstats[linkNum].curr_bytes_in;
    intfstats[linkNum].last_bytes_out = intfstats[linkNum].curr_bytes_out;
    intfstats[linkNum].last_packets_in = intfstats[linkNum].curr_packets_in;
    intfstats[linkNum].last_packets_out = intfstats[linkNum].curr_packets_out;

    // Store metrics read from the vNIC in the current
    for(i=0; i<numCmds; i++)
    {
       if((strcmp(cmdArray[i].keyStr, "tmp_t0BytesIn") == 0) ||
          (strcmp(cmdArray[i].keyStr, "tmp_t1BytesIn") == 0))
          intfstats[linkNum].curr_bytes_in = atoi(cmdArray[i].resultStr);

       if((strcmp(cmdArray[i].keyStr, "tmp_t0BytesOut") == 0) ||
          (strcmp(cmdArray[i].keyStr, "tmp_t1BytesOut") == 0))
          intfstats[linkNum].curr_bytes_out = atoi(cmdArray[i].resultStr);

       if((strcmp(cmdArray[i].keyStr, "tmp_t0PacketsIn") == 0) ||
          (strcmp(cmdArray[i].keyStr, "tmp_t1PacketsIn") == 0))
          intfstats[linkNum].curr_packets_in = atoi(cmdArray[i].resultStr);

       if((strcmp(cmdArray[i].keyStr, "tmp_t0PacketsOut") == 0) ||
          (strcmp(cmdArray[i].keyStr, "tmp_t1PacketsOut") == 0))
          intfstats[linkNum].curr_packets_out = atoi(cmdArray[i].resultStr);
    }
    // printf("intfstats[%d].curr_bytes_in = %d\n", linkNum, intfstats[linkNum].curr_bytes_in);
    // printf("intfstats[%d].curr_bytes_out = %d\n", linkNum, intfstats[linkNum].curr_bytes_out);
    // printf("intfstats[%d].curr_packets_in = %d\n", linkNum, intfstats[linkNum].curr_packets_in);
    // printf("intfstats[%d].curr_packets_out = %d\n", linkNum, intfstats[linkNum].curr_packets_out);
}

int get_severity(char * inStr)
{
   int result = -1;

   if(strcmp(inStr, "CRITICAL") == 0)
     result = EVEL_SEVERITY_CRITICAL;
   else if(strcmp(inStr, "MAJOR") == 0)
     result = EVEL_SEVERITY_MAJOR;
   else if(strcmp(inStr, "MINOR") == 0)
     result = EVEL_SEVERITY_MINOR;
   else if(strcmp(inStr, "WARNING") == 0)
     result = EVEL_SEVERITY_WARNING;
   else if(strcmp(inStr, "NORMAL") == 0)
     result = EVEL_SEVERITY_NORMAL;

  return result;
}

int get_priority(char * inStr)
{
   int result = -1;

   if(strcmp(inStr, "High") == 0)
     result = EVEL_PRIORITY_HIGH;
   else if(strcmp(inStr, "Medium") == 0)
     result = EVEL_PRIORITY_MEDIUM;
   else if(strcmp(inStr, "Normal") == 0)
     result = EVEL_PRIORITY_NORMAL;
   else if(strcmp(inStr, "Low") == 0)
     result = EVEL_PRIORITY_LOW;

  return result;
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

int get_vf_status(char * inStr)
{
   int result = -1;

   if(strcmp(inStr, "Active") == 0)
     result = EVEL_VF_STATUS_ACTIVE;
   else if(strcmp(inStr, "Idle") == 0)
     result = EVEL_VF_STATUS_IDLE;
   else if(strcmp(inStr, "Preparing to terminate") == 0)
     result = EVEL_VF_STATUS_PREP_TERMINATE;
   else if(strcmp(inStr, "Ready to terminate") == 0)
     result = EVEL_VF_STATUS_READY_TERMINATE;
   else if(strcmp(inStr, "Requesting termination") == 0)
     result = EVEL_VF_STATUS_REQ_TERMINATE;

  return result;
}

int main(int argc, char** argv)
{
  char* fqdn = argv[1];
  int port = atoi(argv[2]);
  int i=0;
  int rc;
  pthread_attr_t attr;
  pthread_t flt_thread;
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
                     "vFault",     	           /* Role                  */
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
  rc = pthread_create(&flt_thread, NULL, FaultThread, &i);
  if (rc)
  {
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    exit(-1);
  }
  printf("Main:Created Fault thread \n");

  pthread_join(flt_thread, NULL);

  evel_terminate();
  printf("Terminated\n");
  return 0;
}

void *FaultThread(void *mainFault)
{

   pthread_attr_t attr;
   pthread_t flt_thread01;
   pthread_t flt_thread02;
   pthread_t flt_thread03;
   int faultInstance01 = 0;
   int faultInstance02 = 0;
   int faultInstance03 = 0;
   int rc;

   int numToken;
   jsmntok_t tokens[MAX_TOKENS];
   char js[MAX_BUFFER_SIZE]; 

   jsmn_parser p;
   char ch[BUFSIZE];
   int ret = 0;

   memset(js, 0, MAX_BUFFER_SIZE);
   memset(ch, 0, BUFSIZE);

   sleep(1);
   printf("Running Main Fault thread \n");
   fflush(stdout);

   /* Initialize and set thread detached attribute */
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

   FILE * file = fopen("flt_config.json", "r");

   while((fgets(ch, (BUFSIZE-1), file)) !=NULL)
   {
      strcat(js, ch);
      memset(ch, 0, BUFSIZE);
   }
   //printf("MainFaulThread::the file content is \n %s \n", js);
   //printf("\n MainFaulThread::MMMMMMMMMMMMMMM\n");

   jsmn_init(&p);
   numToken = jsmn_parse(&p, js, strlen(js), tokens, MAX_TOKENS);
   //printf("count-%d\n", numToken);

   //printToken(js,tokens, numToken);

   printf("Main Fault Thread: Creating other fault threads\n");

   ret = isTokenPresent(js, tokens, numToken, "tmp_faultInstance01", "tmp_indirectParameters");
   if (ret == 0)
   {
//      rc = pthread_create(&flt_thread01, NULL, FaultThread01, "tmp_faultInstance01");
      rc = pthread_create(&flt_thread01, NULL, FaultThread01, &ret);
      if (rc)
      {
        printf("Main Fault Thread::ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
      }
      faultInstance01 = 1;
   }

   ret = isTokenPresent(js, tokens, numToken, "tmp_faultInstance02", "tmp_indirectParameters");
   if (ret == 0)
   {
//      rc = pthread_create(&flt_thread02, NULL, FaultThread02, "tmp_faultInstance02");
      rc = pthread_create(&flt_thread02, NULL, FaultThread02, &ret);
      if (rc)
      {
        printf("Main Fault Thread::ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
      }
      faultInstance02 = 1;
   }

   ret = isTokenPresent(js, tokens, numToken, "tmp_faultInstance03", "tmp_indirectParameters");
   if (ret == 0)
   {
      rc = pthread_create(&flt_thread03, NULL, FaultThread03, &ret);
      if (rc)
      {
        printf("Main Fault Thread::ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
      }
      faultInstance03 = 1;
   }

   if (faultInstance01 == 1)
   {
       pthread_join(flt_thread01, NULL);
   }

   if (faultInstance02 == 1)
   {
       pthread_join(flt_thread02, NULL);
   }

   if (faultInstance03 == 1)
   {
       pthread_join(flt_thread03, NULL);
   }

   while(1)
   {
       sleep(100);
   }
}

void *FaultThread01(void *faultInstanceTag)
{
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;
  EVENT_FAULT * fault = NULL;
  EVENT_HEADER* fault_header = NULL;
  int bytes_in;
  int bytes_out;
  int packets_in;
  int packets_out;
  unsigned long long epoch_now;
  int lowWaterMark;

  struct timeval time_val;
  char event_id1[10] = "fault";
  char event_id2[15] = {0};
  char event_id[BUFSIZE] = {0};
  int fault_event_id = 0;

   int numToken;
   jsmntok_t tokens[MAX_TOKENS];
   char js[MAX_BUFFER_SIZE]; 

   jsmn_parser p;
   char ch[BUFSIZE];
   int ret = 0;
   char eName[BUFSIZE];
   char eType[BUFSIZE];
   char nfcCode[BUFSIZE];
   char nfCode[BUFSIZE];
   char prio[BUFSIZE];
   char reportEId[BUFSIZE];
   char reportEName[BUFSIZE];
   char srcId[BUFSIZE];
   char srcName[BUFSIZE];
   char eCategory[BUFSIZE];
   char eventSrcTyp[BUFSIZE];
   char specProb[BUFSIZE];
   char alarmCondn[BUFSIZE];
   char eventSev[BUFSIZE];
   char vfStatus[BUFSIZE];

   int priority;
   int srcTyp;
   int vfStat;
   int eSev;

   char hostname[BUFSIZE];

   int flt_interval;
   ARRAYVAL intfArray[MAX_INTERFACES];
   KEYVALRESULT keyValResultArray[32];
   KEYVALRESULT commandArray[32];
   int numInitCommands = 0;
   int numCommands = 0;
   int linkCount = 0;
   int i = 0;

   memset(hostname, 0, BUFSIZE);
   gethostname(hostname, BUFSIZE);
   printf("FAULT01::The hostname is %s\n", hostname);

   sleep(1);
   printf("FAULT01::Running Fault thread \n");
   fflush(stdout);

   memset(&intfArray[0],0,(sizeof(ARRAYVAL) * MAX_INTERFACES));
   memset(&keyValResultArray[0],0,(sizeof(KEYVALRESULT) * 32));

   memset(js, 0, MAX_BUFFER_SIZE);
   memset(ch, 0, BUFSIZE);
   memset(&fault_intfstat[0],0,(sizeof(vpp_metrics_struct)* MAX_INTERFACES));
   memset(&fault_linkstat[0],0,(sizeof(LINKSTAT) * MAX_INTERFACES));

   FILE * file = fopen("flt_config.json", "r");

   while((fgets(ch, (BUFSIZE-1), file)) !=NULL)
   {
      strcat(js, ch);
      memset(ch, 0, BUFSIZE);
   }
//   printf("FAULT01::the file content is \n %s \n", js);

   jsmn_init(&p);
   numToken = jsmn_parse(&p, js, strlen(js), tokens, MAX_TOKENS);
   printf("FAULT01::count-%d\n", numToken);

//   printToken(js,tokens, numToken);


   ret = getIntToken(js, tokens, numToken, "tmp_faultCheckInterval", "tmp_faultInstance01", &flt_interval);
   if (ret != 0)
   {
      printf("FAULT01::The parameter tmp_faultCheckInterval is not defined, defaulted to 60 seconds\n");
      flt_interval = 60;
   }

   ret = getIntToken(js, tokens, numToken, "tmp_lowWaterMark", "tmp_faultInstance01", &lowWaterMark);
   if (ret != 0)
   {
      printf("FAULT01::The parameter tmp_lowWaterMark is not defined, defaulted to 100\n");
      lowWaterMark = 100;
   }

  ret = getArrayTokens(js, tokens, numToken, "tmp_device", "tmp_directParameters", intfArray, &linkCount);

  printf("FAULT01::Array link count is %d\n", linkCount); 

  /* Copy the link information */
  for(i=0;i<linkCount;i++)
  {
     strcpy(fault_linkstat[i].linkname, &intfArray[i]);
//     printf("FAULT01::Link name %s\n", fault_linkstat[i].linkname);
  }
  
  //read_fault_config_file();

  read_keyVal_params(js, tokens, numToken, "tmp_init", "tmp_faultInstance01", keyValResultArray, &numInitCommands);

  for(i=0;i<linkCount;i++)
  {
     memset(&commandArray[0],0,(sizeof(KEYVALRESULT) * 32));
     memcpy(commandArray, keyValResultArray, (sizeof(KEYVALRESULT) * 32)); 
     format_val_params(commandArray, numInitCommands, fault_linkstat[i].linkname, "$tmp_device");
     runCommands(commandArray, numInitCommands);
     copy_vpp_metic_data(fault_intfstat, commandArray, numInitCommands, i); 
  }

  gettimeofday(&time_val, NULL);

  sleep(flt_interval);

  /***************************************************************************/
  /* Collect metrics from the VNIC                                           */
  /***************************************************************************/
  while(1) 
  {

    ret = getIntToken(js, tokens, numToken, "tmp_faultCheckInterval", "tmp_faultInstance01", &flt_interval);
   if (ret != 0)
   {
      flt_interval = 60;
   }

    ret = getIntToken(js, tokens, numToken, "tmp_lowWaterMark", "tmp_faultInstance01", &lowWaterMark);
   if (ret != 0)
   {
      lowWaterMark = 100;
   }

   ret = getStringToken(js, tokens, numToken, "eventName", "tmp_faultInstance01", eName, BUFSIZE);
   if (ret != 0)
   {
      printf("FAULT01::Missing mandatory parameters - eventName is not there in tmp_faultInstance01. Exiting...\n");
      exit(1);
   }

   ret = getStringToken(js, tokens, numToken, "eventType", "tmp_directParameters", eType, BUFSIZE);
   ret = getStringToken(js, tokens, numToken, "nfcNamingCode", "tmp_directParameters", nfcCode, BUFSIZE);
   ret = getStringToken(js, tokens, numToken, "nfNamingCode", "tmp_directParameters", nfCode, BUFSIZE);
   ret = getStringToken(js, tokens, numToken, "reportingEntityId", "tmp_directParameters", reportEId, BUFSIZE);
   ret = getStringToken(js, tokens, numToken, "sourceId", "tmp_directParameters", srcId, BUFSIZE);
   ret = getStringToken(js, tokens, numToken, "eventCategory", "tmp_faultInstance01", eCategory, BUFSIZE);  

   ret = getStringToken(js, tokens, numToken, "eventSourceType", "tmp_faultInstance01", eventSrcTyp, BUFSIZE);
   if (ret != 0)
   {
      printf("FAULT01::Missing mandatory parameters - eventSourceType is not there in tmp_directParameters, exiting..\n");
      exit(1); 
   }
   srcTyp = get_source(eventSrcTyp);
   if(srcTyp == -1)
   {
      printf("FAULT01::Fault eventSourceType value is not matching, eventSourceType-%s \n", eventSrcTyp);
      exit(1);
   }

   ret = getStringToken(js, tokens, numToken, "vfStatus", "tmp_directParameters", vfStatus, BUFSIZE);
   if (ret != 0)
   {
      printf("FAULT01::Missing mandatory parameters - vfStatus is not there in tmp_directParameters, exiting..\n");
      exit(1); 
   }
   vfStat = get_vf_status(vfStatus);
   if(vfStat == -1)
   {
      printf("FAULT01::Fault vfStatus value is not matching, vfStatus-%s \n", vfStatus);
      exit(1);
   }

   read_keyVal_params(js, tokens, numToken, "tmp_command", "tmp_faultInstance01", keyValResultArray, &numCommands);

   for(i=0;i<linkCount;i++)
   {
       memset(&commandArray[0],0,(sizeof(KEYVALRESULT) * 32));
       memcpy(commandArray, keyValResultArray, (sizeof(KEYVALRESULT) * 32)); 
       format_val_params(commandArray, numCommands, fault_linkstat[i].linkname, "$tmp_device");
       runCommands(commandArray, numCommands);
       copy_vpp_metic_data(fault_intfstat, commandArray, numInitCommands, i); 
   }

   for (int i = 0; i < linkCount; i++)
   {
      if(fault_intfstat[i].curr_bytes_in - fault_intfstat[i].last_bytes_in > 0) {
        bytes_in = fault_intfstat[i].curr_bytes_in - fault_intfstat[i].last_bytes_in;
      }
      else {
        bytes_in = 0;
      }
      if(fault_intfstat[i].curr_bytes_out - fault_intfstat[i].last_bytes_out > 0) {
        bytes_out = fault_intfstat[i].curr_bytes_out - fault_intfstat[i].last_bytes_out;
      }
      else {
        bytes_out = 0;
      }
      if(fault_intfstat[i].curr_packets_in - fault_intfstat[i].last_packets_in > 0) {
        packets_in = fault_intfstat[i].curr_packets_in - fault_intfstat[i].last_packets_in;
      }
      else {
        packets_in = 0;
      }
      if(fault_intfstat[i].curr_packets_out - fault_intfstat[i].last_packets_out > 0) {
        packets_out = fault_intfstat[i].curr_packets_out - fault_intfstat[i].last_packets_out;
      }
      else {
        packets_out = 0;
      }
      if (((bytes_in < lowWaterMark) || (bytes_out < lowWaterMark) || 
          (packets_in < lowWaterMark) || (packets_out < lowWaterMark)) &&
          (fault_linkstat[i].fault_raised == 0)) 
      {
        printf("\n%d - bytes in %d, ouot %d, packets in %d, out %d", i, bytes_in, bytes_out, packets_in, packets_out);
        printf("\nFAULT01::Raising fault\n");
        memset(event_id, 0, BUFSIZE);
        fault_event_id = fault_event_id+1;
        sprintf(event_id2, "%09d", fault_event_id);
        strcat(event_id, event_id1);
        strcat(event_id, event_id2);

        ret = getIntToken(js, tokens, numToken, "tmp_faultCheckInterval", "tmp_faultInstance01", &flt_interval);
        if (ret != 0)
        {
           printf("FAULT01::The parameter tmp_faultCheckInterval is not defined, defaulted to 60 seconds\n");
           flt_interval = 60;
        }

        ret = getIntToken(js, tokens, numToken, "tmp_lowWaterMark", "tmp_faultInstance01", &lowWaterMark);
        if (ret != 0)
        {
           printf("FAULT01::The parameter tmp_lowWaterMark is not defined, defaulted to 100\n");
           lowWaterMark = 100;
        }
        ret = getStringToken(js, tokens, numToken, "reportingEntityName", "tmp_directParameters", reportEName, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT01::Missing mandatory parameters - reportingEntityName is not there in tmp_directParameters\n");
           printf("FAULT01::Defaulting reportingEntityName to hostname\n");
           strcpy(reportEName, hostname);
        }
        ret = getStringToken(js, tokens, numToken, "sourceName", "tmp_directParameters", srcName, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT01::Missing mandatory parameters - sourceName is not there in tmp_directParameters\n");
           printf("FAULT01::Defaulting sourceName to hostname\n");
           strcpy(srcName, hostname);
        }

        ret = getStringToken(js, tokens, numToken, "priority", "tmp_directParameters", prio, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT01::Missing mandatory parameters - priority is not there in tmp_directParameters\nDefaulting priority to Low\n");
           strcpy(prio, "Medium");
        }
        priority = get_priority(prio);
        if(priority == -1)
        {
           printf("FAULT01::Fault priority value is not matching, prioirty-%s \n", prio);
           exit(1);
        }

        ret = getStringTokenV2(js, tokens, numToken, "specificProblem", "tmp_alarmSetParameters", "tmp_faultInstance01", specProb, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT01::Missing mandatory parameters - specificProblem is not there in tmp_alarmSetParameters, exiting ...\n");
           exit(1);
        }
        ret = getStringTokenV2(js, tokens, numToken, "alarmCondition", "tmp_alarmSetParameters", "tmp_faultInstance01", alarmCondn, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT01::Missing mandatory parameters - alarmCondition is not there in tmp_alarmSetParameters, exiting ...\n");
           exit(1);
        }
        ret = getStringTokenV2(js, tokens, numToken, "eventSeverity", "tmp_alarmSetParameters", "tmp_faultInstance01", eventSev, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT01::Missing mandatory parameters - eventSeverity is not there in tmp_alarmSetParameters\n");
           printf("FAULT01::Defaulting eventSeverity to MAJOR\n");
           strcpy(eventSev, "MAJOR");
        }
        eSev = get_severity(eventSev);
        if(eSev == -1)
        {
           printf("FAULT01::Fault eventSeverity value is not matching, eventSeverity-%s \n", eventSev);
           exit(1);
        }

        fault = evel_new_fault(eName, event_id, alarmCondn, 
                               specProb, priority, eSev, srcTyp,vfStat);
        if (fault != NULL)
        {
            fault_linkstat[i].fault_raised = 1;

            struct timeval tv_now;
            gettimeofday(&tv_now, NULL);
            epoch_now = tv_now.tv_usec + 1000000 * tv_now.tv_sec;
            fault_linkstat[i].last_epoch = epoch_now;
  
            fault_header = (EVENT_HEADER *)fault;
  
            evel_fault_category_set(fault, eCategory);
            evel_fault_interface_set(fault, fault_linkstat[i].linkname);
    
            if (eType != NULL)
                evel_fault_type_set(fault, eType); 
      
            evel_start_epoch_set(&fault->header, epoch_now);
            evel_last_epoch_set(&fault->header, epoch_now);
            if(nfcCode != NULL)
                evel_nfcnamingcode_set(&fault->header, nfcCode);
            if(nfCode != NULL)
                evel_nfnamingcode_set(&fault->header, nfCode);
            evel_reporting_entity_name_set(&fault->header, reportEName);
            if(reportEId != NULL)
                evel_reporting_entity_id_set(&fault->header, reportEId);
            if(srcId != NULL )
                evel_source_id_set(&fault->header, srcId);
            if(srcName!= NULL )
                evel_source_name_set(&fault->header, srcName);
    
            evel_rc = evel_post_event(fault_header);

            if(evel_rc == EVEL_SUCCESS)
                printf("FAULT01::Fault event is correctly sent to the collector!\n");
            else
                printf("FAULT01::Post failed %d (%s)\n", evel_rc, evel_error_string());
        }
        else
        {
           printf("FAULT01::New new fault failed (%s)\n", evel_error_string());
        }
      }
      else if (((bytes_in > lowWaterMark) && (bytes_out > lowWaterMark) && 
          (packets_in > lowWaterMark) && (packets_out > lowWaterMark)) &&
          (fault_linkstat[i].fault_raised == 1)) 
      {
         printf("\nFAULT01:: Clearing fault\n");
         memset(event_id, 0, BUFSIZE);
         sprintf(event_id2, "%09d", (i+1));
         strcat(event_id, event_id1);
         strcat(event_id, event_id2);
 
         ret = getIntToken(js, tokens, numToken, "tmp_faultCheckInterval", "tmp_faultInstance01", &flt_interval);
         if (ret != 0)
         {
            printf("FAULT01::The parameter tmp_faultCheckInterval is not defined, defaulted to 60 seconds\n");
            flt_interval = 60;
         }

         ret = getIntToken(js, tokens, numToken, "tmp_lowWaterMark", "tmp_faultInstance01", &lowWaterMark);
         if (ret != 0)
         {
           printf("FAULT01::The parameter tmp_lowWaterMark is not defined, defaulted to 100\n");
           lowWaterMark = 100;
         }
         ret = getStringToken(js, tokens, numToken, "reportingEntityName", "tmp_directParameters", reportEName, BUFSIZE);
         if (ret != 0)
         {
            printf("FAULT01::Missing mandatory parameters - reportingEntityName is not there in tmp_directParameters\n");
            printf("FAULT01::Defaulting reportingEntityName to hostname\n");
            strcpy(reportEName, hostname);
         }
         ret = getStringToken(js, tokens, numToken, "sourceName", "tmp_directParameters", srcName, BUFSIZE);
         if (ret != 0)
         {
            printf("FAULT01::Missing mandatory parameters - sourceName is not there in tmp_directParameters\n");
            printf("FAULT01::Defaulting sourceName to hostname\n");
            strcpy(srcName, hostname);
         }
 
         ret = getStringToken(js, tokens, numToken, "priority", "tmp_directParameters", prio, BUFSIZE);
         if (ret != 0)
         {
            printf("FAULT01::Missing mandatory parameters - priority is not there in tmp_directParameters\nDefaulting priority to Low\n");
            strcpy(prio, "Medium");
         }
         priority = get_priority(prio);
         if(priority == -1)
         {
            printf("FAULT01::Fault priority value is not matching, prioirty-%s \n", prio);
            exit(1);
         }
 
         ret = getStringTokenV2(js, tokens, numToken, "specificProblem", "tmp_alarmClearParameters", "tmp_faultInstance01", specProb, BUFSIZE);
         if (ret != 0)
         {
            printf("FAULT01::Missing mandatory parameters - specificProblem is not there in tmp_alarmClearParameters, exiting ...\n");
            exit(1);
         }
 
         ret = getStringTokenV2(js, tokens, numToken, "alarmCondition", "tmp_alarmClearParameters", "tmp_faultInstance01", alarmCondn, BUFSIZE);
         if (ret != 0)
         {
            printf("FAULT01::Missing mandatory parameters - alarmCondition is not there in tmp_alarmClearParameters, exiting ...\n");
            exit(1);
         }
 
         ret = getStringTokenV2(js, tokens, numToken, "eventSeverity", "tmp_alarmClearParameters", "tmp_faultInstance01", eventSev, BUFSIZE);
         if (ret != 0)
         {
            printf("FAULT01::Missing mandatory parameters - eventSeverity is not there in tmp_alarmClearParameters\n");
            printf("FAULT01::Defaulting eventSeverity to MAJOR\n");
            strcpy(eventSev, "NORMAL");
         }
         eSev = get_severity(eventSev);
         if(eSev == -1)
         {
            printf("FAULT01::Fault eventSeverity value is not matching, eventSeverity-%s \n", eventSev);
            exit(1);
         }
 
         fault = evel_new_fault(eName, event_id, alarmCondn, 
                               specProb, priority, eSev, srcTyp,vfStat);
         if (fault != NULL)
         {
            fault_linkstat[i].fault_raised = 0;

            struct timeval tv_now;
            gettimeofday(&tv_now, NULL);
            epoch_now = tv_now.tv_usec + 1000000 * tv_now.tv_sec;
  
            fault_header = (EVENT_HEADER *)fault;
            evel_fault_category_set(fault, eCategory);
            evel_fault_interface_set(fault, fault_linkstat[i].linkname);
    
            if (eType != NULL)
              evel_fault_type_set(fault, eType); 
      
            evel_start_epoch_set(&fault->header, fault_linkstat[i].last_epoch);
            evel_last_epoch_set(&fault->header, epoch_now);
            fault_linkstat[i].last_epoch = 0;

            if(nfcCode != NULL)
              evel_nfcnamingcode_set(&fault->header, nfcCode);
            if(nfCode != NULL)
              evel_nfnamingcode_set(&fault->header, nfCode);
            evel_reporting_entity_name_set(&fault->header, reportEName);
            if(reportEId != NULL)
              evel_reporting_entity_id_set(&fault->header, reportEId);
            if(srcId != NULL )
            evel_source_id_set(&fault->header, srcId);
            if(srcName!= NULL )
              evel_source_name_set(&fault->header, srcName);
      
            evel_rc = evel_post_event(fault_header);
  
            if(evel_rc == EVEL_SUCCESS)
              printf("FAULT01::Fault event is correctly sent to the collector!\n");
            else
              printf("FAULT01::Post failed %d (%s)\n", evel_rc, evel_error_string());
         }
         else 
           printf("FAULT01::New fault failed (%s)\n", evel_error_string());
      }
   }

   sleep(flt_interval);
  }

  /***************************************************************************/
  /* Terminate                                                               */
  /***************************************************************************/
  sleep(1);
}

void *FaultThread02(void *faultInstanceTag)
{
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;
  EVENT_FAULT * fault = NULL;
  EVENT_HEADER* fault_header = NULL;
  unsigned long long epoch_now;
  unsigned long long last_epoch;

  struct timeval time_val;
  char event_id1[10] = "fault";
  char event_id2[15] = {0};
  char event_id[BUFSIZE] = {0};
  int fault_event_id = 0;
  int i=0;

   int numToken;
   jsmntok_t tokens[MAX_TOKENS];
   char js[MAX_BUFFER_SIZE]; 

   jsmn_parser p;
   char ch[BUFSIZE];
   int ret = 0;
   char eName[BUFSIZE];
   char eType[BUFSIZE];
   char aInterface[BUFSIZE];
   char nfcCode[BUFSIZE];
   char nfCode[BUFSIZE];
   char prio[BUFSIZE];
   char reportEId[BUFSIZE];
   char reportEName[BUFSIZE];
   char srcId[BUFSIZE];
   char srcName[BUFSIZE];
   char eCategory[BUFSIZE];
   char eventSrcTyp[BUFSIZE];
   char specProb[BUFSIZE];
   char alarmCondn[BUFSIZE];
   char eventSev[BUFSIZE];
   char vfStatus[BUFSIZE];

   int priority;
   int srcTyp;
   int vfStat;
   int eSev;

   char hostname[BUFSIZE];

   int flt_interval;
   KEYVALRESULT keyValResultArray[32];
   KEYVALRESULT commandArray[32];
   int numInitCommands = 0;
   int numCommands = 0;
   int fault_raised = 0;

   memset(hostname, 0, BUFSIZE);
   gethostname(hostname, BUFSIZE);
   printf("FAULT02::The hostname is %s\n", hostname);

   sleep(1);
   printf("FAULT02::Running Fault thread \n");
   fflush(stdout);

   memset(&keyValResultArray[0],0,(sizeof(KEYVALRESULT) * 32));

   memset(js, 0, MAX_BUFFER_SIZE);
   memset(ch, 0, BUFSIZE);

   FILE * file = fopen("flt_config.json", "r");

   while((fgets(ch, (BUFSIZE-1), file)) !=NULL)
   {
      strcat(js, ch);
      memset(ch, 0, BUFSIZE);
   }
//   printf("FAULT02::the file content is \n %s \n", js);

   jsmn_init(&p);
   numToken = jsmn_parse(&p, js, strlen(js), tokens, MAX_TOKENS);
   printf("FAULT02::count-%d\n", numToken);

//   printToken(js,tokens, numToken);

   ret = getIntToken(js, tokens, numToken, "tmp_faultCheckInterval", "tmp_faultInstance02", &flt_interval);
   if (ret != 0)
   {
      printf("FAULT02::The parameter tmp_faultCheckInterval is not defined, defaulted to 60 seconds\n");
      flt_interval = 60;
   }
  

  read_keyVal_params(js, tokens, numToken, "tmp_init", "tmp_faultInstance02", keyValResultArray, &numInitCommands);

  memset(&commandArray[0],0,(sizeof(KEYVALRESULT) * 32));
  memcpy(commandArray, keyValResultArray, (sizeof(KEYVALRESULT) * 32)); 
  runCommands(commandArray, numInitCommands);

  gettimeofday(&time_val, NULL);

  sleep(flt_interval);

  /***************************************************************************/
  /* Collect metrics from the VNIC                                           */
  /***************************************************************************/
  while(1) {

   ret = getIntToken(js, tokens, numToken, "tmp_faultCheckInterval", "tmp_faultInstance02", &flt_interval);
   if (ret != 0)
   {
      flt_interval = 60;
   }

   ret = getStringToken(js, tokens, numToken, "eventName", "tmp_faultInstance02", eName, BUFSIZE);
   if (ret != 0)
   {
      printf("FAULT02::Missing mandatory parameters - eventName is not there in tmp_faultInstance02. Exiting...\n");
      exit(1);
   }

   ret = getStringToken(js, tokens, numToken, "eventType", "tmp_directParameters", eType, BUFSIZE);
   ret = getStringToken(js, tokens, numToken, "nfcNamingCode", "tmp_directParameters", nfcCode, BUFSIZE);
   ret = getStringToken(js, tokens, numToken, "nfNamingCode", "tmp_directParameters", nfCode, BUFSIZE);
   ret = getStringToken(js, tokens, numToken, "reportingEntityId", "tmp_directParameters", reportEId, BUFSIZE);
   ret = getStringToken(js, tokens, numToken, "sourceId", "tmp_directParameters", srcId, BUFSIZE);
   ret = getStringToken(js, tokens, numToken, "eventCategory", "tmp_faultInstance02", eCategory, BUFSIZE);  
   ret = getStringToken(js, tokens, numToken, "alarmInterfaceA", "tmp_faultInstance02", aInterface, BUFSIZE);  

   ret = getStringToken(js, tokens, numToken, "eventSourceType", "tmp_faultInstance02", eventSrcTyp, BUFSIZE);
   if (ret != 0)
   {
      printf("FAULT02::Missing mandatory parameters - eventSourceType is not there in tmp_directParameters, exiting..\n");
      exit(1); 
   }
   srcTyp = get_source(eventSrcTyp);
   if(srcTyp == -1)
   {
      printf("FAULT02::Fault eventSourceType value is not matching, eventSourceType-%s \n", eventSrcTyp);
      exit(1);
   }

   ret = getStringToken(js, tokens, numToken, "vfStatus", "tmp_directParameters", vfStatus, BUFSIZE);
   if (ret != 0)
   {
      printf("FAULT02::Missing mandatory parameters - vfStatus is not there in tmp_directParameters, exiting..\n");
      exit(1); 
   }
   vfStat = get_vf_status(vfStatus);
   if(vfStat == -1)
   {
      printf("FAULT02::Fault vfStatus value is not matching, vfStatus-%s \n", vfStatus);
      exit(1);
   }

   read_keyVal_params(js, tokens, numToken, "tmp_command", "tmp_faultInstance02", keyValResultArray, &numCommands);

   memset(&commandArray[0],0,(sizeof(KEYVALRESULT) * 32));
   memcpy(commandArray, keyValResultArray, (sizeof(KEYVALRESULT) * 32)); 
   runCommands(commandArray, numCommands);

   /********************************************************************************
    * Put the condition to set the fault here
    *******************************************************************************/
   if ((atoi(commandArray[0].resultStr) == 1) && (fault_raised == 0)) 
   {
        printf("\nFAULT02::Raising fault\n");
        memset(event_id, 0, BUFSIZE);
        fault_event_id = fault_event_id+1;
        sprintf(event_id2, "%09d", fault_event_id);
        strcat(event_id, event_id1);
        strcat(event_id, event_id2);

        ret = getIntToken(js, tokens, numToken, "tmp_faultCheckInterval", "tmp_faultInstance02", &flt_interval);
        if (ret != 0)
        {
           printf("FAULT02::The parameter tmp_faultCheckInterval is not defined, defaulted to 60 seconds\n");
           flt_interval = 60;
        }

        ret = getStringToken(js, tokens, numToken, "reportingEntityName", "tmp_directParameters", reportEName, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT02::Missing mandatory parameters - reportingEntityName is not there in tmp_directParameters\n");
           printf("FAULT02::Defaulting reportingEntityName to hostname\n");
           strcpy(reportEName, hostname);
        }
        ret = getStringToken(js, tokens, numToken, "sourceName", "tmp_directParameters", srcName, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT02::Missing mandatory parameters - sourceName is not there in tmp_directParameters\n");
           printf("FAULT02::Defaulting sourceName to hostname\n");
           strcpy(srcName, hostname);
        }

        ret = getStringToken(js, tokens, numToken, "priority", "tmp_directParameters", prio, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT02::Missing mandatory parameters - priority is not there in tmp_directParameters\nDefaulting priority to Low\n");
           strcpy(prio, "Medium");
        }
        priority = get_priority(prio);
        if(priority == -1)
        {
           printf("FAULT02::Fault priority value is not matching, prioirty-%s \n", prio);
           exit(1);
        }

        ret = getStringTokenV2(js, tokens, numToken, "specificProblem", "tmp_alarmSetParameters", "tmp_faultInstance02", specProb, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT02::Missing mandatory parameters - specificProblem is not there in tmp_alarmSetParameters, exiting ...\n");
           exit(1);
        }
        ret = getStringTokenV2(js, tokens, numToken, "alarmCondition", "tmp_alarmSetParameters", "tmp_faultInstance02", alarmCondn, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT02::Missing mandatory parameters - alarmCondition is not there in tmp_alarmSetParameters, exiting ...\n");
           exit(1);
        }
        ret = getStringTokenV2(js, tokens, numToken, "eventSeverity", "tmp_alarmSetParameters", "tmp_faultInstance02", eventSev, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT02::Missing mandatory parameters - eventSeverity is not there in tmp_alarmSetParameters\n");
           printf("FAULT02::Defaulting eventSeverity to MAJOR\n");
           strcpy(eventSev, "MAJOR");
        }
        eSev = get_severity(eventSev);
        if(eSev == -1)
        {
           printf("FAULT02::Fault eventSeverity value is not matching, eventSeverity-%s \n", eventSev);
           exit(1);
        }

        fault = evel_new_fault(eName, event_id, alarmCondn, 
                               specProb, priority, eSev, srcTyp,vfStat);
        if (fault != NULL)
        {
          fault_raised = 1;

          struct timeval tv_now;
          gettimeofday(&tv_now, NULL);
          epoch_now = tv_now.tv_usec + 1000000 * tv_now.tv_sec;
          last_epoch = epoch_now;

          fault_header = (EVENT_HEADER *)fault;

          evel_fault_category_set(fault, eCategory);
          if (aInterface != NULL)
            evel_fault_interface_set(fault, aInterface);
  
          if (eType != NULL)
            evel_fault_type_set(fault, eType); 
    
          evel_start_epoch_set(&fault->header, epoch_now);
          evel_last_epoch_set(&fault->header, epoch_now);
          if(nfcCode != NULL)
            evel_nfcnamingcode_set(&fault->header, nfcCode);
          if(nfCode != NULL)
            evel_nfnamingcode_set(&fault->header, nfCode);
          evel_reporting_entity_name_set(&fault->header, reportEName);
          if(reportEId != NULL)
            evel_reporting_entity_id_set(&fault->header, reportEId);
          if(srcId != NULL )
          evel_source_id_set(&fault->header, srcId);
          if(srcName!= NULL )
            evel_source_name_set(&fault->header, srcName);
    
          evel_rc = evel_post_event(fault_header);

          if(evel_rc == EVEL_SUCCESS) {
            printf("FAULT02::Fault event is correctly sent to the collector!\n");
          }
          else {
            printf("FAULT02::Post failed %d (%s)\n", evel_rc, evel_error_string());
        }
      }
      else {
        printf("FAULT02::New new fault failed (%s)\n", evel_error_string());
      }
    }
    /********************************************************************************
     * Put the condition to clear the fault here
     *******************************************************************************/
    else if ((atoi(commandArray[0].resultStr) == 0) && (fault_raised == 1)) 
    {
        printf("\nFAULT02:: Clearing fault\n");
        memset(event_id, 0, BUFSIZE);
        sprintf(event_id2, "%09d", (i+1));
        strcat(event_id, event_id1);
        strcat(event_id, event_id2);

        ret = getIntToken(js, tokens, numToken, "tmp_faultCheckInterval", "tmp_faultInstance02", &flt_interval);
        if (ret != 0)
        {
           printf("FAULT02::The parameter tmp_faultCheckInterval is not defined, defaulted to 60 seconds\n");
           flt_interval = 60;
        }

        ret = getStringToken(js, tokens, numToken, "reportingEntityName", "tmp_directParameters", reportEName, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT02::Missing mandatory parameters - reportingEntityName is not there in tmp_directParameters\n");
           printf("FAULT02::Defaulting reportingEntityName to hostname\n");
           strcpy(reportEName, hostname);
        }
        ret = getStringToken(js, tokens, numToken, "sourceName", "tmp_directParameters", srcName, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT02::Missing mandatory parameters - sourceName is not there in tmp_directParameters\n");
           printf("FAULT02::Defaulting sourceName to hostname\n");
           strcpy(srcName, hostname);
        }

        ret = getStringToken(js, tokens, numToken, "priority", "tmp_directParameters", prio, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT02::Missing mandatory parameters - priority is not there in tmp_directParameters\nDefaulting priority to Low\n");
           strcpy(prio, "Medium");
        }
        priority = get_priority(prio);
        if(priority == -1)
        {
           printf("FAULT02::Fault priority value is not matching, prioirty-%s \n", prio);
           exit(1);
        }

        ret = getStringTokenV2(js, tokens, numToken, "specificProblem", "tmp_alarmClearParameters", "tmp_faultInstance02", specProb, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT02::Missing mandatory parameters - specificProblem is not there in tmp_alarmClearParameters, exiting ...\n");
           exit(1);
        }

        ret = getStringTokenV2(js, tokens, numToken, "alarmCondition", "tmp_alarmClearParameters", "tmp_faultInstance02", alarmCondn, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT02::Missing mandatory parameters - alarmCondition is not there in tmp_alarmClearParameters, exiting ...\n");
           exit(1);
        }

        ret = getStringTokenV2(js, tokens, numToken, "eventSeverity", "tmp_alarmClearParameters", "tmp_faultInstance02", eventSev, BUFSIZE);
        if (ret != 0)
        {
           printf("FAULT02::Missing mandatory parameters - eventSeverity is not there in tmp_alarmClearParameters\n");
           printf("FAULT02::Defaulting eventSeverity to MAJOR\n");
           strcpy(eventSev, "NORMAL");
        }
        eSev = get_severity(eventSev);
        if(eSev == -1)
        {
           printf("FAULT02::Fault eventSeverity value is not matching, eventSeverity-%s \n", eventSev);
           exit(1);
        }

        fault = evel_new_fault(eName, event_id, alarmCondn, 
                               specProb, priority, eSev, srcTyp,vfStat);
        if (fault != NULL)
        {
          fault_raised = 0;

          struct timeval tv_now;
          gettimeofday(&tv_now, NULL);
          epoch_now = tv_now.tv_usec + 1000000 * tv_now.tv_sec;

          fault_header = (EVENT_HEADER *)fault;
          evel_fault_category_set(fault, eCategory);
          if (aInterface != NULL)
            evel_fault_interface_set(fault, aInterface);
  
          if (eType != NULL)
            evel_fault_type_set(fault, eType); 
    
          evel_start_epoch_set(&fault->header, last_epoch);
          evel_last_epoch_set(&fault->header, epoch_now);
          last_epoch = 0;

          if(nfcCode != NULL)
            evel_nfcnamingcode_set(&fault->header, nfcCode);
          if(nfCode != NULL)
            evel_nfnamingcode_set(&fault->header, nfCode);
          evel_reporting_entity_name_set(&fault->header, reportEName);
          if(reportEId != NULL)
            evel_reporting_entity_id_set(&fault->header, reportEId);
          if(srcId != NULL )
          evel_source_id_set(&fault->header, srcId);
          if(srcName!= NULL )
            evel_source_name_set(&fault->header, srcName);
    
          evel_rc = evel_post_event(fault_header);

          if(evel_rc == EVEL_SUCCESS) {
            printf("FAULT02::Fault event is correctly sent to the collector!\n");
          }
          else {
            printf("FAULT02::Post failed %d (%s)\n", evel_rc, evel_error_string());
        }
      }
      else {
        printf("FAULT02::New fault failed (%s)\n", evel_error_string());
      }

    }

    sleep(flt_interval);
  }

  /***************************************************************************/
  /* Terminate                                                               */
  /***************************************************************************/
  sleep(1);
}

void *FaultThread03(void *faultInstanceTag)
{
  sleep(4);
  printf("FAULT03::thread created\n");
  fflush(stdout);
  while (1)
  { sleep (100); }
}

