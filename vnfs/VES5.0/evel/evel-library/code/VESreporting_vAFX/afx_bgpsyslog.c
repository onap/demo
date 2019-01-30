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

/* tracks syslog file */
static unsigned long long prevpos=0;
/* memory for word parsing */
static char ** bgp_res  = NULL;
/* Search list of words */
char* searchlist[MAX_SYSLOG_WORDS];
int numWords=0;

/**************************************************************************//**
 * Sends syslog message with parsed syslog info
 *
 * @param info      Pointer to the line
 * @param tag       Matched TAG field
 * @param sevrty    Severity of parsed line Emergency etc.
 *
 * @retvalue  int   
 *****************************************************************************/
void report_syslog( char *info, char *tag, char *sevrty )
{
  EVENT_SYSLOG * psyslog = NULL;
  EVEL_ERR_CODES evel_rc = EVEL_SUCCESS;
  char vesevid[128];

  sprintf(vesevid,"Syslog_vAfx_%s",oam_intfaddr);
  psyslog = evel_new_syslog("Syslog_vAfx", vesevid,
                            EVEL_SOURCE_VIRTUAL_MACHINE,
                           escape_json(info),
                           tag);
  if (psyslog != NULL)
  {
    evel_header_type_set((EVENT_HEADER *)psyslog, "applicationVnf");
    evel_nfnamingcode_set((EVENT_HEADER *)psyslog, "AFX");
    evel_nfcnamingcode_set((EVENT_HEADER *)psyslog, "AFX");
    evel_syslog_event_source_host_set(psyslog, "Virtual host");
    evel_syslog_facility_set(psyslog, EVEL_SYSLOG_FACILITY_LOCAL0);
    evel_syslog_proc_set(psyslog, tag);
    //evel_syslog_proc_id_set(psyslog, 1423);
    evel_syslog_version_set(psyslog, 1);
    //evel_syslog_addl_filter_set(psyslog, "Name1=Value1|Name2=Value2|Name3=Value3");
    evel_syslog_severity_set(psyslog, sevrty);
    //evel_syslog_sdid_set(psyslog, "ourSDID@32473");
    //evel_syslog_s_data_set(psyslog,info);
    evel_rc = evel_post_event((EVENT_HEADER *)psyslog);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR(" AFX: Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }
  else
  {
    EVEL_ERROR("New Syslog failed");
  }
  EVEL_DEBUG("AFX: Processed full Syslog\n");

}

/**************************************************************************//**
 * Parses Syslog line for exabgp messages
 * Raises BGP Fault event for peer connection setup and teardown
 * Sends Syslog events for matching lines
 *
 * @param line      Pointer to line
 *
 * @retvalue  int   Returns if matching line
 *****************************************************************************/
int processLine(char *line)
{
    int n_spaces = 0, i;
    char *pos;
    char *p;
    int srchcondn = 0;
    int peer_conn = 0;
    int peer_reset = 0;
    char vesevid[128];
    char sevrty[128];
    char *line_bk = NULL;
    int status = 1; /** VFA Status is ACTIVE **/

   if ((pos=strchr(line, '\n')) != NULL)
           *pos = '\0';
   line_bk = strdup(line);

   if( !strstr(line,"EVEL") && !strstr(line,"commonEventHeader")  && strstr(line,"exabgp") )
   {
	if( strstr(line,"Connected to peer neighbor") ) peer_conn = 1;
	else if( strstr(line,"peer reset") ) peer_reset = 1;

       if( peer_conn || peer_reset )
       {
	n_spaces = 0;
	p    = strtok (line, " ");
	/* split string and append tokens to 'res' */

	while (p) {
	  bgp_res = realloc (bgp_res, sizeof (char*) * ++n_spaces);

	  if (bgp_res == NULL){
	    EVEL_DEBUG("Memory allocation failed");
	    exit (-1); /* memory allocation failed */
	  }

	  bgp_res[n_spaces-1] = p;

	  p = strtok (NULL, " ");
	}

	/* realloc one extra element for the last NULL */
	bgp_res = realloc (bgp_res, sizeof (char*) * (n_spaces+1));
	bgp_res[n_spaces] = 0;

	  if( n_spaces > 0 && (peer_conn || peer_reset) )
	  {
	    EVEL_DEBUG ("Exabgp %d %d %d\n", peer_conn, peer_reset, n_spaces);
	   for (i = 0; i < (n_spaces+1); ++i)
	   {
	    if( bgp_res[i] != NULL )EVEL_DEBUG ("res[%d] = %s\n", i, bgp_res[i]);
	   }
	   if( peer_conn == 1 && n_spaces >= 21 )
	    //if( !strcmp(bgp_res[21],"(out)") )
	   {
	     char buf[256];
	     EVEL_DEBUG("****************sending clear fault\n");
	     sprintf(vesevid,"Fault_vAfx_bgp_nbr_%s",oam_intfaddr);
	     sprintf(buf," %s %s %s %s %s %s %s %s %s %s %s ", bgp_res[10], bgp_res[11], bgp_res[12], bgp_res[13], bgp_res[14],bgp_res[15], bgp_res[16], bgp_res[17], bgp_res[18], bgp_res[19],  bgp_res[20]);
	     report_fault("Fault_vAfx_bgp_nbr", vesevid, EVEL_SEVERITY_NORMAL, "routing", "lo:0", "Bgp neighbor up alarm","bgp session to remote router is up",bgp_res[16],bgp_res[18],bgp_res[10], status);
	   }
	   if( peer_reset == 1 && n_spaces >= 21 )
	    //if( !strcmp(bgp_res[21],"Shutdown]") )
	   { char buf[256];
	     EVEL_DEBUG("****************sending fault\n");
	     sprintf(vesevid,"Fault_vAfx_bgp_nbr_%s",oam_intfaddr);
	     sprintf(buf," %s %s %s %s %s %s %s ", bgp_res[15], bgp_res[16], bgp_res[17], bgp_res[18], bgp_res[19],bgp_res[20], bgp_res[21]);
	       report_fault("Fault_vAfx_bgp_nbr", vesevid, EVEL_SEVERITY_MAJOR,"routing","lo:0","Bgp neighbor down alarm","bgp session to remote router is down",bgp_res[9],bgp_res[4],bgp_res[7], status);
	   }
	  }
	}

      }// end exabgp


   if( !strstr(line_bk,"EVEL") && !strstr(line,"commonEventHeader")  && numWords > 0 ){
     for (i=0;i<numWords;i++)
     {
        if( strstr(line_bk,searchlist[i]) != NULL &&
                    strstr(line_bk,"commonEventHeader") == NULL )
        {
          srchcondn = 1;
          pos = searchlist[i];
          break;
        }
        printf("srch:%s:%d:%d\n",searchlist[i],srchcondn,numWords);
     }

    if( srchcondn )
    {
     strcpy(sevrty,"Informational");
     if( strcasestr(line,"Emergency") != NULL ) strcpy(sevrty,"Emergency");
     else if( strcasestr(line_bk,"Alert") != NULL ) strcpy(sevrty,"Alert");
     else if( strcasestr(line_bk,"Critical") != NULL ) strcpy(sevrty,"Critical");
     else if( strcasestr(line_bk,"Error") != NULL ) strcpy(sevrty,"Error");
     else if( strcasestr(line_bk,"Warn") != NULL ) strcpy(sevrty,"Warning");
     else if( strcasestr(line_bk,"Notice") != NULL ) strcpy(sevrty,"Notice");
     else if( strcasestr(line_bk,"Debug") != NULL ) strcpy(sevrty,"Debug");
     report_syslog(line_bk,pos,sevrty);
    }
   }

  if(!srchcondn) 
    free(line_bk);
  return srchcondn;

}


/**************************************************************************//**
 * function to read last n lines from the file
 * at any point without reading the entire file
 *****************************************************************************/
void bgp_tail(FILE* in, int n)
{
    int count = 0;  // To count '\n' characters

    // unsigned long long pos (stores upto 2^64 – 1
    // chars) assuming that long long int takes 8
    // bytes
    unsigned long long pos;
    char str[2*BGPBUFSIZE] = {0};

    // Go to End of file
    if (fseek(in, 0, SEEK_END))
        perror("fseek() failed");
    else
    {
        // pos will contain no. of chars in
        // input file.
        pos = ftell(in);

        // search for '\n' characters
        while (pos>prevpos)
        {
            // Move 'pos' away from end of file.
            if (!fseek(in, --pos, SEEK_SET))
            {
                if (fgetc(in) == '\n')

                    // stop reading when n newlines
                    // is found
                    if (count++ == n)
                        break;
            }
            else
                perror("fseek() failed");
        }
        //printf("pos %d prevpos %d\n",pos,prevpos);

        // print last n lines
        prevpos = pos;
        //printf("Printing %d lines %d -\n", n,pos);
        while (fgets(str, sizeof(str), in))
        {
            //printf("%s\n", str);
            processLine(strdup(str));
            prevpos += strlen(str);
        }
    }
    //printf("\n\n");
}


/**************************************************************************//**
 * Thread function to Monitor Syslog for BGP connections 
 * Reads the filter words from AFX Filter input file
 * Starts tailing syslog for BGP messages and filter words
 *
 * @param threadarg  Thread arguments for startup message
 *
 *****************************************************************************/
void *BgpLoggingAfxThread(void *threadarg)
{
    FILE* fp;
    int i=0,j;
    char line[64];
    char *pos;
    FILE *file;
    int taskid, sum;
    char *hello_msg;
    struct thread_data *my_data;

   my_data = (struct thread_data *) threadarg;
   taskid = my_data->thread_id;
   sum = my_data->sum;
   hello_msg = my_data->message;
   EVEL_DEBUG("Thread %d: %s  Sum=%d\n", taskid, hello_msg, sum);

   sleep(1);

   /* Opens filter file and reads word list */
   file = fopen(AFX_SYSLOG_FILE, "r");

    while(fgets(line, sizeof line, file)!=NULL) {
        //check to be sure reading correctly
        //printf("%s", line);
        if ((pos=strchr(line, '\n')) != NULL)
         *pos = '\0';
        remove_spaces(line);
        //add each filename into array of programs
        if( strlen(line) > 0 )
        {
           searchlist[numWords]=strdup(line);
           //count number of programs in file
           numWords++;
        }
    }
    fclose(file);

    //check to be sure going into array correctly
    for (int j=0 ; j<numWords; j++) {
        EVEL_DEBUG("Search %s\n", searchlist[j]);
    }


    // Open file in read mode
    fp = fopen("/var/log/syslog", "r");
    if (fp == NULL)
    {
        printf("Error while opening file");
        exit(EXIT_FAILURE);
    }

    // call tail() each time
    while(1)
    {
        // read last index lines from the file
        bgp_tail(fp, 8);

        // sleep for 3 seconds
        // note difference in timestamps in logs
        sleep(1);
    }

    /* close the file before ending program */
    fclose(fp);

    if( bgp_res != NULL ) free(bgp_res);

    return 0;
}


