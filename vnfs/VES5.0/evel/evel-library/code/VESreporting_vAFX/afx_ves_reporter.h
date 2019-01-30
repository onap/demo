#ifndef _VES_REPORTER
#define _VES_REPORTER  1

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

#define NUM_THREADS     5
#define MAX_INTERFACES 40

#define BUFSIZE 128
#define BGPBUFSIZE 512
#define PERF_MONITOR_INTERVAL 300
#define LINK_MONITOR_INTERVAL 20

struct thread_data
{
   int  thread_id;
   int  sum;
   char *message;
};

#define AFX_MODULES_FILE  "afxmodules.conf"

void *LinkMonitorAfxThread(void *threadarg);
#define AFX_INTERFACE_FILE  "afxintf.conf"

#define OAM_INTERFACE  "ens3"

void *ServiceMonitorAfxThread(void *threadarg);
#define SERVICE_MONITOR_INTERVAL 30

void *BgpLoggingAfxThread(void *threadarg);
#define AFX_SYSLOG_FILE  "afxfilter.txt"
#define MAX_SYSLOG_WORDS 50

void *MeasureAfxThread(void *threadarg);

typedef struct linkstat {

  char linkname[32];
  char linkdescr[256];
  char linkmode[64];
  int  speedmbps;
  int  linkstat;

}LINKSTAT;


typedef struct dummy_vpp_metrics_struct {

  char linkname[32];
  char linkdescr[256];
  uint64_t rx_bytes;
  uint64_t tx_bytes;
  uint64_t rx_packets;
  uint64_t tx_packets;
  uint64_t rx_mcast;
  uint64_t delta_rx_bytes;
  uint64_t delta_rx_packets;
  uint64_t delta_rx_mcast;
  uint64_t delta_tx_bytes;
  uint64_t delta_tx_packets;

} VPP_METRICS_STRUCT;

void report_fault( char* evname, char *evid, EVEL_SEVERITIES sevty, char *categ, char *intf, char *trapname, char *descr, char *rem_router, char *routername, char *router_ip, int status );

const char *openstack_vm_uuid();
char *get_oam_intfaddr();

int file_is_modified(const char *path, time_t *oldMTime);
void remove_spaces(char* source);
char *escape_json(char *in);

extern char hostname[BUFSIZE];
extern char oam_intfaddr[BUFSIZE];
#endif
