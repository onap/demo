
/*************************************************************************//**
 *
 * Copyright © 2017 AT&T Intellectual Property. All rights reserved.
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
#include <math.h>

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

unsigned long long epoch_start = 0;


#ifdef DOCKER
int measure_traffic() 
{

  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;
  FILE *fp;
  int status;
  char count[10];
  time_t rawtime;
  struct tm * timeinfo;
  char period [21];
  char cmd [100];
  int concurrent_sessions = 0;
  int configured_entities = 0;
  double mean_request_latency = 0;
  double measurement_interval = 1;
  double memory_configured = 0;
  double memory_used = 0;
  int request_rate=0;
  char secs [3];
  int sec;
  double loadavg;

  printf("Checking app traffic\n");
  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime(period,21,"%d/%b/%Y:%H:%M:",timeinfo);
  strftime(secs,3,"%S",timeinfo);
  sec = atoi(secs);
  if (sec == 0) sec = 59;
  sprintf(secs, "%02d", sec);
  strncat(period, secs, 21);
  // ....x....1....x....2.
  // 15/Oct/2016:17:51:19
  strcpy(cmd, "sudo docker logs vHello | grep -c ");
  strncat(cmd, period, 100);

  fp = popen(cmd, "r");
  if (fp == NULL) {
    EVEL_ERROR("popen failed to execute command");
  }

  if (fgets(count, 10, fp) != NULL) {
    request_rate = atoi(count);
    printf("Reporting request rate for second: %s as %d\n", period, request_rate);

    }
    else {
      EVEL_ERROR("New Measurement failed");
    }
    printf("Processed measurement\n");
  
  status = pclose(fp);
  if (status == -1) {
    EVEL_ERROR("pclose returned an error");
  }
  return request_rate;
}

#endif


int main(int argc, char** argv)
{
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;
  EVENT_MEASUREMENT* vpp_m = NULL;
  EVENT_HEADER* vpp_m_header = NULL;
  int bytes_in_this_round;
  int bytes_out_this_round;
  int packets_in_this_round;
  int packets_out_this_round;
  vpp_metrics_struct* last_vpp_metrics = malloc(sizeof(vpp_metrics_struct));
  vpp_metrics_struct* curr_vpp_metrics = malloc(sizeof(vpp_metrics_struct));
  struct timeval time_val;
  //time_t start_epoch;
  //time_t last_epoch;
  char hostname[BUFSIZE];
  char* fqdn = argv[1];
  int port = atoi(argv[2]);
  char* vnic = argv[3];
  MEASUREMENT_VNIC_PERFORMANCE * vnic_performance = NULL;

  printf("\nVector Packet Processing (VPP) measurement collection\n");
  fflush(stdout);

  if (argc != 4)
  {
    fprintf(stderr, "Usage: %s <FQDN>|<IP address> <port> <interface>\n", argv[0]);
    exit(-1);
  }
  srand(time(NULL));

  /**************************************************************************/
  /* Initialize                                                             */
  /**************************************************************************/
  if(evel_initialize(fqdn,                         /* FQDN                  */
                     port, 	                   /* Port                  */
                     NULL,                         /* optional path         */
                     NULL,                         /* optional topic        */
                     0,                            /* HTTPS?                */
                     "",                           /* Username              */
                     "",                           /* Password              */
                     EVEL_SOURCE_VIRTUAL_MACHINE,  /* Source type           */
                     "vLoadBalancer",              /* Role                  */
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
  epoch_start = time_val.tv_sec * 1000000 + time_val.tv_usec;
  sleep(READ_INTERVAL);

  /***************************************************************************/
  /* Collect metrics from the VNIC                                           */
  /***************************************************************************/
  while(1) {
    // Read the number of vDNSs currently active
    int active_dns = 0;
    FILE* fd = fopen("active_dns.txt", "r");
    while(!feof(fd)) {
      if(fscanf(fd, "%d", &active_dns) != 1) /* Avoid infinite loops if the file doesn't contain exactly one number */
        break;
    }

    if(fclose(fd))  {
      printf("Error when closing file\n");
      return 1;
    }

    memset(curr_vpp_metrics, 0, sizeof(vpp_metrics_struct));
    read_vpp_metrics(curr_vpp_metrics, vnic);

    if(active_dns > 0 && (curr_vpp_metrics->bytes_in - last_vpp_metrics->bytes_in > 0)) {
      bytes_in_this_round = (int) round((curr_vpp_metrics->bytes_in - last_vpp_metrics->bytes_in) / active_dns);
    }
    else {
      bytes_in_this_round = 0;
    }
    if(active_dns > 0 && (curr_vpp_metrics->bytes_out - last_vpp_metrics->bytes_out > 0)) {
      bytes_out_this_round = (int) round((curr_vpp_metrics->bytes_out - last_vpp_metrics->bytes_out) / active_dns);
    }
    else {
      bytes_out_this_round = 0;
    }
    if(active_dns > 0 && (curr_vpp_metrics->packets_in - last_vpp_metrics->packets_in > 0)) {
      packets_in_this_round = (int) round((curr_vpp_metrics->packets_in - last_vpp_metrics->packets_in) / active_dns);
    }
    else {
      packets_in_this_round = 0;
    }
    if(active_dns > 0 && (curr_vpp_metrics->packets_out - last_vpp_metrics->packets_out > 0)) {
      packets_out_this_round = (int) round((curr_vpp_metrics->packets_out - last_vpp_metrics->packets_out) / active_dns);
    }
    else {
      packets_out_this_round = 0;
    }

    vpp_m = evel_new_measurement(READ_INTERVAL,"vLoadBalancer","TrafficStats_1.2.3.4");
    vnic_performance = (MEASUREMENT_VNIC_PERFORMANCE *)evel_measurement_new_vnic_performance("eth0", "true");
    evel_meas_vnic_performance_add(vpp_m, vnic_performance);

    if(vpp_m != NULL) {
      printf("New measurement report created...\n");

      evel_measurement_type_set(vpp_m, "HTTP request rate");
      evel_measurement_request_rate_set(vpp_m, rand()%10000);

      evel_vnic_performance_rx_total_pkt_delta_set(vnic_performance, packets_in_this_round);
      evel_vnic_performance_tx_total_pkt_delta_set(vnic_performance, packets_out_this_round);

      evel_vnic_performance_rx_octets_delta_set(vnic_performance, bytes_in_this_round);
      evel_vnic_performance_tx_octets_delta_set(vnic_performance, bytes_out_this_round);

      /***************************************************************************/
      /* Set parameters in the MEASUREMENT header packet                         */
      /***************************************************************************/
      struct timeval tv_now;
      gettimeofday(&tv_now, NULL);
      unsigned long long epoch_now = tv_now.tv_usec + 1000000 * tv_now.tv_sec;

      //last_epoch = start_epoch + READ_INTERVAL * 1000000;
      vpp_m_header = (EVENT_HEADER *)vpp_m;
      //vpp_m_header->start_epoch_microsec = start_epoch;
      //vpp_m_header->last_epoch_microsec = last_epoch;
      evel_start_epoch_set(&vpp_m->header, epoch_start);
      evel_last_epoch_set(&vpp_m->header, epoch_now);
      epoch_start = epoch_now;

      evel_nfcnamingcode_set(&vpp_m->header, "vVNF");
      evel_nfnamingcode_set(&vpp_m->header, "vVNF");
      //strcpy(vpp_m_header->reporting_entity_id.value, "No UUID available");
      //strcpy(vpp_m_header->reporting_entity_name, hostname);
      evel_reporting_entity_name_set(&vpp_m->header, "lbll");
      evel_reporting_entity_id_set(&vpp_m->header, "No UUID available");
      evel_rc = evel_post_event(vpp_m_header);

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
    //gettimeofday(&time_val, NULL);
    //start_epoch = time_val.tv_sec * 1000000 + time_val.tv_usec;

    sleep(READ_INTERVAL);
  }

  /***************************************************************************/
  /* Terminate                                                               */
  /***************************************************************************/
  sleep(1);
  evel_free_measurement(vpp_m);
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
