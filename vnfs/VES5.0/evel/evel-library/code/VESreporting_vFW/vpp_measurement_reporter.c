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

void *MeasThread(void *threadarg);

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

}LINKSTAT;

vpp_metrics_struct meas_intfstat[MAX_INTERFACES];
LINKSTAT meas_linkstat[MAX_INTERFACES];

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
      strcpy(commandArray[i].resultStr, buf);

      if(pclose(fp))  {
          printf("Command not found or exited with error status\n");
          return;
      }
   }
   //for(i = 0; i < numCommands; i++)
     // printf("-%d-valstr and resultStr is- %s - %s\n", i, commandArray[i].valStr, commandArray[i].resultStr);
   printf("%d - %d - %d - %d\n", atoi(commandArray[0].resultStr), atoi(commandArray[1].resultStr), atoi(commandArray[2].resultStr), atoi(commandArray[3].resultStr));
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

/**************************************************************************//**
 * tap live cpu stats
 *****************************************************************************/
void evel_get_cpu_stats(EVENT_MEASUREMENT * measurement)
{
  FILE *fp;
  char path[1024];
  double usage=0.0;
  double idle;
  double intrpt;
  double nice;
  double softirq;
  double steal;
  double sys;
  double user;
  double wait;
  MEASUREMENT_CPU_USE *cpu_use = NULL;

  /* Open the command for reading. */
  //fp = popen("/bin/ls /etc/", "r");
  fp = popen("/usr/bin/top -bn 2 -d 0.01 | grep '^%Cpu' | tail -n 1 ", "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
    printf("%s", path+10);
    sscanf(path+10," %lf us, %lf sy,  %lf ni,  %lf id,  %lf wa,  %lf hi,  %lf si,  %lf st",
    &user,&sys,&nice,&idle,&wait,&intrpt,&softirq,&steal);
  }

  /* close */
  pclose(fp);

  cpu_use = evel_measurement_new_cpu_use_add(measurement, "cpu1", usage);
  if( cpu_use != NULL ){

  evel_measurement_cpu_use_idle_set(cpu_use,idle);
  //evel_measurement_cpu_use_interrupt_set(cpu_use,intrpt);
  //evel_measurement_cpu_use_nice_set(cpu_use,nice);
  //evel_measurement_cpu_use_softirq_set(cpu_use,softirq);
  //evel_measurement_cpu_use_steal_set(cpu_use,steal);
  evel_measurement_cpu_use_system_set(cpu_use,sys);
  evel_measurement_cpu_use_usageuser_set(cpu_use,user);
  //evel_measurement_cpu_use_wait_set(cpu_use,wait);
  //evel_measurement_cpu_use_add(measurement, "cpu2", usage,idle,intrpt,nice,softirq,steal,sys,user,wait);
  }
}

int measure_traffic()
{

  FILE *fp;
  int status;
  char count[10];
  time_t rawtime;
  struct tm * timeinfo;
  char period [21];
  char cmd [100];
  char secs [3];
  int sec;
  int request_rate=0;

  printf("Checking app traffic\n");
  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime(period,21,"%d/%b/%Y:%H:%M:",timeinfo);
  strftime(secs,3,"%S",timeinfo);
  sec = atoi(secs);
  if (sec == 0) sec = 59;
  sprintf(secs, "%02d", sec);
  strncat(period, secs, 21);
  // ....x....1....x....2.
  // 15/Oct/2016:17:51:19
  strcpy(cmd, "sudo docker logs vHello | grep -c ");
  strncat(cmd, period, 100);
  fp = popen(cmd, "r");
  if (fp == NULL) {
    EVEL_ERROR("popen failed to execute command");
  }

  if (fgets(count, 10, fp) != NULL) {
    request_rate = atoi(count);
    printf("Reporting request rate for second: %s as %d\n", period, request_rate);

    }
    else {
      EVEL_ERROR("New Measurement failed");
    }
    printf("Processed measurement\n");

  status = pclose(fp);
  if (status == -1) {
    EVEL_ERROR("pclose returned an error");
  }
  return request_rate;
}



int main(int argc, char** argv)
{
  char* fqdn = argv[1];
  int port = atoi(argv[2]);
  int i=0;
  int rc;
  pthread_attr_t attr;
  pthread_t meas_thread;
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
                     "vFirewall",     	           /* Role                  */
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
  rc = pthread_create(&meas_thread, NULL, MeasThread, &i);
  if (rc)
  {
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    exit(-1);
  }
  printf("Main:Created Meas thread \n");

  pthread_join(meas_thread, NULL);

  evel_terminate();
  printf("Terminated\n");
  return 0;
}

