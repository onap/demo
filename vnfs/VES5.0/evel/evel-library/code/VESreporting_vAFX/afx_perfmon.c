 /*****************************************************************************//***
 * Copyright(c) <2017>, AT&T Intellectual Property.  All other rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:  This product includes
 *    software developed by the AT&T.
 * 4. Neither the name of AT&T nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "evel.h"
#include "afx_ves_reporter.h"

unsigned long long epoch_start = 0;
#define MAX_CHILDREN 100


/**************************************************************************//**
 * Utility to compare linknames for ordering
 *****************************************************************************/
int cmpfunc (const void * a, const void * b)
{
   VPP_METRICS_STRUCT *aptr = (VPP_METRICS_STRUCT *)a;
   VPP_METRICS_STRUCT *bptr = (VPP_METRICS_STRUCT *)b;
   return ( strcmp(aptr->linkname,bptr->linkname) );
}



/**************************************************************************//**
 * Gets CPU Load Statistics
 *
 * @param measurement Pointer to measurement event
 *
 *
 *****************************************************************************/
void evel_get_cpu_stats(EVENT_MEASUREMENT * measurement)
{
  FILE *fp;
  char path[1024];
  char ** res  = NULL;
  int n_spaces = 0, i;
  char *p;
  char cpname[256];

  double usage=0.0;
  double idle;
  double intrpt;
  double nice;
  double softirq;
  //double steal;
  double sys;
  //double user;
  double usrl;
  //double wait;
  MEASUREMENT_CPU_USE *cpu_use = NULL;

  /* Open the command for reading. */
  fp = popen("/usr/bin/mpstat -P ALL ", "r");
  if (fp == NULL) {
    EVEL_ERROR(" AFX: Perf thread Failed to run command\n" );
    pthread_exit(NULL);
  }

  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
    //printf("%s",path);

n_spaces = 0;
p    = strtok (path, " ");
/* split string and append tokens to 'res' */

while (p) {
  res = realloc (res, sizeof (char*) * ++n_spaces);

  if (res == NULL)
    exit (-1); /* memory allocation failed */

  res[n_spaces-1] = p;

  p = strtok (NULL, " ");
}

/* realloc one extra element for the last NULL */
res = realloc (res, sizeof (char*) * (n_spaces+1));
res[n_spaces] = 0;

/* print the result */
if( n_spaces > 2 && (!strcmp(res[2],"all") || isdigit(res[2][0])) )
{
  //for (i = 0; i < (n_spaces+1); ++i)
  //{
  //  printf ("res[%d] = %s\n", i, res[i]);
  //}
  if(isdigit(res[2][0]))
  {
    sprintf (cpname,"cpu%s", res[2]);
  }
  else
    sprintf (cpname,"%s", res[2]);

    usrl = atof(res[3]);
    nice = atof(res[4]);
    sys = atof(res[4]);
    idle = atof(res[12]);
    usage = 100.0 - idle;

  //allocate CPU stats structure
  cpu_use = evel_measurement_new_cpu_use_add(measurement, cpname, usage);
  if( cpu_use != NULL ){
  evel_measurement_cpu_use_idle_set(cpu_use,idle);
  //evel_measurement_cpu_use_interrupt_set(cpu_use,intrpt);
  evel_measurement_cpu_use_nice_set(cpu_use,nice);
  //evel_measurement_cpu_use_softirq_set(cpu_use,softirq);
  //evel_measurement_cpu_use_steal_set(cpu_use,steal);
  evel_measurement_cpu_use_system_set(cpu_use,sys);
  evel_measurement_cpu_use_usageuser_set(cpu_use,usrl);
  //evel_measurement_cpu_use_wait_set(cpu_use,wait);
  }
}

  }

/* free the memory allocated */
if(res != NULL) free (res);
res = NULL;

  /* close */
  pclose(fp);

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

