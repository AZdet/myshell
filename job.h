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
#endif
