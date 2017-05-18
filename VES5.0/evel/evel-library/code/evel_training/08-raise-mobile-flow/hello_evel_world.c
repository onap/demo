#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "evel.h"

/*****************************************************************************/
/* Local prototypes.                                                         */
/*****************************************************************************/
static void demo_mobile_flow(void);

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
  /* Raise a measurement                                                     */
  /***************************************************************************/
  demo_mobile_flow();

  /***************************************************************************/
  /* Terminate                                                               */
  /***************************************************************************/
  sleep(1);
  evel_terminate();
  printf("Terminated\n");

  return 0;
}

/**************************************************************************//**
 * Create and send three mobile flow events.
 *****************************************************************************/
void demo_mobile_flow(void)
{
  MOBILE_GTP_PER_FLOW_METRICS * metrics = NULL;
  EVENT_MOBILE_FLOW * mobile_flow = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;

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
    printf("   Processed full Mobile Flow\n");
  }
  else
  {
    printf("New GTP Per Flow Metrics failed\n");
  }
}
