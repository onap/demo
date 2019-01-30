 /*****************************************************************************//***
 * Copyright(c) <2017>, AT&T Intellectual Property.  All other rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:  This product includes
 *    software developed by the AT&T.
 * 4. Neither the name of AT&T nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY AT&T INTELLECTUAL PROPERTY ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AT&T INTELLECTUAL PROPERTY BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "evel.h"
#include "afx_ves_reporter.h"

/**************************************************************************//**
 * Monitor AFX services for current status
 * returns success status 1 or failure 0.
 *
 * @param cmd       Pointer to Shell cmd to monitor status.
 * @param dtls      Pointer to details of service
 *                  Returns the start time of service
 *                  
 * @retvalue  int   Success 1 service running 0 service inactive 
 *****************************************************************************/
int monitor_afx_service( char *cmd, char *detls, char *svc )
{
    FILE *fp;
    char line[256];
    int n_spaces = 0, i;
    char ** res  = NULL;
    char *p;
    int ret = 0;
    int lnno = 0;


  fp = popen(cmd,"r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    return 0;
  }

  /* Read the output a line at a time - output it. */
  while (fgets(line, sizeof(line)-1, fp) != NULL) {
    //printf("%s",line);
    lnno++;

n_spaces = 0;
p    = strtok (line, " ");
/* split string and append tokens to 'res' */

while (p) {
  res = realloc (res, sizeof (char*) * ++n_spaces);

  if (res == NULL)
    exit (-1); /* memory allocation failed */

  res[n_spaces-1] = p;

  p = strtok (NULL, " ");
}

/* realloc one extra element for the last NULL */
res = realloc (res, sizeof (char*) * (n_spaces+1));
res[n_spaces] = 0;

if( n_spaces > 2 && lnno == 1 )
{
  EVEL_DEBUG("AFX: Service %s\n",res[3]);
  sprintf(svc,"%s",res[3]);
  svc[strlen(svc)-1] = 0;
}

/* print the result */
if( n_spaces > 2 && !strcmp(res[0],"Active:") )
{
  //for (i = 0; i < (n_spaces+1); ++i)
  //{
  // printf ("res[%d] = %s\n", i, res[i]);
  //}
  if(!strncmp(res[1],"activ",5)){
     EVEL_DEBUG("AFX: Service Active\n");
     ret |= 1;
  } else {
     EVEL_DEBUG("AFX: Service Inactive\n");
     ret &= 0;
  }
}

  }

/* free the memory allocated */
if(res != NULL) free (res);

  /* close */
  pclose(fp);

return ret;

}

/**************************************************************************//**
 * Thread function to Monitor AFX services
 * Checks status of afx inout and afx output service every service monitor
 * interval. It reports fault event at startup and whenever status changes
 *
 * @param threadarg  Thread arguments for startup message
 *
 * @retvalue  int   Success 1 service running 0 service inactive
 *****************************************************************************/
void *ServiceMonitorAfxThread(void *threadarg)
{
   int taskid, sum;
   char *hello_msg;
   struct thread_data *my_data;
   char cmd[256];
   char details[256];
   char *afxin = "afx@in*";
   char *afxout = "afx@out*";
   int instate = -1;
   int outstate = -1;
   int tmpstate;
   char evid[128];
   char svc[128];
   int is_critical = 0;

   sleep(1);
   my_data = (struct thread_data *) threadarg;
   taskid = my_data->thread_id;
   sum = my_data->sum;
   hello_msg = my_data->message;
   EVEL_INFO("AFX: Service Monitor Thread %d: %s  Sum=%d\n", taskid, hello_msg, sum);

while(1)
{
    /* invokes afx service monitor function and collects return value */
    sprintf(cmd,"/bin/systemctl status %s ",afxin);
    tmpstate = monitor_afx_service( cmd, details,svc );
    if( tmpstate != instate ){
       printf("New afxin Service report sent %d\n",tmpstate);
       instate = tmpstate;
       if( instate == 1){
         sprintf(evid,"Fault_vAfx_SvcInput_%s",oam_intfaddr);
         if (is_critical == 1){
           is_critical = 0;
           sprintf(evid,"Fault_vAfx_SvcOutput_SvcInput_%s",oam_intfaddr);
           report_fault("Fault_vAfx_Svc_input_Output",evid,
                      EVEL_SEVERITY_NORMAL, "routing", "AFXInput and AFXOutput", 
                      "AFX services input and output down alarm", 
                      details, NULL, NULL, NULL, instate);
         }
         sprintf(evid,"Fault_vAfx_SvcInput_%s",oam_intfaddr);
         report_fault("Fault_vAfx_SvcInput",evid,EVEL_SEVERITY_NORMAL, "routing", "AFXInput", "AFX services input up alarm", details, NULL, NULL, NULL, instate);
       }else{
         sprintf(evid,"Fault_vAfx_SvcInput_%s",oam_intfaddr);
         report_fault("Fault_vAfx_SvcInput",evid,EVEL_SEVERITY_MAJOR, "routing", "AFXInput", "AFX services input down alarm", details, NULL, NULL, NULL, instate);
       }
    }

    /* invokes afx service monitor function and collects return value */
    sprintf(cmd,"/bin/systemctl status %s ",afxout);
    tmpstate = monitor_afx_service( cmd, details, svc );
    if( tmpstate != outstate ){
       printf("New afxout Service report sent %d\n",tmpstate);
       outstate = tmpstate;
       if( outstate == 1){
         if (is_critical == 1){
           is_critical = 0;
           sprintf(evid,"Fault_vAfx_SvcOutput_SvcInput_%s",oam_intfaddr);
           report_fault("Fault_vAfx_Svc_input_Output",evid,
                      EVEL_SEVERITY_NORMAL, "routing", "AFXInput and AFXOutput", 
                      "AFX services input and output down alarm", 
                      details, NULL, NULL, NULL, outstate);
           }
         sprintf(evid,"Fault_vAfx_SvcOutput_%s",oam_intfaddr);
         report_fault("Fault_vAfx_SvcOutput",evid,EVEL_SEVERITY_NORMAL, "routing", "AFXOutput", "AFX services output up alarm", details, NULL, NULL, NULL, outstate );
       } else {
         sprintf(evid,"Fault_vAfx_SvcOutput_%s",oam_intfaddr);
         report_fault("Fault_vAfx_SvcOutput",evid,EVEL_SEVERITY_MAJOR, "routing", "AFXOutput", "AFX services output down alarm", details, NULL, NULL, NULL, outstate);
       }
    }
    if((instate == 0) && (outstate == 0) && (is_critical == 0))
    {
         is_critical = 1;
          
         sprintf(evid,"Fault_vAfx_SvcOutput_SvcInput_%s",oam_intfaddr);
         report_fault("Fault_vAfx_Svc_input_Output",evid,EVEL_SEVERITY_CRITICAL,
                      "routing", "AFXInput and AFXOutput", 
                      "AFX services input and output down alarm", details, 
                      NULL, NULL, NULL, instate);
    }

    sleep(SERVICE_MONITOR_INTERVAL);
}

   pthread_exit(NULL);
}


