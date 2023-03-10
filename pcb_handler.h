#ifndef PCB_HEADER_FILE
#define PCB_HEADER_FILE

void add_cmd(int pid, char cmd[]);
void remove_cmd(int pid);
void update_finish_process(pid_t pid);

#endif
