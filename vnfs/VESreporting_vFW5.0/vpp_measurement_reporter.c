
/*************************************************************************//**
 *
 * Copyright © 2019 AT&T Intellectual Property. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "evel.h"

#define BUFSIZE 128
#define READ_INTERVAL 10

typedef struct dummy_vpp_metrics_struct {
  int bytes_in;
  int bytes_out;
  int packets_in;
  int packets_out;
} vpp_metrics_struct;

void read_vpp_metrics(vpp_metrics_struct *, char *);

int main(int argc, char** argv)
{
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;
  EVENT_MEASUREMENT* vpp_m = NULL;
  EVENT_HEADER* vpp_m_header = NULL;
  EVENT_HEADER* batch_header = NULL;
  MEASUREMENT_VNIC_PERFORMANCE * vnic_performance = NULL;
  int bytes_in_this_round;
  int bytes_out_this_round;
  int packets_in_this_round;
  int packets_out_this_round;
  vpp_metrics_struct* last_vpp_metrics = malloc(sizeof(vpp_metrics_struct));
  vpp_metrics_struct* curr_vpp_metrics = malloc(sizeof(vpp_metrics_struct));
  struct timeval time_val;
  time_t start_epoch;
  time_t last_epoch;
  char hostname[BUFSIZE];
  char eventName[BUFSIZE];
  char eventId[BUFSIZE];
  char* fqdn2 = NULL;
  int port2 = 0;
  char * vnic = NULL;
  memset(eventName, 0, BUFSIZE);
  memset(eventId, 0, BUFSIZE);
  memset(hostname, 0, BUFSIZE);

  strcpy(eventName, "vFirewallBroadcastPackets");
  strcpy(eventId, "mvfs00000001");

  char* fqdn = argv[1];
  int port = atoi(argv[2]);
  if(argc == 6)
  {
     fqdn2 = argv[3];
     port2 = atoi(argv[4]);
     vnic = argv[5];
  }
  else
     vnic = argv[3];

  printf("\nVector Packet Processing (VPP) measurement collection\n");
  fflush(stdout);

  if (!((argc == 4) || (argc == 6)))
  {
    fprintf(stderr, "Usage: %s <FQDN>|<IP address> <port> <FQDN>|<IP address> <port> <interface> \n", argv[0]);
    fprintf(stderr, "OR\n");
    fprintf(stderr, "Usage: %s <FQDN>|<IP address> <port> <interface>\n", argv[0]);
    exit(-1);
  }

  /**************************************************************************/
  /* Initialize                                                             */
  /**************************************************************************/
  if(evel_initialize(fqdn,                         /* FQDN                  */
                     port, 	                   /* Port                  */
                     fqdn2, 	                   /* Backup FQDN           */
                     port2, 	                   /* Backup port           */
                     NULL,                         /* optional path         */
                     NULL,                         /* optional topic        */
                     100,                          /* Ring Buffer size      */
                     0,                            /* HTTPS?                */
                     NULL,                         /* cert file             */
                     NULL,                         /* key  file             */
                     NULL,                         /* ca   info             */
                     NULL,                         /* ca   file             */
                     0,                            /* verify peer           */
                     0,                            /* verify host           */
                     "sample1",                    /* Username              */
                     "sample1",                    /* Password              */
                     "sample1",                    /* Username2             */
                     "sample1",                    /* Password2             */
                     NULL,                         /* Source ip             */
                     NULL,                         /* Source ip2            */
                     EVEL_SOURCE_VIRTUAL_MACHINE,  /* Source type           */
                     "vFirewall",      	           /* Role                  */
                     1))                           /* Verbosity             */

  {
    fprintf(stderr, "\nFailed to initialize the EVEL library!!!\n");
    exit(-1);
  }
  else
  {
    printf("\nInitialization completed\n");
  }

  gethostname(hostname, BUFSIZE);
  memset(last_vpp_metrics, 0, sizeof(vpp_metrics_struct));
  read_vpp_metrics(last_vpp_metrics, vnic);
  gettimeofday(&time_val, NULL);
  start_epoch = time_val.tv_sec * 1000000 + time_val.tv_usec;
  sleep(READ_INTERVAL);

  /***************************************************************************/
  /* Collect metrics from the VNIC                                           */
  /***************************************************************************/
  while(1) {
    memset(curr_vpp_metrics, 0, sizeof(vpp_metrics_struct));
    read_vpp_metrics(curr_vpp_metrics, vnic);

    if(curr_vpp_metrics->bytes_in - last_vpp_metrics->bytes_in > 0) {
      bytes_in_this_round = curr_vpp_metrics->bytes_in - last_vpp_metrics->bytes_in;
    }
    else {
      bytes_in_this_round = 0;
    }
    if(curr_vpp_metrics->bytes_out - last_vpp_metrics->bytes_out > 0) {
      bytes_out_this_round = curr_vpp_metrics->bytes_out - last_vpp_metrics->bytes_out;
    }
    else {
      bytes_out_this_round = 0;
    }
    if(curr_vpp_metrics->packets_in - last_vpp_metrics->packets_in > 0) {
      packets_in_this_round = curr_vpp_metrics->packets_in - last_vpp_metrics->packets_in;
    }
    else {
      packets_in_this_round = 0;
    }
    if(curr_vpp_metrics->packets_out - last_vpp_metrics->packets_out > 0) {
      packets_out_this_round = curr_vpp_metrics->packets_out - last_vpp_metrics->packets_out;
    }
    else {
      packets_out_this_round = 0;
    }

    vpp_m = evel_new_measurement(READ_INTERVAL, eventName, eventId);

    if(vpp_m != NULL) {
      printf("New measurement report created...\n");
      vnic_performance = (MEASUREMENT_VNIC_PERFORMANCE *)evel_measurement_new_vnic_performance(vnic, "true");
      evel_meas_vnic_performance_add(vpp_m, vnic_performance);
      evel_vnic_performance_rx_total_pkt_delta_set(vnic_performance, packets_in_this_round);
      evel_vnic_performance_tx_total_pkt_delta_set(vnic_performance, packets_out_this_round);

      evel_vnic_performance_rx_octets_delta_set(vnic_performance, bytes_in_this_round);
      evel_vnic_performance_tx_octets_delta_set(vnic_performance, bytes_out_this_round);

      /***************************************************************************/
      /* Set parameters in the MEASUREMENT header packet                         */
      /***************************************************************************/
      last_epoch = start_epoch + READ_INTERVAL * 1000000;
      vpp_m_header = (EVENT_HEADER *)vpp_m;
      vpp_m_header->start_epoch_microsec = start_epoch;
      vpp_m_header->last_epoch_microsec = last_epoch;
      evel_reporting_entity_id_set(vpp_m_header, "No UUID available");
      printf("1111\n");
      evel_reporting_entity_name_set(vpp_m_header, hostname);
      printf("1111\n");
      // evel_rc = evel_post_event(vpp_m_header);
      batch_header = evel_new_batch("batch_event_name", "bevent_id");
      evel_batch_add_event(batch_header, vpp_m_header);
      evel_rc = evel_post_event(batch_header);

      if(evel_rc == EVEL_SUCCESS) {
        printf("Measurement report correctly sent to the collector!\n");
      }
      else {
        printf("Post failed %d (%s)\n", evel_rc, evel_error_string());
      }
    }
    else {
      printf("New measurement report failed (%s)\n", evel_error_string());
    }

    last_vpp_metrics->bytes_in = curr_vpp_metrics->bytes_in;
    last_vpp_metrics->bytes_out = curr_vpp_metrics->bytes_out;
    last_vpp_metrics->packets_in = curr_vpp_metrics->packets_in;
    last_vpp_metrics->packets_out = curr_vpp_metrics->packets_out;
    gettimeofday(&time_val, NULL);
    start_epoch = time_val.tv_sec * 1000000 + time_val.tv_usec;

    sleep(READ_INTERVAL);
  }

  /***************************************************************************/
  /* Terminate                                                               */
  /***************************************************************************/
  sleep(1);
  free(last_vpp_metrics);
  free(curr_vpp_metrics);
  evel_terminate();
  printf("Terminated\n");

  return 0;
}

