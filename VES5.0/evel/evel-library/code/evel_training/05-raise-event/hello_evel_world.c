#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "evel.h"

int main(int argc, char ** argv)
{
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;
  EVENT_HEADER * heartbeat = NULL;
  EVENT_FAULT * fault = NULL;

  printf("\nHello AT&T Vendor Event world!\n");
  fflush(stdout);

  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s <FQDN>|<IP address> <port>\n", argv[0]);
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
                      "",                           /* Username              */
                      "",                           /* Password              */
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
  /* Raise a fault                                                           */
  /***************************************************************************/
  fault = evel_new_fault("My alarm condition",
                         "It broke very badly",
                         EVEL_PRIORITY_NORMAL,
                         EVEL_SEVERITY_MAJOR,
			EVEL_SOURCE_HOST,
                       EVEL_VF_STATUS_PREP_TERMINATE);
  if (fault != NULL)
  {
    printf("New fault created...\n");
    evel_rc = evel_post_event((EVENT_HEADER *)fault);
    if (evel_rc == EVEL_SUCCESS)
    {
      printf("Fault posted OK!\n");
    }
    else
    {
      printf("Post failed %d (%s)\n", evel_rc, evel_error_string());
    }
  }
  else
  {
    printf("New fault failed (%s)\n", evel_error_string());
  }

  /***************************************************************************/
  /* Terminate                                                               */
  /***************************************************************************/
  sleep(1);
  evel_terminate();
  printf("Terminated\n");

  return 0;
}
