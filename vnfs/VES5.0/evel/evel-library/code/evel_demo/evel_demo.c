/**************************************************************************//**
 * @file
 * Utility providing example use of the ECOMP Vendor Event Listener API.
 *
 * This software is intended to show the essential elements of the library's
 * use.
 *
 * License
 * -------
 *
 * Copyright(c) <2016>, AT&T Intellectual Property.  All other rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:  This product includes software
 * developed by the AT&T.
 * 4. Neither the name of AT&T nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY AT&T INTELLECTUAL PROPERTY ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AT&T INTELLECTUAL PROPERTY BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/signal.h>
#include <pthread.h>
#include <mcheck.h>
#include <sys/time.h>

#include "jsmn.h"
#include "evel.h"
#include "evel_demo.h"
#include "evel_test_control.h"

/**************************************************************************//**
 * Definition of long options to the program.
 *
 * See the documentation for getopt_long() for details of the structure's use.
 *****************************************************************************/
static const struct option long_options[] = {
    {"help",     no_argument,       0, 'h'},
    {"fqdn",     required_argument, 0, 'f'},
    {"port",     required_argument, 0, 'n'},
    {"path",     required_argument, 0, 'p'},
    {"topic",    required_argument, 0, 't'},
    {"https",    no_argument,       0, 's'},
    {"verbose",  no_argument,       0, 'v'},
    {"cycles",   required_argument, 0, 'c'},
    {"username", required_argument, 0, 'u'},
    {"password", required_argument, 0, 'w'},
    {"nothrott", no_argument,       0, 'x'},
    {0, 0, 0, 0}
  };

/**************************************************************************//**
 * Definition of short options to the program.
 *****************************************************************************/
static const char* short_options = "hf:n:p:t:sc:u:w:vx";

/**************************************************************************//**
 * Basic user help text describing the usage of the application.
 *****************************************************************************/
static const char* usage_text =
"evel_demo [--help]\n"
"          --fqdn <domain>\n"
"          --port <port_number>\n"
"          [--path <path>]\n"
"          [--topic <topic>]\n"
"          [--username <username>]\n"
"          [--password <password>]\n"
"          [--https]\n"
"          [--cycles <cycles>]\n"
"          [--nothrott]\n"
"\n"
"Demonstrate use of the ECOMP Vendor Event Listener API.\n"
"\n"
"  -h         Display this usage message.\n"
"  --help\n"
"\n"
"  -f         The FQDN or IP address to the RESTful API.\n"
"  --fqdn\n"
"\n"
"  -n         The port number the RESTful API.\n"
"  --port\n"
"\n"
"  -p         The optional path prefix to the RESTful API.\n"
"  --path\n"
"\n"
"  -t         The optional topic part of the RESTful API.\n"
"  --topic\n"
"\n"
"  -u         The optional username for basic authentication of requests.\n"
"  --username\n"
"\n"
"  -w         The optional password for basic authentication of requests.\n"
"  --password\n"
"\n"
"  -s         Use HTTPS rather than HTTP for the transport.\n"
"  --https\n"
"\n"
"  -c         Loop <cycles> times round the main loop.  Default = 1.\n"
"  --cycles\n"
"\n"
"  -v         Generate much chattier logs.\n"
"  --verbose\n"
"\n"
"  -x         Exclude throttling commands from demonstration.\n"
"  --nothrott\n";

#define DEFAULT_SLEEP_SECONDS 3
#define MINIMUM_SLEEP_SECONDS 1

unsigned long long epoch_start = 0;

typedef enum {
  SERVICE_CODEC,
  SERVICE_TRANSCODING,
  SERVICE_RTCP,
  SERVICE_EOC_VQM,
  SERVICE_MARKER
} SERVICE_EVENT;

/*****************************************************************************/
/* Local prototypes.                                                         */
/*****************************************************************************/
static void demo_heartbeat(void);
static void demo_fault(void);
static void demo_measurement(const int interval);
static void demo_mobile_flow(void);
static void demo_heartbeat_field_event(void);
static void demo_signaling(void);
static void demo_state_change(void);
static void demo_syslog(void);
static void demo_other(void);
static void demo_voicequality(void);

/**************************************************************************//**
 * Global flag to initiate shutdown.
 *****************************************************************************/
static int glob_exit_now = 0;

static char * api_fqdn = NULL;
static int api_port = 0;
static int api_secure = 0;

static void show_usage(FILE* fp)
{
  fputs(usage_text, fp);
}

/**************************************************************************//**
 * Main function.
 *
 * Parses the command-line then ...
 *
 * @param[in] argc  Argument count.
 * @param[in] argv  Argument vector - for usage see usage_text.
 *****************************************************************************/
