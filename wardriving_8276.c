/****************************************/
/* wardriving main from C code	        */
/* 				        */
/* Filippos Christou 8276 -- fchristou@auth.gr */
/* 				        */
/* enswmatwmena systhmata. 3h ergasia */
/* AUTh 2017			        */
/****************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>


struct sigaction getTime_action;
char buffer[10000]; //big enough to arrange data
pthread_cond_t isNotEmpty;
pthread_mutex_t mut;
FILE *SSIDts,*Diff;

void arrangeFile(char* lineArg)
{
     char line[100],key[80],ts[50],keytemp[80], ch, tempbuff1[20], tempbuff2[20],tmbuf[64], buf[64];
     char *temp;
     FILE *tempF;
     int isFound=0, i=0, hour,min;
     double timePassedscan2write, sec;
     struct timeval tv;
     time_t nowtime;
     struct tm *nowtm;

     strcpy(line,lineArg);

     SSIDts = fopen("SSID_timestamps.txt","r+"); //open file for reading & writing
     if (SSIDts ==NULL)
     {
          printf("Failed to open file . \n" );
          exit(1);
     }
     tempF = fopen("tempFile.txt","w");  //initiate
     if (tempF ==NULL)
     {
          printf("Failed to open file . \n" );
          exit(1);
     }
     fclose(tempF);
     tempF = fopen("tempFile.txt","r+");
     if (tempF ==NULL)
     {
          printf("Failed to open file . \n" );
          exit(1);
     }

     temp = strtok(line,"\"");
     temp = strtok(NULL,"\"");
     strcpy(key,temp);
     temp = strtok(NULL,"\"");
     strcpy(ts,temp);

     //changin format so that I can do the subtraction
     gettimeofday(&tv, NULL);
     nowtime = tv.tv_sec;
     nowtm = localtime(&nowtime);
     strftime(tmbuf, sizeof tmbuf, "%H:%M:%S", nowtm);
     snprintf(buf, sizeof buf, "%s.%06ld", tmbuf, tv.tv_usec);
     
     //taking the data from desirable format
     sprintf(tempbuff1, "%s", buf);
     sscanf(tempbuff1, "%d:%d:%lf" , &hour , &min, &sec);
     timePassedscan2write = hour*60*60 + min*60 + sec;

     sprintf(tempbuff2, "%s", ts);
     sscanf(tempbuff2, "    TS:%d:%d:%lf" , &hour , &min, &sec);
     timePassedscan2write = timePassedscan2write - (hour*60*60 + min*60 + sec);

     Diff = fopen("TimeDiffScan2Write.txt","a"); //open file and append
     if (Diff ==NULL)
     {
          printf("Failed to open file . \n" );
          exit(1);
     }
     fprintf(Diff, "timePassed :%lf\n" , timePassedscan2write);
     fclose(Diff);


     while (1)
     {
          ch = fgetc(SSIDts);
          if (ch=='\"')
          {
               i=0;
               memset(keytemp, 0, 80); //clear previous data
               while (1)
               {
                    ch = fgetc(SSIDts);
                    if (ch == '\"')
                         break;
                    keytemp[i] = ch;
                    i++;
               }
               putc('\"',tempF);
               fprintf(tempF, "%s", keytemp);
               putc('\"',tempF);
               if (strcmp(key,keytemp)==0)
               {
                    isFound=1;
                    fprintf(tempF, "%s", ts);
               }
          }else if (ch == EOF)
          {
               break;
          }else
          {
               //grapse se ena allo arxeio
               putc(ch,tempF);
          }
     }
     if (isFound == 0)
     {
          putc('\n',tempF);
          putc('\"',tempF);
          fprintf(tempF, "%s", key);
          putc('\"',tempF);
          fprintf(tempF, "%s", ts);
     }

     /*rewind(tempF);
     printf("WHOLE tempF file :  \n");
     while (1) 
     {
          ch = fgetc(tempF);
          if (ch == EOF)
               break;
          else
          {
               printf("%c", ch);
          }
     }
     printf("\nEND of WHOLE tempF file  \n"); */

     //write tempF to SSIDts
     rewind(SSIDts);
     rewind(tempF);
     //to tempF einai eksorismoy megalytero apo to SSIDts opote tha to epikalypsei
     while (1) 
     {
          ch = fgetc(tempF);
          if (ch == EOF)
               break;
          else
          {
               putc(ch, SSIDts);
          }
     }

          
     fclose(SSIDts);
     fclose(tempF);
}

