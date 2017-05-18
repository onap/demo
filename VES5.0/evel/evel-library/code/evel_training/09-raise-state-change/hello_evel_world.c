#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "evel.h"

/*****************************************************************************/
/* Local prototypes.                                                         */
/*****************************************************************************/
static void demo_state_change(void);

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
  /* Raise a state change                                                    */
  /***************************************************************************/
  demo_state_change();

  /***************************************************************************/
  /* Terminate                                                               */
  /***************************************************************************/
  sleep(1);
  evel_terminate();
  printf("Terminated\n");

  return 0;
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

  printf("   Processed State Change\n");
}