int main(int argc, char ** argv)
{
  sigset_t sig_set;
  pthread_t thread_id;
  int option_index = 0;
  int param = 0;
  char * api_path = NULL;
  char * api_topic = NULL;
  char * api_username = "";
  char * api_password = "";
  int verbose_mode = 0;
  int exclude_throttling = 0;
  int cycles = 1;
  int cycle;
  int measurement_interval = EVEL_MEASUREMENT_INTERVAL_UKNOWN;

  /***************************************************************************/
  /* We're very interested in memory management problems so check behavior.  */
  /***************************************************************************/
  mcheck(NULL);

  if (argc < 2)
  {
    show_usage(stderr);
    exit(-1);
  }
  param = getopt_long(argc, argv,
                      short_options,
                      long_options,
                      &option_index);
  while (param != -1)
  {
    switch (param)
    {
      case 'h':
        show_usage(stdout);
        exit(0);
        break;

      case 'f':
        api_fqdn = optarg;
        break;

      case 'n':
        api_port = atoi(optarg);
        break;

      case 'p':
        api_path = optarg;
        break;

      case 't':
        api_topic = optarg;
        break;

      case 'u':
        api_username = optarg;
        break;

      case 'w':
        api_password = optarg;
        break;

      case 's':
        api_secure = 1;
        break;

      case 'c':
        cycles = atoi(optarg);
        break;

      case 'v':
        verbose_mode = 1;
        break;

      case 'x':
        exclude_throttling = 1;
        break;

      case '?':
        /*********************************************************************/
        /* Unrecognized parameter - getopt_long already printed an error     */
        /* message.                                                          */
        /*********************************************************************/
        break;

      default:
        fprintf(stderr, "Code error: recognized but missing option (%d)!\n",
                param);
        exit(-1);
    }

    /*************************************************************************/
    /* Extract next parameter.                                               */
    /*************************************************************************/
    param = getopt_long(argc, argv,
                        short_options,
                        long_options,
                        &option_index);
  }

  /***************************************************************************/
  /* All the command-line has parsed cleanly, so now check that the options  */
  /* are meaningful.                                                         */
  /***************************************************************************/
  if (api_fqdn == NULL)
  {
    fprintf(stderr, "FQDN of the Vendor Event Listener API server must be "
                    "specified.\n");
    exit(1);
  }
  if (api_port <= 0 || api_port > 65535)
  {
    fprintf(stderr, "Port for the Vendor Event Listener API server must be "
                    "specified between 1 and 65535.\n");
    exit(1);
  }
  if (cycles <= 0)
  {
    fprintf(stderr, "Number of cycles around the main loop must be an"
                    "integer greater than zero.\n");
    exit(1);
  }

  /***************************************************************************/
  /* Set up default signal behaviour.  Block all signals we trap explicitly  */
  /* on the signal_watcher thread.                                           */
  /***************************************************************************/
  sigemptyset(&sig_set);
  sigaddset(&sig_set, SIGALRM);
  sigaddset(&sig_set, SIGINT);
  pthread_sigmask(SIG_BLOCK, &sig_set, NULL);

  /***************************************************************************/
  /* Start the signal watcher thread.                                        */
  /***************************************************************************/
  if (pthread_create(&thread_id, NULL, signal_watcher, &sig_set) != 0)
  {
    fprintf(stderr, "Failed to start signal watcher thread.");
    exit(1);
  }
  pthread_detach(thread_id);

  /***************************************************************************/
  /* Version info                                                            */
  /***************************************************************************/
  printf("%s built %s %s\n", argv[0], __DATE__, __TIME__);

  /***************************************************************************/
  /* Initialize the EVEL interface.                                          */
  /***************************************************************************/
  if (evel_initialize(api_fqdn,
                      api_port,
                      api_path,
                      api_topic,
                      api_secure,
                      api_username,
                      api_password,
                      EVEL_SOURCE_VIRTUAL_MACHINE,
                      "EVEL demo client",
                      verbose_mode))
  {
    fprintf(stderr, "Failed to initialize the EVEL library!!!");
    exit(-1);
  }
  else
  {
    EVEL_INFO("Initialization completed");
  }

  /***************************************************************************/
  /* Work out a start time for measurements, and sleep for initial period.   */
  /***************************************************************************/
  struct timeval tv_start;
  gettimeofday(&tv_start, NULL);
  epoch_start = tv_start.tv_usec + 1000000 * tv_start.tv_sec;
  sleep(DEFAULT_SLEEP_SECONDS);

  /***************************************************************************/
  /* MAIN LOOP                                                               */
  /***************************************************************************/
  printf("Starting %d loops...\n", cycles);
  cycle = 0;
  while (cycle++ < cycles)
  {
    EVEL_INFO("MAI: Starting main loop");
    printf("\nStarting main loop %d\n", cycle);

    /*************************************************************************/
    /* A 20s-long repeating cycle of behaviour.                              */
    /*************************************************************************/
    if (exclude_throttling == 0)
    {
      switch (cycle % 20)
      {
        case 1:
          printf("   1 - Resetting throttle specification for all domains\n");
          evel_test_control_scenario(TC_RESET_ALL_DOMAINS,
                                     api_secure,
                                     api_fqdn,
                                     api_port);
          break;

        case 2:
          printf("   2 - Switching measurement interval to 2s\n");
          evel_test_control_meas_interval(2,
                                          api_secure,
                                          api_fqdn,
                                          api_port);
          break;

        case 3:
          printf("   3 - Suppressing fault domain\n");
          evel_test_control_scenario(TC_FAULT_SUPPRESS_FIELDS_AND_PAIRS,
                                     api_secure,
                                     api_fqdn,
                                     api_port);
          break;

        case 4:
          printf("   4 - Suppressing measurement domain\n");
          evel_test_control_scenario(TC_MEAS_SUPPRESS_FIELDS_AND_PAIRS,
                                     api_secure,
                                     api_fqdn,
                                     api_port);
          break;

        case 5:
          printf("   5 - Switching measurement interval to 5s\n");
          evel_test_control_meas_interval(5,
                                          api_secure,
                                          api_fqdn,
                                          api_port);
          break;

        case 6:
          printf("   6 - Suppressing mobile flow domain\n");
          evel_test_control_scenario(TC_MOBILE_SUPPRESS_FIELDS_AND_PAIRS,
                                     api_secure,
                                     api_fqdn,
                                     api_port);
          break;

        case 7:
          printf("   7 - Suppressing state change domain\n");
          evel_test_control_scenario(TC_STATE_SUPPRESS_FIELDS_AND_PAIRS,
                                     api_secure,
                                     api_fqdn,
                                     api_port);
          break;

        case 8:
          printf("   8 - Suppressing signaling domain\n");
          evel_test_control_scenario(TC_SIGNALING_SUPPRESS_FIELDS,
                                     api_secure,
                                     api_fqdn,
                                     api_port);
          break;

        case 9:
          printf("   9 - Suppressing service event domain\n");
          evel_test_control_scenario(TC_SERVICE_SUPPRESS_FIELDS_AND_PAIRS,
                                     api_secure,
                                     api_fqdn,
                                     api_port);
          break;

        case 10:
          printf("   10 - Switching measurement interval to 20s\n");
          evel_test_control_meas_interval(20,
                                          api_secure,
                                          api_fqdn,
                                          api_port);
          break;

        case 11:
          printf("   11 - Suppressing syslog domain\n");
          evel_test_control_scenario(TC_SYSLOG_SUPPRESS_FIELDS_AND_PAIRS,
                                     api_secure,
                                     api_fqdn,
                                     api_port);
          break;

        case 12:
          printf("   12 - Switching measurement interval to 10s\n");
          evel_test_control_meas_interval(10,
                                          api_secure,
                                          api_fqdn,
                                          api_port);
          break;

        case 15:
          printf("   Requesting provide throttling spec\n");
          evel_test_control_scenario(TC_PROVIDE_THROTTLING_SPEC,
                                     api_secure,
                                     api_fqdn,
                                     api_port);
          break;
      }
    }
    fflush(stdout);

    /*************************************************************************/
    /* Send a bunch of events.                                               */
    /*************************************************************************/
    demo_heartbeat();
    demo_fault();
    demo_measurement((measurement_interval ==
                                            EVEL_MEASUREMENT_INTERVAL_UKNOWN) ?
                     DEFAULT_SLEEP_SECONDS : measurement_interval);
    demo_mobile_flow();
    demo_heartbeat_field_event();
    demo_signaling();
    demo_state_change();
    demo_syslog();
    demo_other();
    demo_voicequality();

    /*************************************************************************/
    /* MAIN RETRY LOOP.  Check and implement the measurement interval.       */
    /*************************************************************************/
    if (cycle <= cycles)
    {
      int sleep_time;

      /***********************************************************************/
      /* We have a minimum loop time.                                        */
      /***********************************************************************/
      sleep(MINIMUM_SLEEP_SECONDS);

      /***********************************************************************/
      /* Get the latest measurement interval and sleep for the remainder.    */
      /***********************************************************************/
      measurement_interval = evel_get_measurement_interval();
      printf("Measurement Interval = %d\n", measurement_interval);

      if (measurement_interval == EVEL_MEASUREMENT_INTERVAL_UKNOWN)
      {
        sleep_time = DEFAULT_SLEEP_SECONDS - MINIMUM_SLEEP_SECONDS;
      }
      else
      {
        sleep_time = measurement_interval - MINIMUM_SLEEP_SECONDS;
      }
      sleep(sleep_time);
    }
  }

  /***************************************************************************/
  /* We are exiting, but allow the final set of events to be dispatched      */
  /* properly first.                                                         */
  /***************************************************************************/
  sleep(2);
  printf("All done - exiting!\n");
  return 0;
}