void *takeWifiData(void *args)
{
     char *line ;
     char line2p[100],tempbuff[10000]; //line to process//untill 100 chars an SSID name
     char *key, *ts;
     const char delim[2] = "\n";
     int offset=0;

     SSIDts = fopen("SSID_timestamps.txt","w"); //open file for reading & writing
     if (SSIDts ==NULL)
     {
          printf("Failed to open file . \n" );
          exit(1);
     }
     fclose(SSIDts);

     Diff = fopen("TimeDiffScan2Write.txt","w"); //open file for reading & writing
     if (Diff ==NULL)
     {
          printf("Failed to open file . \n" );
          exit(1);
     }
     fclose(Diff);
     
     for(;;)
     {
          pthread_mutex_lock(&mut);
          //check if buffer is empty
          //read a line from the buffer untill empty
          line = strtok(buffer, delim);
          if (line == NULL)
          {
               pthread_cond_wait (&isNotEmpty, &mut);

               line = strtok(buffer, delim);

          }
          offset = strlen(line) + 1; //pote de tha einai to line null afoy bazw to timestamp

          //line will change since pointer, so I copy the value so
          //I copy the value somewhere else

          strcpy(line2p,line);

          //new buffer meiwmenos kata ligo afoy exw diabasei kapoia
          strcpy(tempbuff,buffer+offset);  //an to kanw apeythias exei thema eksaitias pointer
          strcpy(buffer, tempbuff);

          pthread_mutex_unlock(&mut);
          //arrange the line
          
          arrangeFile(line2p);


     }
}

void scanWifiHandler(int signum)
{
     FILE *fp;
     struct timeval timeNow ;
     time_t nowTime;  // format gettimeofday
     struct tm *nowtm; // format gettimeofday
     char tempch[10], timebuf[18] , pathline[60]; //The formatted time
     int temp;

     gettimeofday(&timeNow, NULL);
     nowTime = timeNow.tv_sec;
     nowtm = localtime(&nowTime);
     strftime(tempch, sizeof tempch, "%H:%M:%S", nowtm); //appropriate format ready !
     snprintf(timebuf, sizeof timebuf, "%s.%06ld", tempch, timeNow.tv_usec);
     fp = popen("/bin/sh searchWifi.sh", "r");
   //  fp = fopen("formatUse.txt", "r");
     if (fp == NULL) {
          printf("Failed to run command\n" );
          exit(1);
     }
     //add new line before write
     pthread_mutex_lock(&mut);
     //read untill end of file and APPEND to buffer
     while (fgets(pathline, sizeof(pathline)-1, fp) != NULL)
     {
          //remove new line
          strtok(pathline,"\n");
          //concatenate timestamp
          strcat(pathline, " TS:");
          strcat(pathline, timebuf);
          //add new line
          strcat(pathline, "\n");
          //APPEND it to buffer
          strcat(buffer, pathline) ;
     }
  //   printf("New buffer %s\n", buffer);
     pthread_mutex_unlock(&mut);
     // awake thread to arrange WIFI SSID
     pthread_cond_signal(&isNotEmpty);

     fclose(fp);
     //re-enable handler
     sigaction(SIGALRM, &getTime_action, NULL); //say that SIGALRM shall refer to scanWifiHandler function
}

