#ifndef HEADER_FILE
#define HEADER_FILE

#define LINE_LENGTH 100 // Max # of characters in an input line
#define MAX_PT_ENTRIES 32 // Max entries in the Process Table

void jobs_handler();
void exit_handler();
void kill_handler(pid_t pid);
void wait_handler(pid_t pid);
void resume_handler(pid_t pid);
void suspend_handler(pid_t pid);
void sleep_handler(int time);
struct Process_Info;
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


#endif