/**************************************************************************//**
 * Signal watcher.
 *
 * Signal catcher for incoming signal processing.  Work out which signal has
 * been received and process it accordingly.
 *
 * param[in]  void_sig_set  The signal mask to listen for.
 *****************************************************************************/
void *signal_watcher(void *void_sig_set)
{
  sigset_t *sig_set = (sigset_t *)void_sig_set;
  int sig = 0;
  int old_type = 0;
  siginfo_t sig_info;

  /***************************************************************************/
  /* Set this thread to be cancellable immediately.                          */
  /***************************************************************************/
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old_type);

  while (!glob_exit_now)
  {
    /*************************************************************************/
    /* Wait for a signal to be received.                                     */
    /*************************************************************************/
    sig = sigwaitinfo(sig_set, &sig_info);
    switch (sig)
    {
      case SIGALRM:
        /*********************************************************************/
        /* Failed to do something in the given amount of time.  Exit.        */
        /*********************************************************************/
        EVEL_ERROR( "Timeout alarm");
        fprintf(stderr,"Timeout alarm - quitting!\n");
        exit(2);
        break;

      case SIGINT:
        EVEL_INFO( "Interrupted - quitting");
        printf("\n\nInterrupted - quitting!\n");
        glob_exit_now = 1;
        break;
    }
  }

  evel_terminate();
  exit(0);
  return(NULL);
}

/**************************************************************************//**
 * Create and send a heartbeat event.
 *****************************************************************************/
