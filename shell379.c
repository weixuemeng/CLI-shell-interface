#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "header.h"
#include "pcb_handler.h"
  
#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))

#define MAX_ARGS 7 // Max number of arguments to a command
#define MAX_LENGTH 20 // Max # of characters in an argument

char *sys_cmd[7] = {"exit","jobs","kill","resume","sleep","suspend","wait"};

/*https://www.codeproject.com/Questions/1275515/How-to-initialize-global-struct-in-C*/
struct PCB pcb;
int stop=0;// if stop: 1, not stop:0
int waiting_process=-1;
int wait_index = 0;
int pending_delete[32];

/* signal hander: 
   1. https://www.masterraghu.com/subjects/np/introduction/unix_network_programming_v1.3/ch05lev1sec9.html
   2. http://alumni.cs.ucr.edu/~drougas/code-samples-tutorials/signals.c
   3. https://tildesites.bowdoin.edu/~sbarker/teaching/courses/systems/18spring/lectures/lec24.pdf
 
   break string into token:
    https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split#:~:text=To%20split%20a%20string%20we,words%20are%20separated%20by%20space.
   freopen:
     https://www.ibm.com/docs/en/zos/2.3.0?topic=functions-waitpid-wait-specific-child-process-end
*/

void signal_handler(int signo){
    pid_t   pid;
    int     stat;
    pid = waitpid(-1,&stat, WNOHANG);
    if ( waiting_process>0){ // someone to wait
        if(pid!= waiting_process){ // pid or -1
            pending_delete[wait_index++] = pid;
        }
    }
    else if (signo==SIGCHLD) { // stop not came
        for ( int i = 0; i<pcb.active; i++){
            if((pid == pcb.process_info[i].pid) &&(pcb.process_info[i].state== 'R')){
                update_finish_process(pid);
            }
        }
    }

    return;
}

void instruction_handler(int signo){
    printf("invalid instruction\n");
}
void update_complete_handler(int user_time, int sys_time){
    pcb.user_time = user_time;
    pcb.sys_time = sys_time;
}

