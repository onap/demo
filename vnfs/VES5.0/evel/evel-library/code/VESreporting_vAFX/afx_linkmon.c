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
 * Thread function to Monitor Ethernet links
 * Checks status of afx inout and afx output service every service monitor
 * interval. It reports fault event at startup and whenever status changes
 *
 * @param threadarg  Thread arguments for startup message
 *
 * @retvalue  int   Success 1 service running 0 service inactive
 *****************************************************************************/
void *LinkMonitorAfxThread(void *threadarg)
{
   int taskid, sum;
   char *hello_msg;
   struct thread_data *my_data;
    FILE *fp;
    char const* const fileName = AFX_INTERFACE_FILE;
    char line[256];
    char cmd[256];
    char intf[256];
    char descr[256];
    char linkdata[256];
    int linkcount = 0;
    char *search = ":";
    char *token;
    char *pch;
    time_t filetime = 0;
    int newrun = 1;
    int n_spaces = 0, i;
    char ** res  = NULL;
    char *pos;
    int tlinkstat;
    LINKSTAT intfstats[MAX_INTERFACES];
    char vesevid[128];
    int status = 1; /** VFS Status is Active **/


   sleep(1);
   my_data = (struct thread_data *) threadarg;
   taskid = my_data->thread_id;
   sum = my_data->sum;
   hello_msg = my_data->message;
   EVEL_INFO("AFX: Linkmon Thread %d: %s  Sum=%d\n", taskid, hello_msg, sum);

while(1)
{
    /*Check if file is modified recently */
    if( file_is_modified(fileName, &filetime) )  newrun = 1;

    FILE* file = fopen(fileName, "r"); /* should check the result */
    if( file == NULL ){
      EVEL_ERROR("AFX: Linkmon Interface config file unavailable\n");
      pthread_exit(NULL);
    }

    while ( file != NULL && fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
        sscanf(line,"%s %s",intf,descr);
	//remove_spaces(line);
        if ((pos=strchr(line, '\n')) != NULL)
           *pos = '\0';
        //printf("%s", line);
        sprintf(cmd,"/sbin/ethtool %s\n", intf);
        //printf("%s", cmd);
        if( newrun ){
                strcpy(intfstats[linkcount].linkname,intf);
                strcpy(intfstats[linkcount].linkdescr,line);
        }

  /* Open the ethtool interface command for reading. */
  fp = popen(cmd, "r");
  if (fp == NULL) {
    EVEL_DEBUG("AFX Linkmon: Failed to run command\n" );
    //pthread_exit(NULL);
    sleep(LINK_MONITOR_INTERVAL);
    continue;
  }

  while( !feof(fp) ) {
  /* Read the output a line at a time - output it. */
  while (fgets(linkdata, sizeof(linkdata)-1, fp) != NULL) {
    //EVEL_INFO("AFX: LinkMon :%s:",linkdata);
    if ((pos=strchr(linkdata, '\n')) != NULL)
           *pos = '\0';
    if( strstr(linkdata,"Supported link modes"))
    {
          pch = strstr(linkdata, search);
          while( pch != NULL &&( *pch == ':' || *pch == ' ')) pch++;
          if(pch!=NULL){
            //printf("Link mode %s\n",pch);
            strcpy(intfstats[linkcount].linkmode,pch);
          }
    }
    else if( strstr(linkdata,"Speed"))
    {
          token = strtok(linkdata, search);
          token = strtok(NULL, search);
          intfstats[linkcount].speedmbps = atoi(token);
            //printf("Link speed %d\n",intfstats[linkcount].speedmbps);
    }
    else if( strstr(linkdata,"Link detected:"))
    {
	  /* parse link data for connectivity */
          token = strtok(linkdata, search);
          token = strtok(NULL, search);
          while( *token == ' ') token++;
          if( !strcmp(token,"yes")){
            tlinkstat = 1;
          } else {
            tlinkstat = 0;
          }
          if( newrun || tlinkstat !=  intfstats[linkcount].linkstat)
          {
            EVEL_DEBUG("AFX LinkMon Status changed report fault\n");
            if( tlinkstat == 1 ){
               sprintf(vesevid,"Fault_vAfx_LinkState_%s",oam_intfaddr);
               report_fault("Fault_vAfx_Link_Status",vesevid,EVEL_SEVERITY_NORMAL,"link",intfstats[linkcount].linkdescr,"link up trap_alarm","physical or logical connection to a remote router is up","remrouter","rname",NULL, status);
            } else {
               sprintf(vesevid,"Fault_vAfx_LinkState_%s",oam_intfaddr);
               report_fault("Fault_vAfx_Link_Status",vesevid,EVEL_SEVERITY_CRITICAL,"link",intfstats[linkcount].linkdescr,"link down trap_alarm","physical or logical connection to a remote router is down","remrouter","rname",NULL, status);
            }
          }
          intfstats[linkcount].linkstat = tlinkstat;
    }
    else if( strstr(linkdata,"Cannot get link status:"))
    {
         sprintf(vesevid,"Fault_vAfx_LinkState_%s",oam_intfaddr);
         report_fault("Fault_vAfx_Link_Status",vesevid,EVEL_SEVERITY_CRITICAL,"link",intfstats[linkcount].linkname,"link down trap_alarm","physical or logical connection to a remote router is down","remrouter","rname",NULL, status);
    }
   }
  }

  /* close */
  pclose(fp);
  linkcount++;
  }

  newrun = linkcount = 0;
  if(file)fclose(file);

    sleep(LINK_MONITOR_INTERVAL);
  }

   pthread_exit(NULL);
}

