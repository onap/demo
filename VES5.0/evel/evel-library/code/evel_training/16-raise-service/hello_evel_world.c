#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "evel.h"

/*****************************************************************************/
/* Local prototypes.                                                         */
/*****************************************************************************/
static void demo_service(void);

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
  /* Raise a service event                                                   */
  /***************************************************************************/
  demo_service();

  /***************************************************************************/
  /* Terminate                                                               */
  /***************************************************************************/
  sleep(1);
  evel_terminate();
  printf("Terminated\n");

  return 0;
}

/**************************************************************************//**
 * Create and send a Service event.
 *****************************************************************************/
void demo_service(void)
{
  EVENT_SIGNALING * event = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  event = evel_new_signaling("vendor_x_id",
				"correlator", "1.0.3.1", "1234", "192.168.1.3","3456");
  if (event != NULL)
  {
    evel_signaling_vnfmodule_name_set(event, "vendor_x_module");
    evel_signaling_vnfname_set(event, "vendor_x_vnf");
    evel_signaling_type_set(event, "Service Event");
    evel_signaling_correlator_set(event, "vendor_x_correlator");
    evel_signaling_addl_info_add(event, "prodid", "vendor_x_product_id");
    evel_signaling_addl_info_add(event, "subsys", "vendor_x_subsystem_id");
    evel_signaling_addl_info_add(event, "friendly_name", "vendor_x_friend");

    evel_signaling_addl_info_add(event, "callee", "PCMA");
    evel_signaling_addl_info_add(event, "caller", "G729A");

    evel_signaling_addl_info_add(event, "Name1", "Value1");
    evel_signaling_addl_info_add(event, "Name2", "Value2");

    evel_rc = evel_post_event((EVENT_HEADER *) event);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }
  else
  {
    EVEL_ERROR("New Service failed");
  }
  printf("   Processed Service Events\n");
}
