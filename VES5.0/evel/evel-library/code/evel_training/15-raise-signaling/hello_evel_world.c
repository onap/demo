#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "evel.h"

/*****************************************************************************/
/* Local prototypes.                                                         */
/*****************************************************************************/
static void demo_signaling(void);

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
  /* Raise a Signaling event                                                 */
  /***************************************************************************/
  demo_signaling();

  /***************************************************************************/
  /* Terminate                                                               */
  /***************************************************************************/
  sleep(1);
  evel_terminate();
  printf("Terminated\n");

  return 0;
}

/**************************************************************************//**
 * Create and send a Signaling event.
 *****************************************************************************/
void demo_signaling(void)
{
  EVENT_SIGNALING * event = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

  event = evel_new_signaling("vendor_x_id",
             "correlator", "1.0.3.1", "1234", "192.168.1.3","3456");
  if (event != NULL)
  {
    evel_signaling_type_set(event, "Signaling");
    evel_signaling_vnfmodule_name_set(event, "vendor_x_module");
    evel_signaling_vnfname_set(event, "vendor_x_vnf");
    evel_signaling_correlator_set(event, "vendor_x_correlator");
    evel_signaling_local_ip_address_set(event, "1.0.3.1");
    evel_signaling_local_port_set(event, "1031");
    evel_signaling_remote_ip_address_set(event, "5.3.3.0");
    evel_signaling_remote_port_set(event, "5330");
    evel_signaling_compressed_sip_set(event, "compressed_sip");
    evel_signaling_summary_sip_set(event, "summary_sip");
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
