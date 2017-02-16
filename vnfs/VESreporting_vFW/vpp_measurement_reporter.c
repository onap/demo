
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
  char* fqdn = argv[1];
  int port = atoi(argv[2]);
  char* vnic = argv[3];

  printf("\nVector Packet Processing (VPP) measurement collection\n");
  fflush(stdout);

  if (argc != 4)
  {
    fprintf(stderr, "Usage: %s <FQDN>|<IP address> <port> <interface>\n", argv[0]);
    exit(-1);
  }

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

    vpp_m = evel_new_measurement(READ_INTERVAL);

    if(vpp_m != NULL) {
      printf("New measurement report created...\n");
      evel_measurement_vnic_use_add(vpp_m,		/* Pointer to the measurement      */ 
  			             vnic, 		/* ASCII string with the vNIC's ID */
	            packets_in_this_round, 		/* Packets received      	   */
	           packets_out_this_round,		/* Packets transmitted   	   */
                                        0,		/* Broadcast packets received      */
                                        0,		/* Broadcast packets transmitted   */
		      bytes_in_this_round,		/* Total bytes received            */
		     bytes_out_this_round, 		/* Total bytes transmitted         */
				        0, 		/* Multicast packets received      */
				        0, 		/* Multicast packets transmitted   */
				        0, 		/* Unicast packets received        */
      				        0);		/* Unicast packets transmitted     */

      /***************************************************************************/
      /* Set parameters in the MEASUREMENT header packet                         */
      /***************************************************************************/
      last_epoch = start_epoch + READ_INTERVAL * 1000000;
      vpp_m_header = (EVENT_HEADER *)vpp_m;
      vpp_m_header->start_epoch_microsec = start_epoch;
      vpp_m_header->last_epoch_microsec = last_epoch;
      strcpy(vpp_m_header->reporting_entity_id.value, "No UUID available");
      strcpy(vpp_m_header->reporting_entity_name, hostname);
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
