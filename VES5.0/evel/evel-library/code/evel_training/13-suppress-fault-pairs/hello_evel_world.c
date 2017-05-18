#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "evel.h"
#include "evel_test_control.h"

/*****************************************************************************/
/* Local prototypes.                                                         */
/*****************************************************************************/
static void demo_heartbeat();
static void demo_fault(void);

int main(int argc, char ** argv)
{
  printf("\nHello AT&T Vendor Event world!\n");
  fflush(stdout);

  if (argc != 5)
  {
    fprintf(stderr,
            "Usage: %s <FQDN>|<IP address> <port> "
            "<username> <password>\n", argv[0]);
    exit(-1);
  }

  char * api_fqdn = argv[1];
  int api_port = atoi(argv[2]);
  int api_secure = 0;

  /***************************************************************************/
  /* Initialize                                                              */
  /***************************************************************************/
  if (evel_initialize(api_fqdn,                     /* FQDN                  */
                      api_port,                     /* Port                  */
                      NULL,                         /* optional path         */
                      NULL,                         /* optional topic        */
                      api_secure,                   /* HTTPS?                */
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
  /* Raise a fault.                                                          */
  /***************************************************************************/
  printf("Raise normal fault ...\n");
  fflush(stdout);
  demo_fault();
  sleep(1);

  /***************************************************************************/
  /* Ask for suppression of pairs (under "alarmAdditionalInformation") with  */
  /* names "name1" and "name2".                                              */
  /***************************************************************************/
  printf("TestControl: test collector suppress fault pairs\n");
  fflush(stdout);
  evel_test_control_scenario(TC_FAULT_SUPPRESS_PAIRS,
                             api_secure,
                             api_fqdn,
                             api_port);
  sleep(1);

  /***************************************************************************/
  /* Heartbeat to pick up the suppression change.                            */
  /***************************************************************************/
  demo_heartbeat();

  /***************************************************************************/
  /* Raise the same fault.                                                   */
  /***************************************************************************/
  printf("Raise normal fault ...\n");
  fflush(stdout);
  demo_fault();
  sleep(1);

  /***************************************************************************/
  /* Ask for removal of fault suppression.                                   */
  /***************************************************************************/
  printf("TestControl: test collector remove fault suppression\n");
  fflush(stdout);
  evel_test_control_scenario(TC_FAULT_SUPPRESS_NOTHING,
                             api_secure,
                             api_fqdn,
                             api_port);
  sleep(1);

  /***************************************************************************/
  /* Heartbeat to pick up the suppression change.                            */
  /***************************************************************************/
  demo_heartbeat();

  /***************************************************************************/
  /* Raise the same fault.                                                   */
  /***************************************************************************/
  printf("Raise normal fault ...\n");
  fflush(stdout);
  demo_fault();
  sleep(1);

  /***************************************************************************/
  /* Terminate                                                               */
  /***************************************************************************/
  sleep(1);
  evel_terminate();
  printf("Terminated\n");

  return 0;
}

/**************************************************************************//**
 * Create and send a heatbeat.
 *****************************************************************************/
void demo_heartbeat()
{
  EVENT_HEADER * heartbeat = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

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
}

/**************************************************************************//**
 * Create and send a fault event.
 *****************************************************************************/
void demo_fault(void)
{
  EVENT_FAULT * fault = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

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
    evel_fault_addl_info_add(fault, "name3", "value3");
    evel_fault_addl_info_add(fault, "name4", "value4");
    evel_rc = evel_post_event((EVENT_HEADER *)fault);
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

  printf("   Processed Fault\n");
}
