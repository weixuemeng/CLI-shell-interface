#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "pcb_handler.h"
#define LINE_LENGTH 100 // Max # of characters in an input line
#define MAX_PT_ENTRIES 32 // Max entries in the Process Table

typedef struct{
    pid_t pid;
    char state;
    int  sec;
    char cmd[LINE_LENGTH];
}Process_Info;

struct PCB{
    // active process
    int active;
    Process_Info process_info[MAX_PT_ENTRIES];
    // complete process
    long user_time;
    long sys_time;

};

extern struct PCB pcb;
extern int stop;

void add_cmd(int pid, char cmd[]){
    Process_Info *process = (Process_Info *) malloc(sizeof(Process_Info));
    // struct Process_Info process;
    process->pid = pid;
    process->state = 'R';
    process->sec = 0;
    strcpy(process->cmd,cmd);
    pcb.process_info[pcb.active] = *process;
    free(process); // not sure
    
    pcb.active+=1;  // 1

}
void remove_cmd(int pid){
    int pid_index =0;
    for( int i = 0; i< pcb.active; i++){
        if (pcb.process_info[i].pid == pid){ 
            pid_index= i;
        }
    }
    if ( pcb.active >1){
        for( int i = pid_index; i< pcb.active-1; i++){
            memcpy(&(pcb.process_info[i]), &(pcb.process_info[i+1]), sizeof(pcb.process_info[i+1]));
        }
    }

    pcb.active--;

}
void update_finish_process(pid_t pid){
    // remove entry from running process 
    remove_cmd(pid);  
}