void read_vpp_metrics(vpp_metrics_struct *vpp_metrics, char *vnic) {
  // Define an array of char that contains the parameters of the unix 'cut' command
  char* params[] = {"-f3", "-f11", "-f4", "-f12"};
  // Define the unix command to execute in order to read metrics from the vNIC
  char* cmd_prefix = "sudo cat /proc/net/dev | grep \"";
  char* cmd_mid = "\" | tr -s \' \' | cut -d\' \' ";
  char cmd[BUFSIZE];
  // Define other variables
  char buf[BUFSIZE];		/* buffer used to store VPP metrics 	*/
  int temp[] = {0, 0, 0, 0};	/* temp array that contains VPP values 	*/
  FILE *fp;			/* file descriptor to pipe cmd to shell */
  int i;

  for(i = 0; i < 4; i++) {
    // Clear buffers
    memset(buf, 0, BUFSIZE);
    memset(cmd, 0, BUFSIZE);
    // Build shell command to read metrics from the vNIC
    strcat(cmd, cmd_prefix);
    strcat(cmd, vnic);
    strcat(cmd, cmd_mid);
    strcat(cmd, params[i]);
    
    // Open a pipe and read VPP values
    if ((fp = popen(cmd, "r")) == NULL) {
        printf("Error opening pipe!\n");
        return;
    }

    while (fgets(buf, BUFSIZE, fp) != NULL);
    temp[i] = atoi(buf);

    if(pclose(fp))  {
        printf("Command not found or exited with error status\n");
        return;
    }
  }

  // Store metrics read from the vNIC in the struct passed from the main function
  vpp_metrics->bytes_in = temp[0];
  vpp_metrics->bytes_out = temp[1];
  vpp_metrics->packets_in = temp[2];
  vpp_metrics->packets_out = temp[3];
}
