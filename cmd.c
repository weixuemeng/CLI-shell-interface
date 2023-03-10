#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/wait.h> // waitpid
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/resource.h>
#include "header.h"

extern struct PCB pcb;
extern int stop;
/*
getrusage sources:
    https://pubs.opengroup.org/onlinepubs/7908799/xsh/systime.h.html
    https://www.youtube.com/watch?v=Os5cK0H8EOA
concat string:
    https://www.educative.io/blog/concatenate-string-c
convert int to string:
    https://www.tutorialspoint.com/c_standard_library/c_function_sprintf.htm
 string replace( in jobs handler: handler running process command)
    https://www.youtube.com/watch?v=0qSU0nxIZiE&t=613s
kill:
    https://www.youtube.com/watch?v=3MZjaZxZYrE
get running process time:
    https://techtalkbook.com/using-ps-command-to-get-process-detailed-information/
*/

void string_replace(char *source, char *oldpid, char *newpid){
    char *substring_start = strstr(source,oldpid);
    if( substring_start==NULL){
        return;
    }
    memmove(
            substring_start+strlen(newpid),
            substring_start+strlen(oldpid),
            strlen(substring_start)-strlen(oldpid)+1);

    memcpy(substring_start, newpid, strlen(newpid));

}
void jobs_handler(){
    
    // get the user time for each process
    int sec;
    int pid;
    char pid_str[50]; // pid has maximum 5 digits
    char sec_cmd[50] = "ps -p ";
    char *buf[pcb.active];
    FILE *fp;
    char *running_times[pcb.active];
    char c;
    bool initilaize = false;
    char old_pid[10];
    char new_pid[10];
    char time[50];
    for ( int i = 0; i< pcb.active;i++){
        // concat command with pid
        pid = pcb.process_info[i].pid;
        if (initilaize == false){
            sprintf(old_pid, "%d", pid);
            sprintf(pid_str, "%d -o time", pid);
            strcat(sec_cmd,pid_str);
            initilaize = true;

        }else{
            sprintf(new_pid, "%d", pid);
            string_replace(sec_cmd, old_pid,new_pid);
            sprintf(old_pid, "%d", pid);

        }
        
        fp = popen(sec_cmd,"r");
        int index = 0;
        int start=0;
        char hour[2];
        char min[2];
        char sec[2];
        if(fp == NULL){
            perror( "Could not open pipe\n" );
        }else{
            while(( c=fgetc(fp) ) != EOF){
                if (9<=index && index <= 10){ // hour start 00:00:00
                    time[start++] = c;
                    hour[index-9] = c;
                }
                else if(12<= index <= 13 ){
                    time[start++] = c;
                    min[index-12] = c;
                }
                else if(15<= index <= 16 ){
                    time[start++] = c;
                    sec[index-15] = c;
                }
                index++;
            }
            time[start] = '\0';

            pcb.process_info[i].sec =atoi(hour)*3600+atoi(min)*60+atoi(sec);
            pclose( fp );
        }
        
    }
    
    // get the sys time and user time from getrusage:
    struct rusage myusage;
    getrusage(RUSAGE_CHILDREN, &myusage);
    struct timeval tv_sys = myusage.ru_stime;
    struct timeval tv_usr = myusage.ru_utime;
    time_t sys_time = tv_sys.tv_sec;
    time_t usr_time = tv_usr.tv_sec;
    
    pcb.sys_time = sys_time;
    pcb.user_time = usr_time;


    // print the table:
	printf("\n");
    printf("Running processes: \n");
    if ( pcb.active >0){
        printf(" #    PID   S   SEC   COMMAND\n");
        for ( int i=0; i< pcb.active; i++){
            printf("%d:    %d  %c %d %s\n", i, pcb.process_info[i].pid, pcb.process_info[i].state,pcb.process_info[i].sec,
            pcb.process_info[i].cmd);
        }
    }
    printf("Processes =      %d active\n",pcb.active);
    printf("Completed processes:\n");
    printf("User time =      %ld seconds\n", pcb.user_time);
    printf("Sys time =       %ld seconds\n", pcb.sys_time);
    return;
}
void exit_handler(){

    // get the sys time and user time from getrusage:
    struct rusage myusage;
    getrusage(RUSAGE_CHILDREN, &myusage);
    struct timeval tv_sys = myusage.ru_stime;
    struct timeval tv_usr = myusage.ru_utime;
    time_t sys_time = tv_sys.tv_sec;
    time_t usr_time = tv_usr.tv_sec;
    
    printf("Resources used:\n");
    printf("User time =      %ld seconds\n", usr_time  );
    printf("Sys time =       %ld seconds\n",sys_time);
    
    return;
}
void kill_handler(pid_t pid){
    kill(pid, SIGKILL);
    
}

void wait_handler( pid_t pid){
    // check if pid is in the table or not
    int pid_res = waitpid(pid,NULL,0);
    if (pid_res ==-1){
        perror("wait unsuccessful");
    }
    return;
}
void suspend_handler(pid_t pid){
    stop = 1; // stop the process
    for ( int i = 0; i< pcb.active; i++){
        if ( pcb.process_info[i].pid == pid){
            pcb.process_info[i].state = 'S';
        }
    }
    kill(pid, SIGSTOP);
    return;
}
void resume_handler(pid_t pid){
    stop = 0; // continue the process
    for ( int i = 0; i< pcb.active; i++){
        if ( pcb.process_info[i].pid == pid){
            pcb.process_info[i].state = 'R';
        }
    }
    kill(pid, SIGCONT);
    return;
}
void sleep_handler( int time){
    sleep(time);
}

