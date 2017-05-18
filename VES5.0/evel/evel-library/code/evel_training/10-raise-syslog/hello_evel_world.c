#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "evel.h"

/*****************************************************************************/
/* Local prototypes.                                                         */
/*****************************************************************************/
static void demo_syslog(void);

int main(int argc, char ** argv)
{
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;
  EVENT_HEADER * heartbeat = NULL;

  printf("\nHello AT&T Vendor Event world!\n");
  fflush(stdout);

  if (argc != 5)
  {
    fprintf(stderr,
            "Usage: %s <FQDN>|<IP address> <port> "
            "<username> <password>\n", argv[0]);
    exit(-1);
  }

  /***************************************************************************/
  /* Initialize                                                              */
  /***************************************************************************/
  if (evel_initialize(argv[1],                      /* FQDN                  */
                      atoi(argv[2]),                /* Port                  */
                      NULL,                         /* optional path         */
                      NULL,                         /* optional topic        */
                      0,                            /* HTTPS?                */
                      argv[3],                      /* Username              */
                      argv[4],                      /* Password              */
                      EVEL_SOURCE_VIRTUAL_MACHINE,  /* Source type           */
                      "EVEL training demo",         /* Role                  */
                      0))                           /* Verbosity             */
  {
    fprintf(stderr, "\nFailed to initialize the EVEL library!!!\n");
    exit(-1);
  }
  else
  {
    printf("\nInitialization completed\n");
  }

  /***************************************************************************/
  /* Send a heartbeat just to show we're alive!                              */
  /***************************************************************************/
  heartbeat = evel_new_heartbeat();
  if (heartbeat != NULL)
  {
    evel_rc = evel_post_event(heartbeat);
    if (evel_rc != EVEL_SUCCESS)
    {
      printf("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }
  else
  {
    printf("New heartbeat failed");
  }

  /***************************************************************************/
  /* Raise a syslog                                                          */
  /***************************************************************************/
  demo_syslog();

  /***************************************************************************/
  /* Terminate                                                               */
  /***************************************************************************/
  sleep(1);
  evel_terminate();
  printf("Terminated\n");

  return 0;
}

/**************************************************************************//**
 * Create and send a syslog event.
 *****************************************************************************/
void demo_syslog(void)
{
  EVENT_SYSLOG * syslog = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

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
    evel_rc = evel_post_event((EVENT_HEADER *)syslog);
    if (evel_rc == EVEL_SUCCESS)
    {
      printf("Post OK!\n");
    }
    else
    {
      printf("Post Failed %d (%s)\n", evel_rc, evel_error_string());
    }
  }
  else
  {
    printf("Failed to create event (%s)\n", evel_error_string());
  }

  printf("   Processed Syslog\n");
}