EVEL_INFO("AFX perfmon meminfo %d\n",bytes_read);

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
EVEL_INFO(" AFX perfmon memused %lf\n",memused);

  mem_use = evel_measurement_new_mem_use_add(measurement, "RAM", hostname, membuffsz);
  evel_measurement_mem_use_memcache_set(mem_use,memcache);
  evel_measurement_mem_use_memconfig_set(mem_use,memconfig);
  evel_measurement_mem_use_memfree_set(mem_use,memfree);
  evel_measurement_mem_use_slab_reclaimed_set(mem_use,slabrecl);
  evel_measurement_mem_use_slab_unreclaimable_set(mem_use,slabunrecl);
  evel_measurement_mem_use_usedup_set(mem_use,memused);

}

/**
 *  System command execution output
 *    @param <char> command - system command to execute
 *    @returb <char> execution output
 */
char *system_output (const char *command)
{
  FILE *pipe;
  static char out[512];
  out[0]= '\0';
  pipe = popen (command, "r");
  if( pipe != NULL )
  {
   fgets (out, sizeof(out), pipe);
   pclose (pipe);
  }
  return out;
}


/**
 *  Finding all process's children
 *    @param <Int> - process ID
 *    @param <Int> - array of childs
 */
void find_children (int pid, int children[])
{
  int child_pid, i = 1;
  char empty_command[] = "/bin/ps h -o pid --ppid ";
  char pid_string[5];

  snprintf(pid_string, 5, "%d", pid);

  char *command = (char*) malloc(strlen(empty_command) + strlen(pid_string) + 1);
  sprintf(command, "%s%s", empty_command, pid_string);

  FILE *fp = popen(command, "r");
  if( fp != NULL )
  {
    while (fscanf(fp, "%i", &child_pid) != EOF)
    {
      children[i] = child_pid;
      i++;
    }
  }
  free(command);
  pclose(fp);
}


/**
 *  Parsign `ps` command output
 *    @param <char> out - ps command output
 *    @return <int> cpu utilization
 */
int parse_cpu_utilization (char *out, float *pcpu, float *pmem)
{
  if( strlen(out) > 0 )
  {
   sscanf (out, "%f %f", pcpu, pmem);
  }
  return 0;
}


void gather_afx_stats(EVENT_MEASUREMENT *vpp_m,char *lkstr)
{
  double totcpu=0.0,totmem=0.0;
  float tmpcpu=0.0,tmpmem=0.0;
  FILE *fp;
  char path[512];
  char cmd[128];
  char grp[128];
  char cpustr[64];
  char memstr[64];
  char *p;
  //unsigned pid = atoi(argv[1]);
  unsigned pid;
  // getting array with process children
  int process_children[MAX_CHILDREN] = { 0 };
  unsigned i;
  float common_cpu_usage = 0.0;

  /* Open the command for reading. */
  sprintf(cmd," pgrep -f \"%s\" | xargs --no-run-if-empty ps --no-headers ", lkstr);
  fp = popen(cmd, "r");
  if (fp == NULL) {
    EVEL_ERROR("AFX meas : Failed to run afx command\n" );
    pthread_exit(NULL);
  }

  /* Read the output a line at a time - output it. */
  while ( !feof(fp) ) {
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
    EVEL_DEBUG("AFX meas : %s",path);
    p = &path[0];
    while( p!=NULL && isspace(*p))p++;
    if( strstr(p,"sh -c") == NULL && isdigit(*p))
    {
      sscanf(p,"%d ",&pid);
      //printf(":%d:\n",pid);
  process_children[0] = pid; // parent PID as first element
  find_children(pid, process_children);

  // calculating summary processor utilization
  for (i = 0; i < sizeof(process_children)/sizeof(int); ++i)
  {
    if (process_children[i] > 0)
    {
      char *command = (char*)malloc(128);
      EVEL_DEBUG ("AFX meas :/bin/ps -p %i -o 'pcpu,pmem' --no-headers\n", process_children[i]);
      sprintf (command, "/bin/ps -p %i -o 'pcpu,pmem' --no-headers", process_children[i]);
      parse_cpu_utilization(system_output(command),&tmpcpu,&tmpmem);
      totcpu += tmpcpu;
      totmem += tmpmem;
    }
  }
  sprintf(grp,"%s_%d",lkstr,pid);
  EVEL_DEBUG("%s %d CPU %f MEM %f\n", grp, totcpu, totmem);
  sprintf(cpustr,"%f",totcpu);
  sprintf(memstr,"%f",totmem);
  evel_measurement_custom_measurement_add(vpp_m,grp,"cpu",cpustr);
  evel_measurement_custom_measurement_add(vpp_m,grp,"mem",memstr);

    }
   }
  }
  pclose(fp);


}



