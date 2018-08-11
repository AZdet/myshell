#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "job.h"

int nextjid = 1;

/* 清除job，把job的值恢复为默认值 */
void clearjob(job_t *job)
{
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* 初始化job_table */
void initjob_table(job_t *job_table)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        clearjob(&job_table[i]);
}

/* 返回当前已经分配的最大jid */
int maxjid(job_t *job_table)
{
    int i, max = 0;

    for (i = 0; i < MAXJOBS; i++)
        if (job_table[i].jid > max)
            max = job_table[i].jid;
    return max;
}

/* 向job_table新增一个job */
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
    printf("Tried to create too many jobs\n");
    return 0;
}

/* 从job_table里删除对应pid号的job */
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

/* 返回当前属于foreground的job的pid，没有的话返回0 */
pid_t fgpid(job_t *job_table)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        if (job_table[i].state == FG)
            return job_table[i].pid;
    return 0;
}

/* 根据pid得到对应的job */
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

/* 根据jid得到对应的job */
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

/* 从pid转换为jid */
int pid2jid(job_t *job_table, pid_t pid)
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

/* 显示job_table */
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
            printf("%s\n", job_table[i].cmdline);
        }
    }
}