int main(int argc, char *argv[]){
    printf("SHELL379: ");
    char cmd[LINE_LENGTH]; // can't be *cmd
    fgets( cmd, LINE_LENGTH,stdin);
    cmd[strlen(cmd)-1] = '\0';
    char exit[] = "exit\0";
    
    pcb.active=0;
    pcb.user_time=0;
    pcb.sys_time = 0;
    

    while( strcmp(cmd,exit) !=0 ){
        // todo: get the 7 arguments
        int cmd_length = strlen(cmd);
        char *myargs[8];
        int time = 0;
        for( int i=0; i< 7; i++){
            myargs[i] = (char *)malloc( MAX_LENGTH* sizeof(char));
        }
        myargs[7] = NULL;

 	    int arg_index = 0;  // which arguments {1,2,3..7}
        int char_index = 0;
        bool start = false;
        // set the arg into myargs
        char cmd1[LINE_LENGTH];
        strcpy(cmd1,cmd);

        char *ptr = strtok(cmd, " \n");
        
        while(ptr != NULL)
        {
            strcpy(myargs[arg_index],ptr);
            ptr = strtok(NULL, " \n");
            arg_index++;
        }
        free(myargs[arg_index]);
        myargs[arg_index] = NULL;
        arg_index--;


        // check if it is the listed command
        int new_cmd = 1;
        int cmd_index = 0;
        bool match = false;
        for ( int i=0; i< 7;i++) {
            if (strcmp(myargs[0],sys_cmd[i])==0 ){
                new_cmd = false;
                cmd_index = i;
                break;
            }
        }
        char *empty_str = " ";
        /*check background command*/
        bool bg_cmd = false;
        if ( strcmp(myargs[arg_index],"&")== 0){
            bg_cmd = true;
        }

// if it is the above command( basic)
        if(new_cmd== 0){
            if ( cmd_index==0){ // exit
                int status;
                for( int i = 0; i< pcb.active;i++){
                    waitpid(pcb.process_info[i].pid,NULL,0);
                }
                //while (wait(&status) > 0);  // parents wait all the children
                break;
                
            }
            if (cmd_index == 1){ // jobs_handler
                jobs_handler();

            }
            if (cmd_index == 2){ // kill_handler
                if ( strcmp(myargs[1],empty_str)==0){
                    fprintf(stderr, "illigal pid");
                }else{
                    // if pid does not exist in teh pcb table
                    
                    // if pid exist
                    pid_t kill_pid = atoi(myargs[1]);
                    
                    kill_handler(kill_pid);

                }
            
            }
            if (cmd_index == 3){ // resume (sigcont)
                if( arg_index <1){
                    // goto handler ( not terminate: invalid input)
                    perror("invalid command");
                }
                int pid_resume = atoi(myargs[1]);
                resume_handler(pid_resume);
       
            }
            if (cmd_index == 4){ // sleep
                int sleep_time = atoi(myargs[1]);
                sleep_handler(sleep_time);

            }  
            if (cmd_index == 5){ // suspend   (sigstop)
                int suspend_pid = atoi(myargs[1]);
                suspend_handler(suspend_pid);
            }
            if (cmd_index == 6){ // wait
                // get pid
                int wait_pid = atoi(myargs[1]);
                waiting_process = wait_pid;
                int a = waitpid(wait_pid,NULL,0);// processs wait ( 12345), handler wait(-1)
                pending_delete[wait_index-1] = waiting_process;
                
                // delete the process
                for ( int i = 0; i<wait_index; i++){
                    update_finish_process(pending_delete[i]);

                }
                waiting_process = -1;
                wait_index = 0;
            }
        }
// create new processes (user)
// https://www.digitalocean.com/community/tutorials/execvp-function-c-plus-plus
        else{ 

            // Invalid instruction signal handler
            signal(SIGILL, instruction_handler);
            
            // zombie process signal handler
            signal(SIGCHLD, signal_handler);
            int pid = fork();
            

            char * cmd_1 = myargs[0];
             myargs[arg_index+1]=NULL;

            if ( pid <0){// invalid fork
                perror("fork failed");
                _exit(1);
            }
            else if( pid ==0){ // child
                // find "<' and ">"
                char less='<';
                char larger = '>';
                char input[MAX_LENGTH];
                char output[MAX_LENGTH];
                
                for ( int i = 0; i<= arg_index;i++){
                    if( *(myargs[i])== less){
                        strcpy(input,(myargs[i]+1));
                        //myargs[i] = NULL;

                        freopen(input, "r", stdin);

                    }
                    if( *(myargs[i])== larger){
                        strcpy(output,(myargs[i]+1));
                        //strcpy(myargs[i], output);
                        myargs[i] = NULL;
                        freopen( output, "w", stdout);
                    }
                }

                if (execvp(cmd_1, myargs) ==-1){
                    perror("execve");
                }

            }
            else{ // parent
                if (bg_cmd== false){ // wait
                    // put into running process
                    add_cmd(pid,cmd1);
                    // parent wait child process
                    if ( waiting_process>0)
                        waitpid(waiting_process,NULL,0);
                    else{
                        wait(NULL);
                    }
                    // check and remove the running process(if not removed from the sigchild)
                    for ( int i = 0; i< pcb.active;i++){
                        if (pid == pcb.process_info[i].pid){
                            remove_cmd(pid);
                        }
                    }
                }
                else{// no wait (&)
                    add_cmd(pid,cmd1);
                }
                
            }
        }


        // TODO: free the malloc
        for( int i=0; i< 7; i++){
            free(myargs[i]);
        }
        
        
        printf("SHELL379: ");
        fgets(cmd, 200, stdin);
        cmd[strlen(cmd)-1] = '\0';
        
    }
    for( int i = 0; i< pcb.active;i++){
        waitpid(pcb.process_info[i].pid,NULL,0);
        
    }
    exit_handler();
    return 0;
}
