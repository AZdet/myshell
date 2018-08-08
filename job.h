#ifndef _JOB_H_
#define _JOB_H_
#include <sys/types.h>
#define DISPLAY_LEN 20
#define MAXJOBS 20
typedef enum
{
    UNDEF,
    FG,
    BG,
    ST
} JOBSTATE;

typedef struct
{
    pid_t pid;
    int jid;
    JOBSTATE state;
    char cmdline[DISPLAY_LEN];
} job_t;

void clearjob(job_t *job);
void initjob_table(job_t *job_table);
int addjob(job_t *job_table, pid_t pid, int state, char *cmdline);
int deletejob(job_t *job_table, pid_t pid);
pid_t fgpid(job_t *job_table);
job_t *getjobpid(job_t *job_table, pid_t pid);
job_t *getjobjid(job_t *job_table, int jid);
int pid2jid(job_t *job_table, pid_t pid);
void listjob_table(job_t *job_table);

#endif
