#include <stdio.h>
#include <signal.h>
#include <sys/types.h>


void sigint_handler(int sig){
    pid_t pid = fgpid(jobs);  
    
    if (pid != 0) {     
        kill(-pid, sig);
    }   
    return;   
}
void sigtstp_handler(int sig){
    pid_t pid = fgpid(jobs);  
    //check for valid pid
    if (pid != 0) { 
        kill(-pid, sig);  
    }  
    return;   
}
void sigchld_handler(int sig){
	pid_t pid;
	int status;
	struct job_t* job;
	
	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
		if (WIFSIGNALED(status)) {
			if (WTERMSIG(status) == SIGINT) {
				printf("Job [%d] (%d) terminated by signal %d\n",
					pid2jid(pid), pid, WTERMSIG(status));
				deletejob(job_list, pid);
			}
		}
		else if (WIFSTOPPED(status)) {
			job = getjobpid(job_list, pid);
				job->state = ST;
				printf("Job [%d] (%d) stopped by signal %d\n",
					pid2jid(pid), pid, WSTOPSIG(status));
		}
		else
			deletejob(job_list, pid);
	}
	if (pid == -1 && errno != ECHILD)
		unix_error("waitpid error");
	return;
}



int main(){
    signal(SIGINT,  sigint_handler);   /* ctrl-c */
    signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
    signal(SIGCHLD, sigchld_handler);
    while (1){
        //printf("loop\n");
    }
}