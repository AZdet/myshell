#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "job.h"

int nextjid = 1;

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


/* clearjob - Clear the entries in a job struct */
void clearjob(job_t *job)
{
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjob_table - Initialize the job list */
void initjob_table(job_t *job_table)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        clearjob(&job_table[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(job_t *job_table)
{
    int i, max = 0;

    for (i = 0; i < MAXJOBS; i++)
        if (job_table[i].jid > max)
            max = job_table[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(job_t *job_table, pid_t pid, int state, char *cmdline)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (job_table[i].pid == 0)
        {
            job_table[i].pid = pid;
            job_table[i].state = state;
            job_table[i].jid = nextjid++;
            if (nextjid > MAXJOBS)
                nextjid = 1;
            strcpy(job_table[i].cmdline, cmdline);
            return 1;
        }
    }
    printf("Tried to create too many job_table\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(job_t *job_table, pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (job_table[i].pid == pid)
        {
            clearjob(&job_table[i]);
            nextjid = maxjid(job_table) + 1;
            return 1;
        }
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(job_t *job_table)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        if (job_table[i].state == FG)
            return job_table[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
job_t *getjobpid(job_t *job_table, pid_t pid)
{
    int i;

    if (pid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (job_table[i].pid == pid)
            return &job_table[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
job_t *getjobjid(job_t *job_table, int jid)
{
    int i;

    if (jid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (job_table[i].jid == jid)
            return &job_table[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;
    for (i = 0; i < MAXJOBS; i++)
        if (job_table[i].pid == pid)
        {
            return job_table[i].jid;
        }
    return 0;
}

/* listjob_table - Print the job list */
void listjob_table(job_t *job_table)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (job_table[i].pid != 0)
        {
            printf("[%d] (%d) ", job_table[i].jid, job_table[i].pid);
            switch (job_table[i].state)
            {
            case BG:
                printf("Running ");
                break;
            case FG:
                printf("Foreground ");
                break;
            case ST:
                printf("Stopped ");
                break;
            default:
                printf("listjob_table: Internal error: job[%d].state=%d ",
                       i, job_table[i].state);
            }
            printf("%s", job_table[i].cmdline);
        }
    }
}




// int main()
// {
//     signal(SIGINT, sigint_handler);   /* ctrl-c */
//     signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
//     signal(SIGCHLD, sigchld_handler);
//     while (1)
//     {
//         //printf("loop\n");
//     }
// }