void *MeasThread(void *mainMeas)
{
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;
  EVENT_MEASUREMENT * vpp_m = NULL;
  MEASUREMENT_CPU_USE *cpu_use = NULL;
  EVENT_HEADER* vpp_m_header = NULL;
  int bytes_in;
  int bytes_out;
  int packets_in;
  int packets_out;
  unsigned long long epoch_now;
  int request_rate = 0;

  struct timeval time_val;
  MEASUREMENT_VNIC_PERFORMANCE * vnic_performance = NULL;
  char event_id1[10] = "mvfs";
  char event_id2[15] = {0};
  char event_id[BUFSIZE] = {0};
  int meas_event_id = 0;

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
   char cpuId[BUFSIZE];

   int priority;

   char hostname[BUFSIZE];

   int meas_interval;
   ARRAYVAL intfArray[MAX_INTERFACES];
   KEYVALRESULT keyValResultArray[32];
   KEYVALRESULT keyValResultArray2[32];
   KEYVALRESULT vnicCommandArray[32];
   KEYVALRESULT cpuUsageCommandArray[32];
   int numInitCommands = 0;
   int numVnicCommands = 0;
   int numCpuUsageCommands = 0;
   int linkCount = 0;
   double usage=0.0;
   double cpuIdle=0.0;
   double cpuSystem=0.0;
   double cpuUser=0.0;
   double intrpt;
   double nice;
   double softirq;
   double steal;
   double wait;

   int i = 0;

   memset(hostname, 0, BUFSIZE);
   gethostname(hostname, BUFSIZE);
   printf("MeasThread::The hostname is %s\n", hostname);

   sleep(1);
   printf("MeasThread::Running Meas thread \n");
   fflush(stdout);

   memset(&intfArray[0],0,(sizeof(ARRAYVAL) * MAX_INTERFACES));
   memset(&keyValResultArray[0],0,(sizeof(KEYVALRESULT) * 32));

   memset(js, 0, MAX_BUFFER_SIZE);
   memset(ch, 0, BUFSIZE);
   memset(&meas_intfstat[0],0,(sizeof(vpp_metrics_struct)* MAX_INTERFACES));
   memset(&meas_linkstat[0],0,(sizeof(LINKSTAT) * MAX_INTERFACES));

   FILE * file = fopen("meas_config.json", "r");

   while((fgets(ch, (BUFSIZE-1), file)) !=NULL)
   {
      strcat(js, ch);
      memset(ch, 0, BUFSIZE);
   }
//   printf("MeasThread::the file content is \n %s \n", js);

   jsmn_init(&p);
   numToken = jsmn_parse(&p, js, strlen(js), tokens, MAX_TOKENS);
   printf("MeasThread::count-%d\n", numToken);

//   printToken(js,tokens, numToken);


   ret = getIntToken(js, tokens, numToken, "measurementInterval", "tmp_directParameters", &meas_interval);
   if (ret != 0)
   {
      printf("MeasThread::The parameter measurementInterval is not defined, defaulted to 60 seconds\n");
      meas_interval = 60;
   }

  ret = getArrayTokens(js, tokens, numToken, "tmp_device", "tmp_directParameters", intfArray, &linkCount);

  printf("MeasThread::Array link count is %d\n", linkCount); 

  /* Copy the link information */
  for(i=0;i<linkCount;i++)
  {
     strcpy(meas_linkstat[i].linkname, &intfArray[i]);
//     printf("MeasThread::Link name %s\n", meas_linkstat[i].linkname);
  }
  

  read_keyVal_params(js, tokens, numToken, "tmp_init", "tmp_indirectParameters", keyValResultArray, &numInitCommands);

  for(i=0;i<linkCount;i++)
  {
     memset(&vnicCommandArray[0],0,(sizeof(KEYVALRESULT) * 32));
     memcpy(vnicCommandArray, keyValResultArray, (sizeof(KEYVALRESULT) * 32)); 
     format_val_params(vnicCommandArray, numInitCommands, meas_linkstat[i].linkname, "$tmp_device");
     runCommands(vnicCommandArray, numInitCommands);
     copy_vpp_metic_data(meas_intfstat, vnicCommandArray, numInitCommands, i); 
  }

  gettimeofday(&time_val, NULL);

  sleep(meas_interval);

  /***************************************************************************/
  /* Collect metrics from the VNIC                                           */
  /***************************************************************************/
  while(1) 
  {

    ret = getIntToken(js, tokens, numToken, "measurementInterval", "tmp_directParameters", &meas_interval);
   if (ret != 0)
   {
      meas_interval = 60;
   }

   ret = getStringToken(js, tokens, numToken, "eventName", "tmp_directParameters", eName, BUFSIZE);
   if (ret != 0)
   {
      printf("MeasThread::Missing mandatory parameters - eventName is not there in tmp_directParameters. Exiting...\n");
      exit(1);
   }

   ret = getStringToken(js, tokens, numToken, "eventType", "tmp_directParameters", eType, BUFSIZE);
   ret = getStringToken(js, tokens, numToken, "nfcNamingCode", "tmp_directParameters", nfcCode, BUFSIZE);
   ret = getStringToken(js, tokens, numToken, "nfNamingCode", "tmp_directParameters", nfCode, BUFSIZE);
   ret = getStringToken(js, tokens, numToken, "reportingEntityId", "tmp_directParameters", reportEId, BUFSIZE);
   ret = getStringToken(js, tokens, numToken, "sourceId", "tmp_directParameters", srcId, BUFSIZE);
   ret = getStringToken(js, tokens, numToken, "reportingEntityName", "tmp_directParameters", reportEName, BUFSIZE);
   if (ret != 0)
   {
      printf("MeasThread::Missing mandatory parameters - reportingEntityName is not there in tmp_directParameters\n");
      printf("MeasThread::Defaulting reportingEntityName to hostname\n");
      strcpy(reportEName, hostname);
   }
   ret = getStringToken(js, tokens, numToken, "sourceName", "tmp_directParameters", srcName, BUFSIZE);
   if (ret != 0)
   {
      printf("MeasThread::Missing mandatory parameters - sourceName is not there in tmp_directParameters\n");
      printf("MeasThread::Defaulting sourceName to hostname\n");
      strcpy(srcName, hostname);
   }

   ret = getStringToken(js, tokens, numToken, "priority", "tmp_directParameters", prio, BUFSIZE);
   if (ret != 0)
   {
      printf("MeasThread::Missing mandatory parameters - priority is not there in tmp_directParameters\nDefaulting priority to Low\n");
      strcpy(prio, "Medium");
   }
   priority = get_priority(prio);
   if(priority == -1)
   {
      printf("MeasThread::Meas priority value is not matching, prioirty-%s \n", prio);
      exit(1);
   }

   read_keyVal_params(js, tokens, numToken, "tmp_vnic_command", "vNicPerformance", keyValResultArray, &numVnicCommands);

   for(i=0;i<linkCount;i++)
   {
       memset(&vnicCommandArray[0],0,(sizeof(KEYVALRESULT) * 32));
       memcpy(vnicCommandArray, keyValResultArray, (sizeof(KEYVALRESULT) * 32)); 
       format_val_params(vnicCommandArray, numVnicCommands, meas_linkstat[i].linkname, "$tmp_device");
       runCommands(vnicCommandArray, numVnicCommands);
       copy_vpp_metic_data(meas_intfstat, vnicCommandArray, numVnicCommands, i); 
   }

   memset(event_id, 0, BUFSIZE);
   meas_event_id = meas_event_id+1;
   sprintf(event_id2, "%09d", meas_event_id);
   strcat(event_id, event_id1);
   strcat(event_id, event_id2);

   vpp_m = evel_new_measurement(meas_interval, eName,event_id);

   if(vpp_m != NULL)
   {
      printf("New measurement report created...\n");

      for (int i = 0; i < linkCount; i++)
      {
         if(meas_intfstat[i].curr_bytes_in - meas_intfstat[i].last_bytes_in > 0) {
           bytes_in = meas_intfstat[i].curr_bytes_in - meas_intfstat[i].last_bytes_in;
         }
         else {
           bytes_in = 0;
         }
         if(meas_intfstat[i].curr_bytes_out - meas_intfstat[i].last_bytes_out > 0) {
           bytes_out = meas_intfstat[i].curr_bytes_out - meas_intfstat[i].last_bytes_out;
         }
         else {
           bytes_out = 0;
         }
         if(meas_intfstat[i].curr_packets_in - meas_intfstat[i].last_packets_in > 0) {
           packets_in = meas_intfstat[i].curr_packets_in - meas_intfstat[i].last_packets_in;
         }
         else {
           packets_in = 0;
         }
         if(meas_intfstat[i].curr_packets_out - meas_intfstat[i].last_packets_out > 0) {
           packets_out = meas_intfstat[i].curr_packets_out - meas_intfstat[i].last_packets_out;
         }
         else {
           packets_out = 0;
         }
         vnic_performance = (MEASUREMENT_VNIC_PERFORMANCE *)evel_measurement_new_vnic_performance(meas_linkstat[i].linkname, "true");
         evel_meas_vnic_performance_add(vpp_m, vnic_performance);
         evel_vnic_performance_rx_total_pkt_delta_set(vnic_performance, packets_in);
         evel_vnic_performance_tx_total_pkt_delta_set(vnic_performance, packets_out);

         evel_vnic_performance_rx_octets_delta_set(vnic_performance, bytes_in);
         evel_vnic_performance_tx_octets_delta_set(vnic_performance, bytes_out);

         if (strcmp(meas_linkstat[i].linkname, "docker") == 0)
         {
           request_rate = measure_traffic();
         }
      }

      evel_measurement_request_rate_set(vpp_m, request_rate);

      //evel_get_cpu_stats(vpp_m);
      memset(&keyValResultArray2[0],0,(sizeof(KEYVALRESULT) * 32));
      memset(&cpuUsageCommandArray[0],0,(sizeof(KEYVALRESULT) * 32));

      ret = getStringToken(js, tokens, numToken, "cpuIdentifier", "cpuUsage", cpuId, BUFSIZE);
      if (ret != 0)
      {
         printf("MeasThread::Missing parameters - cpuIdentifier is not there in cpuUsage, default to Cpu1\n");
         strcpy(cpuId, "Cpu1");
      }
      read_keyVal_params(js, tokens, numToken, "tmp_cpuuse_command", "cpuUsage", keyValResultArray2, &numCpuUsageCommands);
      memcpy(cpuUsageCommandArray, keyValResultArray2, (sizeof(KEYVALRESULT) * 32)); 
      runCommands(cpuUsageCommandArray, numCpuUsageCommands);

      cpu_use = evel_measurement_new_cpu_use_add(vpp_m, cpuId, usage);
      if( cpu_use != NULL )
      {
/****************************
         for(i=0; i<numCpuUsageCommands; i++)
         {
            if(strcmp(cpuUsageCommandArray[i].keyStr, "tmp_cpuIdle") == 0)
            {
               cpuIdle = atof(cpuUsageCommandArray[i].resultStr);
            }
            if(strcmp(cpuUsageCommandArray[i].keyStr, "tmp_cpuUsageSystem") == 0)
            {
               cpuSystem = atof(cpuUsageCommandArray[i].resultStr);
            }
            if(strcmp(cpuUsageCommandArray[i].keyStr, "tmp_cpuUsageUser") == 0)
            {
               cpuUser = atof(cpuUsageCommandArray[i].resultStr);
            }
         } **********************/
         printf("%s", cpuUsageCommandArray[0].resultStr);
         sscanf(cpuUsageCommandArray[0].resultStr, " %lf us, %lf sy,  %lf ni,  %lf id,  %lf wa,  %lf hi,  %lf si,  %lf st", &cpuUser,&cpuSystem,&nice,&cpuIdle,&wait,&intrpt,&softirq,&steal);
printf("user - %f, system - %f, idle - %f\n", cpuUser, cpuSystem, cpuIdle);

         evel_measurement_cpu_use_idle_set(cpu_use, cpuIdle);
         evel_measurement_cpu_use_system_set(cpu_use, cpuSystem);
         evel_measurement_cpu_use_usageuser_set(cpu_use, cpuUser);
      }

      struct timeval tv_now;
      gettimeofday(&tv_now, NULL);
      epoch_now = tv_now.tv_usec + 1000000 * tv_now.tv_sec;

      vpp_m_header = (EVENT_HEADER *)vpp_m;


      if (eType != NULL)
          evel_measurement_type_set(vpp_m, eType); 

      evel_start_epoch_set(&vpp_m->header, epoch_start);
      evel_last_epoch_set(&vpp_m->header, epoch_now);
      epoch_start= epoch_now;

      if(nfcCode != NULL)
          evel_nfcnamingcode_set(&vpp_m->header, nfcCode);
      if(nfCode != NULL)
          evel_nfnamingcode_set(&vpp_m->header, nfCode);
      evel_reporting_entity_name_set(&vpp_m->header, reportEName);
      if(reportEId != NULL)
          evel_reporting_entity_id_set(&vpp_m->header, reportEId);
      if(srcId != NULL )
          evel_source_id_set(&vpp_m->header, srcId);
      if(srcName!= NULL )
          evel_source_name_set(&vpp_m->header, srcName);

      evel_rc = evel_post_event(vpp_m_header);

      if(evel_rc == EVEL_SUCCESS)
          printf("MeasThread::Meas event is correctly sent to the collector!\n");
      else
          printf("MeasThread::Post failed %d (%s)\n", evel_rc, evel_error_string());
   }
   else
   {
      printf("MeasThread::Measurement event creation failed (%s)\n", evel_error_string());
   }

   sleep(meas_interval);
  }

  /***************************************************************************/
  /* Terminate                                                               */
  /***************************************************************************/
  sleep(1);
}