/**************************************************************************//**
 * tap live afx stats
 *****************************************************************************/
void evel_get_afx_stats(EVENT_MEASUREMENT * measurement)
{
   gather_afx_stats(measurement,"AFXin");
   gather_afx_stats(measurement,"AFXout");
}

/**************************************************************************//**
 * Thread function to Monitor AFX Performance parameters
 * Opens interface list file to get list of interfaces
 * Opens Device driver file every READ_INTERVAL and calculates
 *   Delta packets and bytes
 *
 * @param threadarg  Thread arguments for startup message
 *
 * 
 *****************************************************************************/
void *MeasureAfxThread(void *threadarg)
{
  struct thread_data *my_data;
   int taskid, sum;
   char *hello_msg;
  struct timeval time_val;
  int i;
  //time_t start_epoch;
  //time_t last_epoch;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;
  EVENT_MEASUREMENT* vpp_m = NULL;
  EVENT_HEADER* vpp_m_header = NULL;
  MEASUREMENT_VNIC_PERFORMANCE * vnic_performance = NULL;
  //struct timeval tv_start;
    FILE *fp;
    char const* const fileName = AFX_INTERFACE_FILE;
    char line[256];
    char intf[256];
    char descr[256];
    char linkdata[256];
    int linkcount = 0;
    time_t filetime = 0;
    int newrun = 0;
    int n_spaces = 0, j;
    char ** res  = NULL;
    char *pos;
    char *p;
    int tlinkstat;
    VPP_METRICS_STRUCT intfstats[MAX_INTERFACES];
    VPP_METRICS_STRUCT *ptritem;
    uint64_t  curr_rx_bytes;
    uint64_t  curr_rx_packets;
    uint64_t  curr_rx_mcast;
    uint64_t  curr_tx_bytes;
    uint64_t  curr_tx_packets;
  char vesevid[128];


   sleep(1);
   my_data = (struct thread_data *) threadarg;
   taskid = my_data->thread_id;
   sum = my_data->sum;
   hello_msg = my_data->message;
   EVEL_INFO("AFX Perf Thread %d: %s  Sum=%d\n", taskid, hello_msg, sum);
   sprintf(vesevid,"measurementsForVfScaling_vAfx_%s",get_oam_intfaddr());

  gettimeofday(&time_val, NULL);
  epoch_start = time_val.tv_sec * 1000000 + time_val.tv_usec;
  sleep(PERF_MONITOR_INTERVAL);

  while(1) {

    if( file_is_modified(fileName, &filetime) )  newrun = 1;

    if( newrun ) {

    linkcount = 0;
    FILE* file = fopen(fileName, "r"); /* should check the result */

    while ( file != NULL && fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
        //printf("%s", line);
        sscanf(line,"%s %s",intf,descr);
	//remove_spaces(line);
        if ((pos=strchr(line, '\n')) != NULL)
           *pos = '\0';
        if( newrun ){
                memset(&intfstats[linkcount],0,sizeof(VPP_METRICS_STRUCT));
                strcpy(intfstats[linkcount].linkname,intf);
                strcpy(intfstats[linkcount].linkdescr,line);
        }
        linkcount++;
     }
     fclose(file);
     if ((pos=strchr(line, '\n')) != NULL)
           *pos = '\0';
     qsort(intfstats,linkcount,sizeof(VPP_METRICS_STRUCT),cmpfunc);
   }


  /* Open the command for reading. */
  fp = popen("cat /proc/net/dev ", "r");
  if (fp == NULL) {
    EVEL_DEBUG("Failed to run net command\n" );
    //exit(1);
    sleep(PERF_MONITOR_INTERVAL);
    continue;
  }

  /* Read the output a line at a time - output it. */
  while ( !feof(fp) ) {
  while (fgets(linkdata, sizeof(linkdata)-1, fp) != NULL) {
    //printf("%s",linkdata);

n_spaces = 0;
p    = strtok (linkdata, " ");
/* split string and append tokens to 'res' */

while (p) {
  res = realloc (res, sizeof (char*) * ++n_spaces);

  if (res == NULL){
    EVEL_DEBUG("Memory allocation failed");
    exit (-1); /* memory allocation failed */
  }

  res[n_spaces-1] = p;

  p = strtok (NULL, " ");
}

/* realloc one extra element for the last NULL */
res = realloc (res, sizeof (char*) * (n_spaces+1));
res[n_spaces] = 0;


 if( !strstr(res[0],"Inter") && !strstr(res[0],"face") )
 {
  /* for (i = 0; i < (n_spaces+1); ++i)
  {
    printf ("res[%d] = %s\n", i, res[i]);
  }*/
  if ((pos=strchr(res[0], ':')) != NULL)
           *pos = '\0';
   /* using bsearch() to find value 32 in the array */
   ptritem = (VPP_METRICS_STRUCT*) bsearch (res[0], intfstats, linkcount, sizeof(VPP_METRICS_STRUCT), cmpfunc);
   if( ptritem == NULL  )
   {
      EVEL_DEBUG("Item = %s could not be found\n", res[0]);
   }
   else if( !strcmp(res[0],ptritem->linkname) )
   {
      EVEL_DEBUG( "AFX Perf values Link %d rxb %d rxp %d rxm %d txb %d txp %d\n", linkcount, ptritem->rx_bytes, ptritem->rx_packets, ptritem->rx_mcast, ptritem->tx_bytes, ptritem->tx_packets );

      curr_rx_bytes = strtoull(res[1],NULL,10);
      curr_rx_packets = strtoull(res[2],NULL,10);
      curr_rx_mcast = strtoull(res[8],NULL,10);
      curr_tx_bytes = strtoull(res[9],NULL,10);
      curr_tx_packets = strtoull(res[10],NULL,10);
      if( !newrun ){
         ptritem->delta_rx_bytes = curr_rx_bytes - ptritem->rx_bytes;
         ptritem->delta_rx_packets = curr_rx_packets - ptritem->rx_packets;
         ptritem->delta_rx_mcast = curr_rx_mcast - ptritem->rx_mcast;
         ptritem->delta_tx_bytes = curr_tx_bytes - ptritem->tx_bytes;
         ptritem->delta_tx_packets = curr_tx_packets - ptritem->tx_packets;
      }
      else
      {
         ptritem->delta_rx_bytes = 0;
         ptritem->delta_rx_packets = 0;
         ptritem->delta_rx_mcast = 0;
         ptritem->delta_tx_bytes = 0;
         ptritem->delta_tx_packets =  0;
      }
      ptritem->rx_bytes = curr_rx_bytes;
      ptritem->rx_packets = curr_rx_packets;
      ptritem->rx_mcast = curr_rx_mcast;
      ptritem->tx_bytes = curr_tx_bytes;
      ptritem->tx_packets = curr_tx_packets;

     //printf( "Acc %s %d %d %d %d %d\n", ptritem->linkname, curr_rx_bytes, curr_rx_packets, curr_rx_mcast, curr_tx_bytes,  curr_tx_packets );
     //printf( "Delta %d %d %d %d %d\n", ptritem->delta_rx_bytes, ptritem->delta_rx_packets, ptritem->delta_rx_mcast, ptritem->delta_tx_bytes, ptritem->delta_tx_packets );

   }
  }
  }
 }
 pclose(fp);
 newrun = 0;
 if(res != NULL) free (res);
 res = NULL;

  /***************************************************************************/
  /* Collect metrics from the VNIC                                           */
  /***************************************************************************/
    vpp_m = evel_new_measurement(PERF_MONITOR_INTERVAL,"measurementsForVfScaling_vAfx",vesevid);

    if(vpp_m != NULL) {
    evel_header_type_set((EVENT_HEADER *)vpp_m, "applicationVnf");
    evel_nfcnamingcode_set((EVENT_HEADER *)vpp_m, "AFX");
    evel_nfnamingcode_set((EVENT_HEADER *)vpp_m, "AFX");
      EVEL_DEBUG("New measurement report created...\n");

    for( i=0; i<linkcount;i++)
    {
      ptritem = &intfstats[i];
      vnic_performance = (MEASUREMENT_VNIC_PERFORMANCE *)evel_measurement_new_vnic_performance
                                (ptritem->linkdescr, "true");

      evel_meas_vnic_performance_add(vpp_m, vnic_performance);

     //printf( "Acc %s %d %d %d %d %d\n", ptritem->linkname, curr_rx_bytes, curr_rx_packets, curr_rx_mcast, curr_tx_bytes,  curr_tx_packets );
     //printf( "Delta %d %d %d %d %d\n", delta_rx_bytes, delta_rx_packets, delta_rx_mcast, delta_tx_bytes, delta_tx_packets );
      evel_vnic_performance_rx_octets_acc_set(vnic_performance, ptritem->rx_bytes);
      evel_vnic_performance_rx_octets_delta_set(vnic_performance, ptritem->delta_rx_bytes);
      evel_vnic_performance_tx_octets_acc_set(vnic_performance, ptritem->tx_bytes);
      evel_vnic_performance_tx_octets_delta_set(vnic_performance, ptritem->delta_tx_bytes);

      evel_vnic_performance_rx_mcast_pkt_acc_set(vnic_performance, ptritem->rx_mcast);
      evel_vnic_performance_rx_mcast_pkt_delta_set(vnic_performance, ptritem->delta_rx_mcast);

      evel_vnic_performance_rx_total_pkt_acc_set(vnic_performance, ptritem->rx_packets);
      evel_vnic_performance_rx_total_pkt_delta_set(vnic_performance, ptritem->delta_rx_packets);
      evel_vnic_performance_tx_total_pkt_acc_set(vnic_performance, ptritem->tx_packets);
      evel_vnic_performance_tx_total_pkt_delta_set(vnic_performance, ptritem->delta_tx_packets);

    }

      evel_get_cpu_stats(vpp_m);
      evel_get_mem_stats(vpp_m);
      evel_get_afx_stats(vpp_m);
      /***************************************************************************/
      /* Set parameters in the MEASUREMENT header packet                         */
      /***************************************************************************/
      struct timeval tv_now;
      gettimeofday(&tv_now, NULL);
      unsigned long long epoch_now = tv_now.tv_usec + 1000000 * tv_now.tv_sec;

      //last_epoch = start_epoch + PERF_MONITOR_INTERVAL * 1000000;
      vpp_m_header = (EVENT_HEADER *)vpp_m;
      evel_start_epoch_set(&vpp_m->header, epoch_start);
      evel_last_epoch_set(&vpp_m->header, epoch_now);
      epoch_start = epoch_now;

      evel_reporting_entity_name_set(&vpp_m->header, "AFXM");
      evel_reporting_entity_id_set(&vpp_m->header, hostname);
      evel_rc = evel_post_event(vpp_m_header);

      if(evel_rc == EVEL_SUCCESS) {
        EVEL_DEBUG("Measurement report correctly sent to the collector!\n");
      }
      else {
        EVEL_DEBUG("Post failed %d (%s)\n", evel_rc, evel_error_string());
      }
    }
    else {
      EVEL_DEBUG("New measurement report failed (%s)\n", evel_error_string());
    }
  
    sleep(PERF_MONITOR_INTERVAL);
  }

  /***************************************************************************/
  /* Terminate                                                               */
  /***************************************************************************/
  sleep(1);

  return 0;
}

