
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
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "evel.h"
#include "afx_ves_reporter.h"

int (*afxFunctions[NUM_THREADS]) (void *threadarg);

char *messages[NUM_THREADS];
char hostname[BUFSIZE];
char oam_intfaddr[BUFSIZE];
struct thread_data thread_data_array[NUM_THREADS];


void *HeartbeatAfxThread(void *threadarg)
{
   int taskid, sum;
   char *hello_msg;
   struct thread_data *my_data;
   char hrtbtevid[256];

   sleep(1);
   my_data = (struct thread_data *) threadarg;
   taskid = my_data->thread_id;
   sum = my_data->sum;
   hello_msg = my_data->message;
   printf("Thread %d: %s  Sum=%d\n", taskid, hello_msg, sum);

while(1)
{
  EVENT_HEADER * heartbeat = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  /***************************************************************************/
  /* Heartbeat                                                               */
  /***************************************************************************/
  sprintf(hrtbtevid,"Heartbeat_vAfx_%s",oam_intfaddr);
  heartbeat = evel_new_heartbeat_nameid("Heartbeat_vAfx",hrtbtevid);
  //heartbeat = evel_new_heartbeat();
  if (heartbeat != NULL)
  {
    evel_header_type_set(heartbeat, "applicationVnf");
    evel_nfcnamingcode_set(heartbeat, "AFX");
    evel_nfnamingcode_set(heartbeat, "AFX");
    evel_rc = evel_post_event(heartbeat);
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
  sleep(15);
}

   pthread_exit(NULL);
}

int checklist(char *modname, char **list, int numt)
{
 int i;
 for(i=0;i<numt;i++)
 {
   if( !strcasecmp(list[i],modname) )
	return 1;
 }
 return 0;
}


int start_threads(void)
{

pthread_t threads[NUM_THREADS];
int *taskids[NUM_THREADS];
int rc, t, sum;
char *modlist[NUM_THREADS];
int modcounter = 0;
char line[128];
char *pos;
FILE *file;

pthread_attr_t attr;

sum=0;
messages[0] = "Heartbeat started!";
messages[1] = "Link monitoring started";
messages[2] = "AFX Measurement started!";
messages[3] = "Service Monitoring started";
messages[4] = "BGP Monitoring started";

    file = fopen(AFX_MODULES_FILE, "r"); /* should check the result */

    if( file != NULL ){
      while ( file != NULL && fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
        //printf("%s", line);
        remove_spaces(line);
        if ((pos=strchr(line, '\n')) != NULL)
           *pos = '\0';
        if( modcounter >= NUM_THREADS )
        {
	   EVEL_ERROR("AFX modules file %s has more modules than allowed\n",AFX_MODULES_FILE);
	   exit(1);
	}
        modlist[modcounter] = strdup(line);
        modcounter++;
     }
     fclose(file);
   }


for(t=0;t<NUM_THREADS;t++) {
  if( file == NULL || (t == 0 && checklist("HeartBeat",modlist, modcounter)) ||
      (t == 1 && checklist("LinkMonitor",modlist, modcounter)) ||
      (t == 2 && checklist("ScalingMeasurements",modlist, modcounter)) ||
      (t == 3 && checklist("ServiceMonitor",modlist, modcounter)) ||
      (t == 4 && checklist("SyslogBgp",modlist, modcounter))
     ) 
  {
  sum = sum + t;
  thread_data_array[t].thread_id = t;
  thread_data_array[t].sum = sum;
  thread_data_array[t].message = messages[t];

  /* Initialize and set thread detached attribute */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  printf("Creating thread %d\n", t);
  rc = pthread_create(&threads[t], NULL, afxFunctions[t], (void *) 
       &thread_data_array[t]);
  if (rc) {
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    exit(-1);
    }
  }
  else threads[t] = NULL;
}

  //pthread_exit(NULL);
  for(t=0;t<NUM_THREADS;t++) {
     if( threads[t] != NULL )
       pthread_join(threads[t],NULL);
  }
  return sum;
}

void read_lpfile(char *fname, char **usn, char **pwd)
{
    FILE *fp;
    int i=0;   // count how many lines are in the file
    char line[256];
    char *pos;

    fp=fopen(fname, "r");
    while (fgets(line, sizeof(line), fp)) {
      //printf("%s", line);
      if ((pos=strchr(line, '\n')) != NULL)
           *pos = '\0';
      i++;
      if( i == 1 && strlen(line) < 64 ) *usn = strdup(line);
      if( i == 2 && strlen(line) < 64 ) *pwd = strdup(line);
    }
    fclose(fp);
}


int main(int argc, char** argv)
{
  char* fqdn = argv[1];
  int port = atoi(argv[2]);
  char *fqdn2 = NULL;
  int port2;
  char* lpfile = argv[3];
  char* lpfile2 = NULL;
  //char* usname = argv[3];
  //char* passwd = argv[4];
  char* usname = NULL;
  char* passwd = NULL;
  char* usname2 = NULL;
  char* passwd2 = NULL;
  int secty = atoi(argv[4]);
  int dbglvl = atoi(argv[5]);
  //int hrtbtintval = atoi(argv[6]);
  int rc = EVEL_SUCCESS;
  char oam_intf[64];
  char const* const fileName = "afxintf.conf";
  char line[128];
  FILE *file=NULL;
  char *pos;
  struct stat sb;


  printf("\nvAFX VES Processing (VPP) measurement collection\n");
  fflush(stdout);

if( argc == 9 )
{
  fqdn2 = argv[6];
  port2 = atoi(argv[7]);
  lpfile2 = argv[8];
}


  if ( !(argc == 6 || argc == 9 ) || port < 0 || dbglvl < 0 )
  {
    fprintf(stderr, "Usage: %s <DCAE FQDN>|<IP address> <port> <credential file> <debug level> \n", argv[0]);
    fprintf(stderr, "Or: %s <DCAE FQDN>|<IP address> <port> <credential file> <debug level> <DCAE FQDN2>|<IP address2> <port2> <credential file2> \n", argv[0]);
    exit(-1);
  }

  if( stat(lpfile,&sb)<0  )
  {
    fprintf(stderr, "Error: Invalid Login password file %s \n", lpfile);
    EVEL_ERROR("Error: Invalid Login password file %s \n", lpfile);
    exit(-1);
  }
  read_lpfile(lpfile,&usname,&passwd);
  //fprintf(stderr, "Login:%s:\n", usname);
  //fprintf(stderr, "Password:%s:\n", passwd);
  if ( usname == NULL || passwd == NULL || strlen(usname) < 5 || strlen(passwd) < 5 )
  {
    fprintf(stderr, "Error: Invalid credentials in file %s \n", lpfile);
    EVEL_ERROR("Error: Invalid credentials in file %s \n", lpfile);
    exit(-1);
  }

if( argc == 9 )
{
  if( stat(lpfile2,&sb)<0  )
  {
    fprintf(stderr, "Error: Invalid Redundant collector Login password file %s \n", lpfile2);
    EVEL_ERROR("Error: Invalid Login password file %s \n", lpfile2);
    exit(-1);
  }
  read_lpfile(lpfile2,&usname2,&passwd2);
  if ( usname2 == NULL || passwd2 == NULL || strlen(usname2) < 5 || strlen(passwd2) < 5 )
  {
    fprintf(stderr, "Error: Invalid credentials in file %s \n", lpfile2);
    EVEL_ERROR("Error: Invalid credentials in file %s \n", lpfile2);
    exit(-1);
  }
  //fprintf(stderr, "Login:%s:\n", usname2);
  //fprintf(stderr, "Password:%s:\n", passwd2);
}

  srand(time(NULL));
  gethostname(hostname, BUFSIZE);

  strcpy(oam_intf,OAM_INTERFACE);
  sprintf(oam_intfaddr,"%s",get_oam_intfaddr(oam_intf));

  /**************************************************************************/
  /* Initialize                                                             */
  /**************************************************************************/
  do {

if( argc == 6 )
{
  rc = evel_initialize(fqdn,                       /* FQDN                  */
                     port,                         /* Port                  */
                     NULL,                         /* backup fqdn         */
                     0,                            /* backup port         */
                     NULL,                         /* optional path         */
                     NULL,                         /* optional topic        */
                     1000,                         /* Ring buf size         */
                     secty,                            /* HTTPS?                */
                     NULL, /*"/home/gs244f/sslcerts/testclient.crt",*/
                     NULL, /*"/home/gs244f/sslcerts/testclient.key",*/
                     NULL, /*"/etc/pki/ca-trust/source/ca-bundle.legacy.crt",*/
                     NULL, /*"/home/gs244f/sslcerts/www.testsite.com.crt",*/
                     0, 0,
                     usname,                      /* Username              */
                     passwd,                      /* Password              */
                     NULL,                        /* Username2             */
                     NULL,                        /* Password2              */
                     NULL,                        /* source ip             */
                     NULL,                        /* backup ip              */
                     EVEL_SOURCE_VIRTUAL_MACHINE,  /* Source type           */
                     "vAFX",                    /* Role                  */
                     dbglvl);                /* Verbosity             */
   if(rc != EVEL_SUCCESS){
    fprintf(stderr, "\nFailed to initialize the EVEL library!!!\n");
    exit(-1);
   }
   else
   {
    printf("\nInitialization completed\n");
   }
} else {

  rc = evel_initialize(fqdn,          /* FQDN           */
                     port,            /* Port           */
                     fqdn2,           /* backup fqdn    */
                     port2,           /* backup port    */
                     NULL,            /* optional path  */
                     NULL,            /* optional topic */
                     1000,            /* RingB size     */
                     secty,           /* HTTPS? */
                     NULL, /*"/home/gs244f/sslcerts/testclient.crt",*/
                     NULL, /*"/home/gs244f/sslcerts/testclient.key",*/
                     NULL, /*"/etc/pki/ca-trust/source/ca-bundle.legacy.crt",*/
                     NULL, /*"/home/gs244f/sslcerts/www.testsite.com.crt",*/
                     0, 0,
                     usname,          /* Username              */
                     passwd,          /* Password              */
                     usname2,         /* Username2             */
                     passwd2,         /* Password2             */
                     NULL,            /* source ip             */
                     NULL,            /* backup ip             */
                     EVEL_SOURCE_VIRTUAL_MACHINE,  /* Source type   */
                     "vAFX",          /* Role                  */
                     dbglvl);         /* Verbosity             */
   if(rc != EVEL_SUCCESS){
    fprintf(stderr, "\nFailed to initialize the EVEL library!!!\n");
    evel_terminate();
    exit(-1);
   }
   else
   {
    printf("\nInitialization completed\n");
   }
}

  } while( rc != EVEL_SUCCESS);

  afxFunctions[0] = HeartbeatAfxThread;
  afxFunctions[1] = LinkMonitorAfxThread;
  afxFunctions[2] = MeasureAfxThread;
  afxFunctions[3] = ServiceMonitorAfxThread;
  afxFunctions[4] = BgpLoggingAfxThread;

  start_threads();

  evel_terminate();
  printf("Terminated\n");
}