int main()
{
     clock_t start_cl,end_cl;
     double cpu_time_used, realTimePassed,cpu_idle_rate, sampleSpaceRate;
     struct timeval cpuTime_1, cpuTime_2 ;
     pthread_t p_SSIDArrange;
     //initiate
     pthread_mutex_init(&mut,NULL);
     pthread_cond_init(&isNotEmpty, NULL);
     int integerPart, decimalPart ;
     char tempbuffer[50];
     sigset_t waitingMask ;
     FILE *cpuIdleF;

     cpuIdleF = fopen("CPUidleRate.txt","w"); //open file for reading & writing
     if (cpuIdleF ==NULL)
     {
          printf("Failed to open file . \n" );
          exit(1);
     }
     fclose(cpuIdleF);

     //set up waiting Mask such that it waits only the SIGALRM signal
     sigfillset(&waitingMask);
     sigdelset(&waitingMask, SIGALRM);
     sigdelset(&waitingMask, SIGINT); //we offer user the capability to quit Stopwatch8276 without waiting for next sample
     //set up the structure to specify the new action
     getTime_action.sa_handler = scanWifiHandler;
     sigemptyset(&getTime_action.sa_mask); //set all signals exlduded from the set
     getTime_action.sa_flags = 0 ; //deactivate flags (especially must deactivate SA_RESTART)

     struct itimerval timePass;
     printf("Welcome to wardriving_8276. We remind you Control-C, which is the only way out of the programm.\n");
     printf("Give scan sample space rate in seconds : ");
     scanf("%lf", &sampleSpaceRate); //ara kathe sampleSpaceRate seconds prepei na blepw ti wra einai
     //diaxwrise ta seconds (integerPart) apo ta millisec (decimalPart)
     sprintf(tempbuffer, "%lf", sampleSpaceRate);
     sscanf(tempbuffer, "%d.%d" , &integerPart , &decimalPart);
     timePass.it_interval.tv_sec = integerPart;
     timePass.it_interval.tv_usec = decimalPart;
     //timePass.it_interval.tv_sec = (int)sampleSpaceRate;
     //timePass.it_interval.tv_usec = (int) ((sampleSpaceRate-(int)sampleSpaceRate)*10e6) ; //akribeia mexri 10e-6 millisec
     timePass.it_value = timePass.it_interval ; //h prwth apopeira deigmatolipsias de tha diaferei apo oles tis alles

     printf("\n## Wi-fi ##  Scanning . . .\n");
     start_cl=clock();
     gettimeofday(&cpuTime_1, NULL); 

     //define the signal action for SIGALRM that alarm() uses
     sigaction(SIGALRM, &getTime_action, NULL); //say that SIGALRM shall refer to scanWifiHandler function
     setitimer(ITIMER_REAL, &timePass, NULL);   //set alarm to be enabled every timPass.it_interval seconds
     //gettimeofday(&start, NULL); //instantly begin measuring time !
     //unleash the first thread, which writes to file
     pthread_create(&p_SSIDArrange,NULL,takeWifiData, NULL );
     //for ever
     for(;;)
     {
          sigsuspend(&waitingMask);
          gettimeofday(&cpuTime_2, NULL); 
          end_cl=clock();
          realTimePassed = ((cpuTime_2.tv_usec - cpuTime_1.tv_usec)/1.0e6 + cpuTime_2.tv_sec - cpuTime_1.tv_sec) ;
          cpu_time_used = ((double) (end_cl - start_cl))/CLOCKS_PER_SEC ;
          cpu_idle_rate = (realTimePassed-cpu_time_used)/realTimePassed * 100 ;
          cpuIdleF = fopen("CPUidleRate.txt","a"); //open file and append
          if (cpuIdleF ==NULL)
          {
               printf("Failed to open file . \n" );
               exit(1);
          }
          fprintf(cpuIdleF, "Cpu_idle_rate :%f%%\n", cpu_idle_rate);
          fclose(cpuIdleF);
     }
     //join consumer thread
     pthread_join(p_SSIDArrange, NULL);
     printf("\n##wardriving## terminated. \n");
}