void demo_heartbeat(void)
{
  EVENT_HEADER * heartbeat = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  /***************************************************************************/
  /* Heartbeat                                                               */
  /***************************************************************************/
  heartbeat = evel_new_heartbeat();
  if (heartbeat != NULL)
  {
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
}

/**************************************************************************//**
 * tap live cpu stats
 *****************************************************************************/
void evel_get_cpu_stats(EVENT_MEASUREMENT * measurement)
{
  FILE *fp;
  char path[1024];
  double usage;
  double idle;
  double intrpt;
  double nice;
  double softirq;
  double steal;
  double sys;
  double user;
  double wait;
  MEASUREMENT_CPU_USE *cpu_use;

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
  evel_measurement_cpu_use_idle_set(cpu_use,idle);
  evel_measurement_cpu_use_interrupt_set(cpu_use,intrpt);
  evel_measurement_cpu_use_nice_set(cpu_use,nice);
  evel_measurement_cpu_use_softirq_set(cpu_use,softirq);
  evel_measurement_cpu_use_steal_set(cpu_use,steal);
  evel_measurement_cpu_use_system_set(cpu_use,sys);
  evel_measurement_cpu_use_usageuser_set(cpu_use,user);
  evel_measurement_cpu_use_wait_set(cpu_use,wait);
  //evel_measurement_cpu_use_add(measurement, "cpu2", usage,idle,intrpt,nice,softirq,steal,sys,user,wait);
}

/**************************************************************************//**
 * tap live disk stats
 *****************************************************************************/
void evel_get_disk_stats(EVENT_MEASUREMENT * measurement)
{
  FILE *fp;
  char path[1024];
  double rrqm;
  double wrqm;
  double rs;
  double ws;
  double rkbs;
  double wkbs;
  double avgrqs;
  double avgqqs;
  double wait;
  double rawait;
  double wawait;
  double svctm;
  double util;
  MEASUREMENT_DISK_USE * disk_use = NULL;

  /* Open the command for reading. */
  //fp = popen("/bin/ls /etc/", "r");
  fp = popen("/usr/bin/iostat -xtd | grep '^sda' | tail -n 1 ", "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
    printf("%s", path+10);
    sscanf(path+10,"%lf    %lf    %lf    %lf    %lf    %lf    %lf    %lf    %lf    %lf    %lf    %lf    %lf",
    &rrqm,&wrqm,&rs,&ws,&rkbs,&wkbs,&avgrqs,&avgqqs,&wait,&rawait,&wawait,&svctm,&util);
  }

  /* close */
  pclose(fp);

  disk_use = evel_measurement_new_disk_use_add(measurement, "sda");
  evel_measurement_disk_use_iotimeavg_set(disk_use, rrqm);
  evel_measurement_disk_use_iotimelast_set(disk_use, wrqm);
  evel_measurement_disk_use_iotimemax_set(disk_use, rs);
  evel_measurement_disk_use_iotimemin_set(disk_use, ws);
  evel_measurement_disk_use_mergereadavg_set(disk_use, rkbs);
  evel_measurement_disk_use_mergereadlast_set(disk_use, wkbs);
  evel_measurement_disk_use_mergereadmax_set(disk_use, avgrqs);
  evel_measurement_disk_use_mergereadmin_set(disk_use, avgqqs);
  evel_measurement_disk_use_mergewritelast_set(disk_use, wait);
  evel_measurement_disk_use_mergewritemax_set(disk_use, rawait);
  evel_measurement_disk_use_mergewritemin_set(disk_use, wawait);
  evel_measurement_disk_use_octetsreadavg_set(disk_use, svctm);
  evel_measurement_disk_use_octetsreadlast_set(disk_use, util);
}


/**************************************************************************//**
 * tap live memory stats
 *****************************************************************************/
void evel_get_mem_stats(EVENT_MEASUREMENT * measurement)
{
char buffer[4096];
char line[100];
char c;
int  bcount=0,lcount=0;

double membuffsz=0.0;
double memcache=0.0;
double memconfig=0.0;
double memfree=0.0;
double slab=0.0;
double slabrecl=0.0;
double slabunrecl=0.0;
double memused=0.0;
MEASUREMENT_MEM_USE *mem_use = NULL;


FILE * filp = fopen("/proc/meminfo", "rb");
int bytes_read = fread(buffer, sizeof(char), 4096, filp);
fclose(filp);

printf("meminfo %d\n",bytes_read);

while ( bcount < bytes_read )
{
  for(lcount=0; buffer[bcount] != '\n';bcount++,lcount++)
  {
     line[lcount] = buffer[bcount];
  }
  if( lcount > 0 )
  {
    line[lcount] = '\0';
    //printf("%s\n",line);
    if(!strncmp(line,"Buffers:", strlen("Buffers:")))
    {
        sscanf(line+strlen("Buffers:"),"%lf",&membuffsz);
        //printf("membuff %lf\n",membuffsz);
    }
    else if(!strncmp(line,"Cached:", strlen("Cached:")))
    {
        sscanf(line+strlen("Cached:"),"%lf",&memcache);
    }
    else if(!strncmp(line,"MemTotal:", strlen("MemTotal:")))
    {
        sscanf(line+strlen("MemTotal:"),"%lf",&memconfig);
    }
    else if(!strncmp(line,"MemFree:", strlen("MemFree:")))
    {
        sscanf(line+strlen("MemFree:"),"%lf",&memfree);
    }
    else if(!strncmp(line,"Slab:", strlen("Slab:")))
    {
        sscanf(line+strlen("Slab:"),"%lf",&slab);
    }
    else if(!strncmp(line,"SReclaimable:", strlen("SReclaimable:")))
    {
        sscanf(line+strlen("SReclaimable:"),"%lf",&slabrecl);
    }
    else if(!strncmp(line,"SUnreclaim:", strlen("SUnreclaim:")))
    {
        sscanf(line+strlen("SUnreclaim:"),"%lf",&slabunrecl);
    }
  }
  bcount++;
}

memused = memconfig - memfree - membuffsz - memcache - slab;
printf("memused %lf\n",memused);

  mem_use = evel_measurement_new_mem_use_add(measurement, "RAM", "vm1", membuffsz);
  evel_measurement_mem_use_memcache_set(mem_use,memcache);
  evel_measurement_mem_use_memconfig_set(mem_use,memconfig);
  evel_measurement_mem_use_memfree_set(mem_use,memfree);
  evel_measurement_mem_use_slab_reclaimed_set(mem_use,slabrecl);
  evel_measurement_mem_use_slab_unreclaimable_set(mem_use,slabunrecl);
  evel_measurement_mem_use_usedup_set(mem_use,memused);

}

/**************************************************************************//**
 * Create and send three fault events.
 *****************************************************************************/
void demo_fault(void)
{
  EVENT_FAULT * fault = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  /***************************************************************************/
  /* Fault                                                                   */
  /***************************************************************************/
  fault = evel_new_fault("An alarm condition",
                         "Things are broken",
                         EVEL_PRIORITY_NORMAL,
                         EVEL_SEVERITY_MAJOR,
			 EVEL_SOURCE_VIRTUAL_MACHINE,
			 EVEL_VF_STATUS_READY_TERMINATE);
  if (fault != NULL)
  {
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
  printf("   Processed empty Fault\n");

  fault = evel_new_fault("Another alarm condition",
                         "It broke badly",
                         EVEL_PRIORITY_NORMAL,
                         EVEL_SEVERITY_MAJOR,
			 EVEL_SOURCE_PORT,
			 EVEL_VF_STATUS_REQ_TERMINATE);
  if (fault != NULL)
  {
    evel_fault_type_set(fault, "Bad things happening");
    evel_fault_category_set(fault, "Failed category");
    evel_fault_interface_set(fault, "An Interface Card");
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
  printf("   Processed partial Fault\n");

  fault = evel_new_fault("My alarm condition",
                         "It broke very badly",
                         EVEL_PRIORITY_NORMAL,
                         EVEL_SEVERITY_MAJOR,
			 EVEL_SOURCE_HOST,
			 EVEL_VF_STATUS_PREP_TERMINATE);
  if (fault != NULL)
  {
    evel_fault_type_set(fault, "Bad things happen...");
    evel_fault_interface_set(fault, "My Interface Card");
    evel_fault_addl_info_add(fault, "name1", "value1");
    evel_fault_addl_info_add(fault, "name2", "value2");
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
  printf("   Processed full Fault\n");
}

/**************************************************************************//**
 * Create and send a measurement event.
 *****************************************************************************/
void demo_measurement(const int interval)
{
  EVENT_MEASUREMENT * measurement = NULL;
  MEASUREMENT_LATENCY_BUCKET * bucket = NULL;
  MEASUREMENT_VNIC_PERFORMANCE * vnic_performance = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  /***************************************************************************/
  /* Measurement                                                             */
  /***************************************************************************/
  measurement = evel_new_measurement(interval);
  if (measurement != NULL)
  {
    evel_measurement_type_set(measurement, "Perf management...");
    evel_measurement_conc_sess_set(measurement, 1);
    evel_measurement_cfg_ents_set(measurement, 2);
    evel_measurement_mean_req_lat_set(measurement, 4.4);
    evel_measurement_request_rate_set(measurement, 6);
    //evel_measurement_cpu_use_add(measurement, "cpu1", 11.11);
    //evel_measurement_cpu_use_add(measurement, "cpu2", 22.22);
    evel_measurement_addl_info_add(measurement, "name1", "value1");
    evel_measurement_addl_info_add(measurement, "name2", "value2");
    evel_get_cpu_stats(measurement);
    evel_get_disk_stats(measurement);
    evel_measurement_fsys_use_add(measurement,"00-11-22",100.11, 100.22, 33,
                                  200.11, 200.22, 44);
    evel_measurement_fsys_use_add(measurement,"33-44-55",300.11, 300.22, 55,
                                  400.11, 400.22, 66);

    bucket = evel_new_meas_latency_bucket(20);
    evel_meas_latency_bucket_low_end_set(bucket, 0.0);
    evel_meas_latency_bucket_high_end_set(bucket, 10.0);
    evel_meas_latency_bucket_add(measurement, bucket);

    bucket = evel_new_meas_latency_bucket(30);
    evel_meas_latency_bucket_low_end_set(bucket, 10.0);
    evel_meas_latency_bucket_high_end_set(bucket, 20.0);
    evel_meas_latency_bucket_add(measurement, bucket);

    vnic_performance = evel_measurement_new_vnic_performance("eth0", "true");
                                           
  evel_vnic_performance_rx_bcast_pkt_acc_set(vnic_performance, 1000000.023);
  evel_vnic_performance_rx_bcast_pkt_delta_set(vnic_performance,1234.767346);
  evel_vnic_performance_rx_discard_pkt_acc_set(vnic_performance, 127146784.234738587);
  evel_vnic_performance_rx_discard_pkt_delta_set(vnic_performance, 123445);
  evel_vnic_performance_rx_error_pkt_acc_set(vnic_performance, 2736448376);
  evel_vnic_performance_rx_error_pkt_delta_set(vnic_performance, 3474438764);
  evel_vnic_performance_rx_mcast_pkt_acc_set(vnic_performance, 42464676);
  evel_vnic_performance_rx_mcast_pkt_delta_set(vnic_performance, 42678657654);
  evel_vnic_performance_rx_octets_acc_set(vnic_performance, 47658745);
  evel_vnic_performance_rx_octets_delta_set(vnic_performance, 47656465465);
  evel_vnic_performance_rx_total_pkt_acc_set(vnic_performance, 4765764654444);
  evel_vnic_performance_rx_total_pkt_delta_set(vnic_performance, 4545665656);
  evel_vnic_performance_rx_ucast_pkt_acc_set(vnic_performance, 4765745546.);
  evel_vnic_performance_rx_ucast_pkt_delta_set(vnic_performance, 4768765.78347856);
  evel_vnic_performance_tx_bcast_pkt_acc_set(vnic_performance, 747665.347647);
  evel_vnic_performance_tx_bcast_pkt_delta_set(vnic_performance, 3468765.4774);
  evel_vnic_performance_tx_discarded_pkt_acc_set(vnic_performance, 53625345.53);
  evel_vnic_performance_tx_discarded_pkt_delta_set(vnic_performance, 5465345.72455);
  evel_vnic_performance_tx_error_pkt_acc_set(vnic_performance, 7632754.754);
  evel_vnic_performance_tx_error_pkt_delta_set(vnic_performance, 34646875444.);
  evel_vnic_performance_tx_mcast_pkt_acc_set(vnic_performance, 2734875.5534);
  evel_vnic_performance_tx_mcast_pkt_delta_set(vnic_performance, 562346534.654);
  evel_vnic_performance_tx_octets_acc_set(vnic_performance, 2785845.76874);
  evel_vnic_performance_tx_octets_delta_set(vnic_performance, 76532645.75);
  evel_vnic_performance_tx_total_pkt_acc_set(vnic_performance, 652365.5435);
  evel_vnic_performance_tx_total_pkt_delta_set(vnic_performance, 754354.456);
  evel_vnic_performance_tx_ucast_pkt_acc_set(vnic_performance, 738254835);
  evel_vnic_performance_tx_ucast_pkt_delta_set(vnic_performance, 763274);
    evel_meas_vnic_performance_add(measurement, vnic_performance);

    vnic_performance = evel_measurement_new_vnic_performance("eth1", "false");
  evel_vnic_performance_rx_mcast_pkt_delta_set(vnic_performance, 42678657654);
  evel_vnic_performance_rx_octets_acc_set(vnic_performance, 47658745);
  evel_vnic_performance_rx_octets_delta_set(vnic_performance, 47656465465);
  evel_vnic_performance_rx_total_pkt_acc_set(vnic_performance, 4765764654444);
  evel_vnic_performance_rx_total_pkt_delta_set(vnic_performance, 4545665656);
  evel_vnic_performance_rx_ucast_pkt_acc_set(vnic_performance, 4765745546.);
  evel_vnic_performance_rx_ucast_pkt_delta_set(vnic_performance, 4768765.78347856);
  evel_vnic_performance_tx_bcast_pkt_acc_set(vnic_performance, 747665.347647);
  evel_vnic_performance_tx_bcast_pkt_delta_set(vnic_performance, 3468765.4774);
  evel_vnic_performance_tx_discarded_pkt_acc_set(vnic_performance, 53625345.53);
    evel_meas_vnic_performance_add(measurement, vnic_performance);

    evel_measurement_errors_set(measurement, 1, 0, 2, 1);

    evel_measurement_feature_use_add(measurement, "FeatureA", 123);
    evel_measurement_feature_use_add(measurement, "FeatureB", 567);

    evel_measurement_codec_use_add(measurement, "G711a", 91);
    evel_measurement_codec_use_add(measurement, "G729ab", 92);

    evel_get_mem_stats(measurement);
    evel_measurement_media_port_use_set(measurement, 1234);

    evel_measurement_vnfc_scaling_metric_set(measurement, 1234.5678);

    evel_measurement_custom_measurement_add(measurement,
                                            "Group1", "Name1", "Value1");
    evel_measurement_custom_measurement_add(measurement,
                                            "Group2", "Name1", "Value1");
    evel_measurement_custom_measurement_add(measurement,
                                            "Group2", "Name2", "Value2");

    /*************************************************************************/
    /* Work out the time, to use as end of measurement period.               */
    /*************************************************************************/
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    unsigned long long epoch_now = tv_now.tv_usec + 1000000 * tv_now.tv_sec;
    evel_start_epoch_set(&measurement->header, epoch_start);
    evel_last_epoch_set(&measurement->header, epoch_now);
    epoch_start = epoch_now;
    evel_reporting_entity_name_set(&measurement->header, "measurer");
    evel_reporting_entity_id_set(&measurement->header, "measurer_id");

    evel_rc = evel_post_event((EVENT_HEADER *)measurement);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR("Post Measurement failed %d (%s)",
                 evel_rc,
                 evel_error_string());
    }
  }
  else
  {
    EVEL_ERROR("New Measurement failed");
  }
  printf("   Processed Measurement\n");
}

/**************************************************************************//**
 * Create and send three mobile flow events.
 *****************************************************************************/
void demo_mobile_flow(void)
{
  MOBILE_GTP_PER_FLOW_METRICS * metrics = NULL;
  EVENT_MOBILE_FLOW * mobile_flow = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  /***************************************************************************/
  /* Mobile Flow                                                             */
  /***************************************************************************/
  metrics = evel_new_mobile_gtp_flow_metrics(12.3,
                                             3.12,
                                             100,
                                             2100,
                                             500,
                                             1470409421,
                                             987,
                                             1470409431,
                                             11,
                                             (time_t)1470409431,
                                             "Working",
                                             87,
                                             3,
                                             17,
                                             123654,
                                             4561,
                                             0,
                                             12,
                                             10,
                                             1,
                                             3,
                                             7,
                                             899,
                                             901,
                                             302,
                                             6,
                                             2,
                                             0,
                                             110,
                                             225);
  if (metrics != NULL)
  {
    mobile_flow = evel_new_mobile_flow("Outbound",
                                       metrics,
                                       "TCP",
                                       "IPv4",
                                       "2.3.4.1",
                                       2341,
                                       "4.2.3.1",
                                       4321);
    if (mobile_flow != NULL)
    {
      evel_rc = evel_post_event((EVENT_HEADER *)mobile_flow);
      if (evel_rc != EVEL_SUCCESS)
      {
        EVEL_ERROR("Post Mobile Flow failed %d (%s)",
                   evel_rc,
                   evel_error_string());
      }
    }
    else
    {
      EVEL_ERROR("New Mobile Flow failed");
    }
    printf("   Processed empty Mobile Flow\n");
  }
  else
  {
    EVEL_ERROR("New GTP Per Flow Metrics failed - skipping Mobile Flow");
    printf("   Skipped empty Mobile Flow\n");
  }

  metrics = evel_new_mobile_gtp_flow_metrics(132.0001,
                                             31.2,
                                             101,
                                             2101,
                                             501,
                                             1470409422,
                                             988,
                                             1470409432,
                                             12,
                                             (time_t)1470409432,
                                             "Inactive",
                                             88,
                                             4,
                                             18,
                                             123655,
                                             4562,
                                             1,
                                             13,
                                             11,
                                             2,
                                             4,
                                             8,
                                             900,
                                             902,
                                             303,
                                             7,
                                             3,
                                             1,
                                             111,
                                             226);
  if (metrics != NULL)
  {
    mobile_flow = evel_new_mobile_flow("Inbound",
                                       metrics,
                                       "UDP",
                                       "IPv6",
                                       "2.3.4.2",
                                       2342,
                                       "4.2.3.2",
                                       4322);
    if (mobile_flow != NULL)
    {
      evel_mobile_flow_app_type_set(mobile_flow, "Demo application");
      evel_mobile_flow_app_prot_type_set(mobile_flow, "GSM");
      evel_mobile_flow_app_prot_ver_set(mobile_flow, "1");
      evel_mobile_flow_cid_set(mobile_flow, "65535");
      evel_mobile_flow_con_type_set(mobile_flow, "S1-U");
      evel_mobile_flow_ecgi_set(mobile_flow, "e65535");
      evel_mobile_flow_gtp_prot_type_set(mobile_flow, "GTP-U");
      evel_mobile_flow_gtp_prot_ver_set(mobile_flow, "1");
      evel_mobile_flow_http_header_set(mobile_flow,
                                       "http://www.something.com");
      evel_mobile_flow_imei_set(mobile_flow, "209917614823");
      evel_mobile_flow_imsi_set(mobile_flow, "355251/05/850925/8");
      evel_mobile_flow_lac_set(mobile_flow, "1");
      evel_mobile_flow_mcc_set(mobile_flow, "410");
      evel_mobile_flow_mnc_set(mobile_flow, "04");
      evel_mobile_flow_msisdn_set(mobile_flow, "6017123456789");
      evel_mobile_flow_other_func_role_set(mobile_flow, "MME");
      evel_mobile_flow_rac_set(mobile_flow, "514");
      evel_mobile_flow_radio_acc_tech_set(mobile_flow, "LTE");
      evel_mobile_flow_sac_set(mobile_flow, "1");
      evel_mobile_flow_samp_alg_set(mobile_flow, 1);
      evel_mobile_flow_tac_set(mobile_flow, "2099");
      evel_mobile_flow_tunnel_id_set(mobile_flow, "Tunnel 1");
      evel_mobile_flow_vlan_id_set(mobile_flow, "15");

      evel_rc = evel_post_event((EVENT_HEADER *)mobile_flow);
      if (evel_rc != EVEL_SUCCESS)
      {
        EVEL_ERROR("Post Mobile Flow failed %d (%s)",
                   evel_rc,
                   evel_error_string());
      }
    }
    else
    {
      EVEL_ERROR("New Mobile Flow failed");
    }
    printf("   Processed partial Mobile Flow\n");
  }
  else
  {
    EVEL_ERROR("New GTP Per Flow Metrics failed - skipping Mobile Flow");
    printf("   Skipped partial Mobile Flow\n");
  }

  metrics = evel_new_mobile_gtp_flow_metrics(12.32,
                                             3.122,
                                             1002,
                                             21002,
                                             5002,
                                             1470409423,
                                             9872,
                                             1470409433,
                                             112,
                                             (time_t)1470409433,
                                             "Failed",
                                             872,
                                             32,
                                             172,
                                             1236542,
                                             45612,
                                             2,
                                             122,
                                             102,
                                             12,
                                             32,
                                             72,
                                             8992,
                                             9012,
                                             3022,
                                             62,
                                             22,
                                             2,
                                             1102,
                                             2252);
  if (metrics != NULL)
  {
    evel_mobile_gtp_metrics_dur_con_fail_set(metrics, 12);
    evel_mobile_gtp_metrics_dur_tun_fail_set(metrics, 13);
    evel_mobile_gtp_metrics_act_by_set(metrics, "Remote");
    evel_mobile_gtp_metrics_act_time_set(metrics, (time_t)1470409423);
    evel_mobile_gtp_metrics_deact_by_set(metrics, "Remote");
    evel_mobile_gtp_metrics_con_status_set(metrics, "Connected");
    evel_mobile_gtp_metrics_tun_status_set(metrics, "Not tunneling");
    evel_mobile_gtp_metrics_iptos_set(metrics, 1, 13);
    evel_mobile_gtp_metrics_iptos_set(metrics, 17, 1);
    evel_mobile_gtp_metrics_iptos_set(metrics, 4, 99);
    evel_mobile_gtp_metrics_large_pkt_rtt_set(metrics, 80);
    evel_mobile_gtp_metrics_large_pkt_thresh_set(metrics, 600.0);
    evel_mobile_gtp_metrics_max_rcv_bit_rate_set(metrics, 1357924680);
    evel_mobile_gtp_metrics_max_trx_bit_rate_set(metrics, 235711);
    evel_mobile_gtp_metrics_num_echo_fail_set(metrics, 1);
    evel_mobile_gtp_metrics_num_tun_fail_set(metrics, 4);
    evel_mobile_gtp_metrics_num_http_errors_set(metrics, 2);
    evel_mobile_gtp_metrics_tcp_flag_count_add(metrics, EVEL_TCP_CWR, 10);
    evel_mobile_gtp_metrics_tcp_flag_count_add(metrics, EVEL_TCP_URG, 121);
    evel_mobile_gtp_metrics_qci_cos_count_add(
                                metrics, EVEL_QCI_COS_UMTS_CONVERSATIONAL, 11);
    evel_mobile_gtp_metrics_qci_cos_count_add(
                                            metrics, EVEL_QCI_COS_LTE_65, 122);

    mobile_flow = evel_new_mobile_flow("Outbound",
                                       metrics,
                                       "RTP",
                                       "IPv8",
                                       "2.3.4.3",
                                       2343,
                                       "4.2.3.3",
                                       4323);
    if (mobile_flow != NULL)
    {
      evel_mobile_flow_addl_field_add(mobile_flow, "name1", "value1");
      evel_mobile_flow_addl_field_add(mobile_flow, "name2", "value2");
      evel_mobile_flow_addl_field_add(mobile_flow, "name3", "value3");

      evel_mobile_flow_app_type_set(mobile_flow, "Demo application 2");
      evel_mobile_flow_app_prot_type_set(mobile_flow, "GSM");
      evel_mobile_flow_app_prot_ver_set(mobile_flow, "2");
      evel_mobile_flow_cid_set(mobile_flow, "1");
      evel_mobile_flow_con_type_set(mobile_flow, "S1-U");
      evel_mobile_flow_ecgi_set(mobile_flow, "e1");
      evel_mobile_flow_gtp_prot_type_set(mobile_flow, "GTP-U");
      evel_mobile_flow_gtp_prot_ver_set(mobile_flow, "1");
      evel_mobile_flow_http_header_set(mobile_flow, "http://www.google.com");
      evel_mobile_flow_imei_set(mobile_flow, "209917614823");
      evel_mobile_flow_imsi_set(mobile_flow, "355251/05/850925/8");
      evel_mobile_flow_lac_set(mobile_flow, "1");
      evel_mobile_flow_mcc_set(mobile_flow, "410");
      evel_mobile_flow_mnc_set(mobile_flow, "04");
      evel_mobile_flow_msisdn_set(mobile_flow, "6017123456789");
      evel_mobile_flow_other_func_role_set(mobile_flow, "MMF");
      evel_mobile_flow_rac_set(mobile_flow, "514");
      evel_mobile_flow_radio_acc_tech_set(mobile_flow, "3G");
      evel_mobile_flow_sac_set(mobile_flow, "1");
      evel_mobile_flow_samp_alg_set(mobile_flow, 2);
      evel_mobile_flow_tac_set(mobile_flow, "2099");
      evel_mobile_flow_tunnel_id_set(mobile_flow, "Tunnel 2");
      evel_mobile_flow_vlan_id_set(mobile_flow, "4096");

      evel_rc = evel_post_event((EVENT_HEADER *)mobile_flow);
      if (evel_rc != EVEL_SUCCESS)
      {
        EVEL_ERROR("Post Mobile Flow failed %d (%s)",
                   evel_rc,
                   evel_error_string());
      }
    }
    else
    {
      EVEL_ERROR("New Mobile Flow failed");
    }
    printf("   Processed full Mobile Flow\n");
  }
  else
  {
    EVEL_ERROR("New GTP Per Flow Metrics failed - skipping Mobile Flow");
    printf("   Skipped full Mobile Flow\n");
  }
}

/**************************************************************************//**
 * Create and send a Heartbeat field event.
 *****************************************************************************/
void demo_heartbeat_field_event(void)
{
  EVENT_HEARTBEAT_FIELD * event = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  event = evel_new_heartbeat_field(3);
  if (event != NULL)
  {
    evel_hrtbt_interval_set(event, 10);
    evel_hrtbt_field_addl_field_add(event, "Name1", "Value1");
    evel_hrtbt_field_addl_field_add(event, "Name2", "Value2");

    evel_rc = evel_post_event((EVENT_HEADER *) event);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }
  else
  {
    EVEL_ERROR("New heartbeat field failed");
  }
  printf("   Processed heartbeat Events\n");
}

/**************************************************************************//**
 * Create and send a Signaling event.
 *****************************************************************************/
void demo_signaling(void)
{
  EVENT_SIGNALING * event = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  event = evel_new_signaling("vendor_x", "correlator", "1.0.3.1", "1234", "192.168.1.3","3456");
  if (event != NULL)
  {
    evel_signaling_addl_info_add(event, "name1", "value1");
    evel_signaling_addl_info_add(event, "name2", "value2");
    evel_signaling_type_set(event, "Signaling");
    evel_signaling_correlator_set(event, "vendor_x_correlator");
    evel_signaling_local_ip_address_set(event, "1.0.3.1");
    evel_signaling_local_port_set(event, "1031");
    evel_signaling_remote_ip_address_set(event, "5.3.3.0");
    evel_signaling_remote_port_set(event, "5330");
    evel_signaling_compressed_sip_set(event, "compressed_sip");
    evel_signaling_summary_sip_set(event, "summary_sip");
    evel_signaling_vnfmodule_name_set(event, "vendor_x_module");
    evel_signaling_vnfname_set(event, "vendor_x_vnf");
    evel_rc = evel_post_event((EVENT_HEADER *) event);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }
  else
  {
    EVEL_ERROR("New Signaling failed");
  }
  printf("   Processed Signaling\n");
}

/**************************************************************************//**
 * Create and send a state change event.
 *****************************************************************************/
void demo_state_change(void)
{
  EVENT_STATE_CHANGE * state_change = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  /***************************************************************************/
  /* State Change                                                            */
  /***************************************************************************/
  state_change = evel_new_state_change(EVEL_ENTITY_STATE_IN_SERVICE,
                                       EVEL_ENTITY_STATE_OUT_OF_SERVICE,
                                       "Interface");
  if (state_change != NULL)
  {
    evel_state_change_type_set(state_change, "State Change");
    evel_state_change_addl_field_add(state_change, "Name1", "Value1");
    evel_state_change_addl_field_add(state_change, "Name2", "Value2");
    evel_rc = evel_post_event((EVENT_HEADER *)state_change);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }
  else
  {
    EVEL_ERROR("New State Change failed");
  }
  printf("   Processed State Change\n");
}

/**************************************************************************//**
 * Create and send two syslog events.
 *****************************************************************************/
void demo_syslog(void)
{
  EVENT_SYSLOG * syslog = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  /***************************************************************************/
  /* Syslog                                                                  */
  /***************************************************************************/
  syslog = evel_new_syslog(EVEL_SOURCE_VIRTUAL_NETWORK_FUNCTION,
                           "EVEL library message",
                           "EVEL");
  if (syslog != NULL)
  {
    evel_rc = evel_post_event((EVENT_HEADER *)syslog);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }
  else
  {
    EVEL_ERROR("New Syslog failed");
  }
  printf("   Processed empty Syslog\n");

  syslog = evel_new_syslog(EVEL_SOURCE_VIRTUAL_MACHINE,
                           "EVEL library message",
                           "EVEL");
  if (syslog != NULL)
  {
    evel_syslog_event_source_host_set(syslog, "Virtual host");
    evel_syslog_facility_set(syslog, EVEL_SYSLOG_FACILITY_LOCAL0);
    evel_syslog_proc_set(syslog, "vnf_process");
    evel_syslog_proc_id_set(syslog, 1423);
    evel_syslog_version_set(syslog, 1);
    evel_syslog_addl_filter_set(syslog, "Name1=Value1|Name2=Value2|Name3=Value3");
    evel_syslog_sdid_set(syslog, "u354@876876");
    evel_syslog_severity_set(syslog, "Error");
    evel_rc = evel_post_event((EVENT_HEADER *)syslog);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }
  else
  {
    EVEL_ERROR("New Syslog failed");
  }
  printf("   Processed full Syslog\n");
}


/**************************************************************************//**
 * Create and send VoiceQuality events.
 *****************************************************************************/
void demo_voicequality(void)
{
  EVENT_VOICE_QUALITY * vq = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  /***************************************************************************/
  /* Voice Quality                                                           */
  /***************************************************************************/
  vq = evel_new_voice_quality("calleeCodec", "callerCodec",
                           "correlator", "midrtcp",
                           "EVEL");
  if (vq != NULL)
  {
    evel_rc = evel_post_event((EVENT_HEADER *)vq);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }
  else
  {
    EVEL_ERROR("New Syslog failed");
  }
  printf("   Processed empty Syslog\n");

  vq = evel_new_voice_quality("calleeCodec", "callerCodec",
                           "correlator", "midrtcp",
                           "EVEL");
  if (vq != NULL)
  {
    evel_voice_quality_end_metrics_add(vq, "adjacent", EVEL_SERVICE_ENDPOINT_CALLER,
					0,1,2,3,4,5,6,7,8,9,10,11,12,13,4,15,16,17,18);
    evel_voice_quality_end_metrics_add(vq, "adjacent", EVEL_SERVICE_ENDPOINT_CALLEE,
					0,1,2,3,4,5,6,7,8,9,10,11,12,13,4,15,16,17,18);

    evel_voice_quality_vnfmodule_name_set(vq, "vnfmodule");
    evel_voice_quality_vnfname_set(vq, "vFW");
    evel_voice_quality_phone_number_set(vq, "123456789");


    evel_rc = evel_post_event((EVENT_HEADER *)vq);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }
  else
  {
    EVEL_ERROR("New Syslog failed");
  }
  printf("   Processed full Syslog\n");
}

/**************************************************************************//**
 * Create and send two other events.
 *****************************************************************************/
void demo_other(void)
{
  EVENT_OTHER * other = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  /***************************************************************************/
  /* Other                                                                   */
  /***************************************************************************/
  other = evel_new_other();
  if (other != NULL)
  {
    evel_rc = evel_post_event((EVENT_HEADER *)other);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }
  else
  {
    EVEL_ERROR("New Other failed");
  }
  printf("   Processed empty Other\n");

  other = evel_new_other();
  if (other != NULL)
  {


   char * jsstr = "{"
                 "\"data1\":[1,2,3,4,5,6,7,8,9],"
                 "\"data2\":["
                     "[3,4,5,6,1],"
                     "[8,4,5,6,1],"
                     "[10,4,5,3,61],"
                     "[3,4,5,6,1],"
                     "[3,4,5,6,1],"
                     "[3,4,5,6,1]"
                 "]"
             "}";


char * jsstr2 = "{ \"employee\":{ \"name\":\"John\", \"age\":30, \"city\":\"New York\" } }";

    evel_other_field_set_namedarraysize(other,50);
    evel_other_field_add_namedarray(other,"name1", "disk1", "10000");
    evel_other_field_add_namedarray(other,"name2", "disk2", "20000");
    evel_other_field_add_namedarray(other,"name1", "disk2", "20000");
    evel_other_field_add_namedarray(other,"name1", "disk1", "20000");

     EVEL_JSON_OBJECT_INSTANCE * njinst = evel_new_jsonobjinstance(jsstr );
     evel_epoch_microsec_set(njinst,9287578586767);
     EVEL_INTERNAL_KEY * nkey = evel_new_internal_key("key1");
     evel_internal_key_keyorder_set(nkey , 2);
     evel_internal_key_keyvalue_set(nkey , "val1");
     EVEL_INTERNAL_KEY * nkey2= evel_new_internal_key("key2");
     evel_internal_key_keyorder_set(nkey2, 2);
     evel_internal_key_keyvalue_set(nkey2, "val2");
     evel_jsonobjinst_add_objectkey(njinst, nkey);
     evel_jsonobjinst_add_objectkey(njinst, nkey2);

     EVEL_JSON_OBJECT_INSTANCE * njinst2 = evel_new_jsonobjinstance(jsstr2 );
     evel_epoch_microsec_set(njinst2,927578586767);

    EVEL_JSON_OBJECT * myobj =  evel_new_jsonobject("Myjobject");
    evel_jsonobject_objectschema_set(myobj,"jsonschema5.0");
    evel_jsonobject_objectschemaurl_set(myobj,"http://jsonschema5.0.att.com");
    evel_jsonobject_nfsubscribedobjname_set(myobj,"nfobj1");
    evel_jsonobject_nfsubscriptionid_set(myobj,"nfid1234");
    evel_jsonobject_add_jsoninstance(myobj,njinst);
    evel_jsonobject_add_jsoninstance(myobj,njinst2);
    evel_other_field_add_jsonobj(other,myobj);

    evel_other_field_add(other,
                         "Other field 1",
                         "Other value 1");


    evel_rc = evel_post_event((EVENT_HEADER *)other);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }
  else
  {
    EVEL_ERROR("New Other failed");
  }
  printf("   Processed small Other\n");
  other = evel_new_other();
  if (other != NULL)
  {
    evel_other_field_add(other,
                         "Other field A",
                         "Other value A");
    evel_other_field_add(other,
                         "Other field B",
                         "Other value B");
    evel_other_field_add(other,
                         "Other field C",
                         "Other value C");

    evel_rc = evel_post_event((EVENT_HEADER *)other);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }
  else
  {
    EVEL_ERROR("New Other failed");
  }
  printf("   Processed large Other\n");
}
