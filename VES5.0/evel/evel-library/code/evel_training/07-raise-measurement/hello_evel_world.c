#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "evel.h"

/*****************************************************************************/
/* Local prototypes.                                                         */
/*****************************************************************************/
static void demo_measurement(void);

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
  demo_measurement();

  /***************************************************************************/
  /* Terminate                                                               */
  /***************************************************************************/
  sleep(1);
  evel_terminate();
  printf("Terminated\n");

  return 0;
}

/**************************************************************************//**
 * Create and send a measurement event.
 *****************************************************************************/
void demo_measurement(void)
{
  EVENT_MEASUREMENT * measurement = NULL;
  MEASUREMENT_LATENCY_BUCKET * bucket = NULL;
  MEASUREMENT_VNIC_PERFORMANCE * vnic_use = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;
  MEASUREMENT_CPU_USE * cpu_use = NULL;

  /***************************************************************************/
  /* Measurement                                                             */
  /***************************************************************************/
  measurement = evel_new_measurement(5.5);
  if (measurement != NULL)
  {
    printf("New measurement created...\n");
    evel_measurement_type_set(measurement, "Perf management...");
    evel_measurement_conc_sess_set(measurement, 1);
    evel_measurement_cfg_ents_set(measurement, 2);
    evel_measurement_mean_req_lat_set(measurement, 4.4);
    evel_measurement_request_rate_set(measurement, 6);

  cpu_use = evel_measurement_new_cpu_use_add(measurement, "cpu1", 11.11);
  evel_measurement_cpu_use_idle_set(cpu_use,22.22);
  evel_measurement_cpu_use_interrupt_set(cpu_use,33.33);
  evel_measurement_cpu_use_nice_set(cpu_use,44.44);
  evel_measurement_cpu_use_softirq_set(cpu_use,55.55);
  evel_measurement_cpu_use_steal_set(cpu_use,66.66);
  evel_measurement_cpu_use_system_set(cpu_use,77.77);
  evel_measurement_cpu_use_usageuser_set(cpu_use,88.88);
  evel_measurement_cpu_use_wait_set(cpu_use,99.99);

  cpu_use = evel_measurement_new_cpu_use_add(measurement, "cpu2", 22.22);
  evel_measurement_cpu_use_idle_set(cpu_use,12.22);
  evel_measurement_cpu_use_interrupt_set(cpu_use,33.33);
  evel_measurement_cpu_use_nice_set(cpu_use,44.44);
  evel_measurement_cpu_use_softirq_set(cpu_use,55.55);
  evel_measurement_cpu_use_steal_set(cpu_use,66.66);
  evel_measurement_cpu_use_system_set(cpu_use,77.77);
  evel_measurement_cpu_use_usageuser_set(cpu_use,88.88);
  evel_measurement_cpu_use_wait_set(cpu_use,19.99);

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

    vnic_use = evel_measurement_new_vnic_performance("eth0", "true");
  evel_vnic_performance_rx_octets_delta_set(vnic_use, 47656465465);
  evel_vnic_performance_rx_total_pkt_acc_set(vnic_use, 4765764654444);
  evel_vnic_performance_rx_total_pkt_delta_set(vnic_use, 4545665656);
  evel_vnic_performance_rx_ucast_pkt_acc_set(vnic_use, 4765745546.);
  evel_vnic_performance_rx_ucast_pkt_delta_set(vnic_use, 4768765.78347856);
  evel_vnic_performance_tx_bcast_pkt_acc_set(vnic_use, 747665.347647);
  evel_vnic_performance_tx_bcast_pkt_delta_set(vnic_use, 3468765.4774);
  evel_vnic_performance_tx_discarded_pkt_acc_set(vnic_use, 53625345.53);
  evel_vnic_performance_tx_discarded_pkt_delta_set(vnic_use, 5465345.72455);
    evel_meas_vnic_performance_add(measurement, vnic_use);

    vnic_use = evel_measurement_new_vnic_performance("eth1", "false");
  evel_vnic_performance_rx_total_pkt_acc_set(vnic_use, 4765764654444);
  evel_vnic_performance_rx_total_pkt_delta_set(vnic_use, 4545665656);
  evel_vnic_performance_rx_ucast_pkt_acc_set(vnic_use, 4765745546.);
  evel_vnic_performance_rx_ucast_pkt_delta_set(vnic_use, 4768765.78347856);
  evel_vnic_performance_tx_bcast_pkt_acc_set(vnic_use, 747665.347647);
    evel_meas_vnic_performance_add(measurement, vnic_use);

    evel_measurement_errors_set(measurement, 1, 0, 2, 1);

    evel_measurement_feature_use_add(measurement, "FeatureA", 123);
    evel_measurement_feature_use_add(measurement, "FeatureB", 567);

    evel_measurement_codec_use_add(measurement, "G711a", 91);
    evel_measurement_codec_use_add(measurement, "G729ab", 92);

    evel_measurement_media_port_use_set(measurement, 1234);

    evel_measurement_vnfc_scaling_metric_set(measurement, 1234.5678);

    evel_measurement_custom_measurement_add(measurement,
                                            "Group1", "Name1", "Value1");
    evel_measurement_custom_measurement_add(measurement,
                                            "Group2", "Name1", "Value1");
    evel_measurement_custom_measurement_add(measurement,
                                            "Group2", "Name2", "Value2");

    evel_rc = evel_post_event((EVENT_HEADER *)measurement);
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

  printf("   Processed Measurement\n");
}
