#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include "job.h"
#include "test.h"
#define MAX_LINE 80 /* The maximum length command */
#define MAX_CMD_LEN 7

// typedef enum {
// // basic functionality
//     cd,
//     clr,
//     dir,
//     echo,
//     environ,
//     help,
//     pwd,
//     quit,
//     time,
//     umask,
//     exit,
// // manage processes
//     bg,   // 1. add handler for ctrl-z to suspend a process
//     fg,   // 2. fg put it in foreground, with help of tcsetpgrp
//     jobs,  // 3. both fg and bg continue the process
//     exec,
// // shell programming
//     set,
//     shift,
//     test,
//     unset
// }
// COMMEND;

char Internal_CMDS[][MAX_CMD_LEN + 1] = {"cd", "clr", "dir", "echo",
                                         "environ", "exit", "help", "pwd", "quit", "time", "umask",
                                         "bg", "fg", "jobs", "exec", "set", "shift", "test", "unset"};

void init();
void setpath(char *newpath);                                          /*设置搜索路径*/
int readCmd();                                                        /*读取用户输入*/
int is_internal_cmd(char *cmd);                                       /*解析内部命令*/
int do_pipe(char *cmd, int cmdlen);                                   /*解析管道命令*/
int io_redirect(char *cmd, int cmdlen);                               /*解析重定向*/
int normal_cmd(char *cmd, int cmdlen, int infd, int outfd, int fork); /*执行普通命令*/
/*其他函数……. */

char cmd[MAX_LINE + 1];                  // + 1 for '\0'，记录总的命令
char *cmd_argvs[MAX_LINE][MAX_LINE + 1]; // cmd_argvs[i][j]为第i个命令中的第j个参数，其中第0个参数就是命令本身, +1 for NULL
int argcs[MAX_LINE];                     // argcs[i]为第i个命令的长度
int num_cmd = 0;

// char *job_table[MAX_JOBS]; // 所有后台工作的信息，格式为“job_num  pid  cmd”
// int job_num_in_use[MAX_JOBS];
// int job_num = 0;
job_t job_table[MAXJOBS];

int is_pipe;
int is_io_redirect;
int infile;  //
int outfile; //
int is_background;
int is_error;

int should_run = 1; /* flag to determine when to exit program */

// void init(){
//     signal(SIGINT, sigint_handler);   /* ctrl-c */
//     signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
//     signal(SIGCHLD, sigchld_handler);
//     /*设置命令提示符*/
//     printf("myshell>");
//     fflush(stdout);
//     /*设置默认的搜索路径*/
//     setpath("/bin:/usr/bin");
//     /*……*/
// }

int do_internal_cmd(char **cmd_argv, int *ptr_argc, char *argv[], char *envp[])
{
    int i, ret;
    if (!strcmp(cmd_argv[0], "cd"))
    {
        if (!cmd_argv[1])
        {
            char *cur_path = getcwd(NULL, 0);
            printf("current path is: %s\n", cur_path);
            free(cur_path);
        }
        else
        {
            if (cmd_argv[2])
            {
                fprintf(stderr, "Error: cd only takes one argument!");
                is_error = 1;
                return 1;
            }
            ret = chdir(cmd_argv[1]);
        }
        if (!ret)
        {
            perror("chdir");
            return 1;
        }
    }
    else if (!strcmp(cmd_argv[0], "cls"))
    {
        if (cmd_argv[1])
        {
            fprintf(stderr, "Error: cls takes no argument!");
            is_error = 1;
            return 1;
        }
        printf("\033c"); // 特殊的terminal code，可以通过man console_codes查看详情
    }
    else if (!strcmp(cmd_argv[0], "dir"))
    {
        if (!cmd_argv[1])
        {
            fprintf(stderr, "Error: dir takes one argument!");
            is_error = 1;
            return 1;
        }
        else if (cmd_argv[2])
        {
            fprintf(stderr, "Error: dir takes only one argument!");
            is_error = 1;
            return 1;
        }
        DIR *dir = opendir(cmd_argv[1]);
        if (!dir)
        {
            perror("opendir");
            is_error = 1;
            return 1;
        }
        struct dirent *ent;
        while ((ent = readdir(dir)))
        {
            if (ent->d_name[0] != '.')
                printf("%s ", ent->d_name);
        }
        printf("\n");
    }
    else if (!strcmp(cmd_argv[0], "echo"))
    {
        if (!cmd_argv[1])
        {
            fprintf(stderr, "Error: echo takes one argument!");
            is_error = 1;
            return 1;
        }
        else if (cmd_argv[2])
        {
            fprintf(stderr, "Error: echo takes only one argument!");
            is_error = 1;
            return 1;
        }
        printf(cmd_argv[1]);
    }
    else if (!strcmp(cmd_argv[0], "environ"))
    {
        if (cmd_argv[1])
        {
            fprintf(stderr, "Error: help takes no argument!");
            is_error = 1;
            return 1;
        }
        extern char **environ;
        for (char **env = environ; *env; env++)
            printf("%s\n", *env);
        // while(*envp)
        //     printf("%s\n",*envp++);
    }
    else if (!strcmp(cmd_argv[0], "exit"))
    {
        if (!cmd_argv[1])
        {
            fprintf(stderr, "Error: umask takes one argument!");
            is_error = 1;
            return 1;
        }
        else if (cmd_argv[2])
        {
            fprintf(stderr, "Error: umask takes only one argument!");
            is_error = 1;
            return 1;
        }
        int ret_code, ret;
        ret = sscanf(cmd_argv[1], "%d", &ret_code);
        if (ret != 1)
        {
            fprintf(stderr, "Error: invalid return code after exit!");
            is_error = 1;
            return 1;
        }
        exit(ret_code);
    }
    else if (!strcmp(cmd_argv[0], "help"))
    {
        if (cmd_argv[1])
        {
            fprintf(stderr, "Error: help takes no argument!");
            is_error = 1;
            return 1;
        }
        system("cat readme | more");
    }
    else if (!strcmp(cmd_argv[0], "pwd"))
    {
        if (cmd_argv[1])
        {
            fprintf(stderr, "Error: pwd takes no argument!");
            is_error = 1;
            return 1;
        }
        char *pwd = getcwd(NULL, 0);
        printf("%s\n", pwd);
        free(pwd);
    }
    else if (!strcmp(cmd_argv[0], "quit"))
    {
        if (cmd_argv[1])
        {
            fprintf(stderr, "Error: quit takes no argument!");
            is_error = 1;
            return 1;
        }
        should_run = 0;
    }
    else if (!strcmp(cmd_argv[0], "time"))
    {
        if (cmd_argv[1])
        {
            fprintf(stderr, "Error: time takes no argument!");
            is_error = 1;
            return 1;
        }
        time_t rawtime;
        struct tm *timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        printf("Current local time and date: %s", asctime(timeinfo));
    }
    else if (!strcmp(cmd_argv[0], "umask"))
    {
        if (!cmd_argv[1])
        {
            fprintf(stderr, "Error: umask takes one argument!");
            is_error = 1;
            return 1;
        }
        else if (cmd_argv[2])
        {
            fprintf(stderr, "Error: umask takes only one argument!");
            is_error = 1;
            return 1;
        }
        mode_t mode;
        int ret;
        ret = sscanf(cmd_argv[1], "%o", &mode);
        if (ret == 1 && mode >= 0 && mode <= 0777)
        {
            umask(mode);
        }
        else
        {
            fprintf(stderr, "Error: invalid umask mode!");
            is_error = 1;
            return 1;
        }
    }
    else if (!strcmp(cmd_argv[0], "bg") || !strcmp(cmd_argv[0], "fg"))
    {
        return do_bgfg(cmd_argv);
    }

    else if (!strcmp(cmd_argv[0], "jobs"))
    {
        listjob_table(job_table);
    }
    else if (!strcmp(cmd_argv[0], "exec"))
    {
        execvp(cmd_argv[1], &cmd_argv[1]);
    }
    else if (!strcmp(cmd_argv[0], "set"))
    {
        if (!cmd_argv[1] || !cmd_argv[2])
        {
            fprintf(stderr, "Error: set takes two arguments!");
            is_error = 1;
            return 1;
        }
        else if (cmd_argv[3])
        {
            fprintf(stderr, "Error: set takes only two arguments!");
            is_error = 1;
            return 1;
        }
        int ret = setenv(cmd_argv[1], cmd_argv[2], 1); // overwrite = 1
        if (!ret)
        {
            perror("setenv");
            is_error = 1;
            return 1;
        }
    }
    else if (!strcmp(cmd_argv[0], "shift"))
    {
        // get $#
        // n+1 ... # --> 1 ... #-n
        int shift_pos = 1, ret, i;
        if (argv[1])
        {
            ret = sscanf(cmd_argv[1], "%d", &shift_pos);
            if (ret != 1 || shift_pos <= 0)
            {
                fprintf(stderr, "Error: invalid number after shift!\n");
                is_error = 1;
                return 1;
            }
            else if (cmd_argv[2])
            {
                fprintf(stderr, "Error: shift only takes one argument!\n");
                is_error = 1;
                return 1;
            }
        }
        for (i = 1; i < *ptr_argc - shift_pos; i++)
        {
            argv[i] = argv[i + shift_pos];
        }
        *ptr_argc -= shift_pos;
    }
    else if (!strcmp(cmd_argv[0], "test"))
    {
        do_test(cmd_argv);
    }
    else if (!strcmp(cmd_argv[0], "unset"))
    {
        if (!cmd_argv[1])
        {
            fprintf(stderr, "Error: unset takes one argument!");
            is_error = 1;
            return 1;
        }
        else if (cmd_argv[2])
        {
            fprintf(stderr, "Error: unset takes only one argument!");
            is_error = 1;
            return 1;
        }
        int ret = unsetenv(cmd_argv[1]);
        if (!ret)
        {
            perror("unsetenv");
            is_error = 1;
            return 1;
        }
    }
    return 0;
}

int readCmd()
{
    char *tok, *next_tok;
    int num_arg = 0;
    printf(">");
    fgets(cmd, MAX_LINE, stdin);
    //int len = strlen(cmd);
    num_cmd = 0;
    is_pipe = is_io_redirect = is_background = is_error = 0;
    infile = outfile = -1;
    // structure of a command
    // cmd args [| cmd args]* [[< filename] [> filename]  [>> filename]]* [&]
    tok = strtok(cmd, " "); // strtok会自动加入'\0'
    cmd_argvs[num_cmd][num_arg++] = tok;
    while (tok)
    {
        tok = strtok(NULL, " ");
        if (!tok)
            break;
        if (!strcmp(tok, "|"))
        { // 结束当前指令，开始下一条子命令
            cmd_argvs[num_cmd][num_arg] = NULL;
            argcs[num_cmd] = num_arg;
            num_cmd++;
            num_arg = 0;
        }
        else if (!strcmp(tok, "<") || !strcmp(tok, ">") || !strcmp(tok, ">>") || !strcmp(tok, "&"))
        { // 达到命令的末尾部分
            next_tok = strtok(NULL, " ");
            if (!strcmp(tok, "&"))
            {
                is_background = 1;
                if (next_tok)
                {
                    fprintf(stderr, "Error: unexpected token after &\n");
                    is_error = 1;
                }
                break; // 到＆后，总的命令应该停止
            }
            else
            {
                if (!next_tok)
                {
                    fprintf(stderr, "Error: unexpected end after %s\n", tok);
                    is_error = 1;
                    break;
                }
                else if (!strcmp(tok, "<"))
                {
                    //infile = fopen(next_tok, "r");
                    infile = open(next_tok, O_RDONLY);
                    if (infile == -1)
                    {
                        fprintf(stderr, "Error: can not open file %s\n", next_tok);
                        is_error = 1;
                        break;
                    }
                }
                else if (!strcmp(tok, ">"))
                {
                    // outfile = fopen(next_tok, "w");
                    outfile = open(next_tok, O_WRONLY | O_CREAT | O_TRUNC);
                    if (outfile == -1)
                    {
                        fprintf(stderr, "Error: can not open file %s\n", next_tok);
                        is_error = 1;
                        break;
                    }
                }
                else if (!strcmp(tok, ">>"))
                {
                    //outfile = fopen(next_tok, "a");
                    outfile = open(next_tok, O_WRONLY | O_CREAT | O_APPEND);
                    if (outfile == -1)
                    {
                        fprintf(stderr, "Error: can not open file %s\n", next_tok);
                        is_error = 1;
                        break;
                    }
                }
            }
        }
        else
        { // 仍在同一条子命令中，继续添加参数
            cmd_argvs[num_cmd][num_arg++] = tok;
        }
    }
}

int executeCmd(int cmd_idx, int *ptr_argc, char *argv[], char *envp[])
{
    sigset_t mask;
    int ret;
    if (is_internal_cmd(cmd_argvs[cmd_idx][0]))
    {
        do_internal_cmd(cmd_argvs[cmd_idx], ptr_argc, argv, envp);
    }
    else
    {
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL); // block SIGCHLD

        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            is_error = 1;
        }
        else if (!pid)
        { // child process
            // 添加job_table记录, should be in main process
            // int tmp_job_num = (job_num + 1) % MAX_CMD_LEN;
            // job_num = ;
            // job_num_in_use[job_num] = 1;
            sigprocmask(SIG_UNBLOCK, &mask, NULL);
            setpgid(0, 0);

            ret = execvp(cmd_argvs[cmd_idx][0], cmd_argvs[cmd_idx]);
            if (ret < 0)
            {
                perror("execvp");
                exit(1);
            }
            // 删除job_table记录
            exit(0);
        }
        else
        { // parent process
            JOBSTATE state = (is_background) ? BG : FG;
            addjob(job_table, pid, state, cmd_argvs[cmd_idx][0]);
            sigprocmask(SIG_UNBLOCK, &mask, NULL);

            if (is_background)
            {
                printf("[%d] (%d) %s", pid2jid(pid), pid, cmd_argvs[cmd_idx]);
            }
            else
            {
                waitfg(pid);
            }
        }
    }
}
void do_fgbg(char *argv[])
{
    pid_t pid;
    job_t *job;
    int jid;
    if (!argv[1])
    {
        fprintf(stderr, "Error: unset takes one argument!");
        is_error = 1;
        return 1;
    }
    if (argv[1][0] == '%')
    {
        jid = atoi(&argv[1][1]);
        //get job
        job = getjobjid(job_table, jid);
        if (job == NULL)
        {
            printf("%s: No such job\n", argv[1]);
            return;
        }
        else
        {
            //get the pid if a valid job for later to kill
            pid = job->pid;
        }
    }
    // if it is a pid
    else if (isdigit(argv[1][0]))
    {
        //get pid
        pid = atoi(argv[1]);
        //get job
        job = getjobpid(job_table, pid);
        if (job == NULL)
        {
            printf("(%d): No such process\n", pid);
            return;
        }
    }
    else
    {
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        return;
    }
    // set job and pid
    kill(-pid, SIGCONT);
    if (!strcmp(argv[0], "fg"))
    {
        job->state = FG;
        waitfg(job->pid);
    }
    else
    {
        job->state = BG;
        printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
    }
}
/*
bg fg jobs
cd clr dir echo exec exit environ help pwd quit time umask
set shift test  unset
*/
int main(int argc, char *argv[], char *envp[])
{
    //char *args[MAX_LINE/2 + 1]; /* command line arguments */

    int i;
    int tmp_in, tmp_out;
    //init();
    while (should_run)
    {
        /**
        * After reading user input, the steps are: 
        *内部命令：
        *…..
        *外部命令：
        * (1) fork a child process using fork()
        * (2) the child process will invoke execvp()
        * (3) if command included &, parent will invoke wait()
        *…..
        */
        readCmd();
        if (is_error)
            continue;
        tmp_in = dup(0);
        tmp_out = dup(1);
        if (infile == -1)
            infile = dup(tmp_in);
        for (i = 0; i < num_cmd; i++)
        {
            dup2(infile, 0);
            close(infile);
            if (i == num_cmd - 1)
            {
                if (outfile == -1)
                    outfile = dup(tmp_out);
            }
            else
            {
                int fdpipe[2];
                pipe(fdpipe);
                infile = fdpipe[0];
                outfile = fdpipe[1];
            }
            dup2(outfile, 1);
            close(outfile);

            // 完成IO定向，执行子命令
            executeCmd(i, &argc, argv, envp);

            dup2(tmp_in, 0);
            dup2(tmp_out, 1);
        }
    }
    return 0;
}

void sigint_handler(int sig)
{
    pid_t pid = fgpid(job_table);

    if (pid != 0)
    {
        kill(-pid, sig);
    }
    return;
}
void sigtstp_handler(int sig)
{
    pid_t pid = fgpid(job_table);
    //check for valid pid
    if (pid != 0)
    {
        kill(-pid, sig);
    }
    return;
}
void sigchld_handler(int sig)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(fgpid(job_table), &status, WNOHANG | WUNTRACED)) > 0)
    {
        if (WIFSTOPPED(status))
        {
            //change state if stopped
            job_t *job = getjobpid(job_table, pid);
            if (!job){

            }
            job->state = ST;
            int jid = pid2jid(pid);
            printf("Job [%d] (%d) Stopped by signal %d\n", jid, pid, WSTOPSIG(status));
        }
        else if (WIFSIGNALED(status))
        {
            //delete is signaled
            int jid = pid2jid(pid);
            printf("Job [%d] (%d) terminated by signal %d\n", jid, pid, WTERMSIG(status));
            deletejob(job_table, pid);
        }
        else if (WIFEXITED(status))
        { // child terminated normally
            //exited
            deletejob(job_table, pid);
        }
    }
    return;
}

void sigchld_handler(int sig)
{
    pid_t pid;
    int status;
    job_t *job;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0)
    { // wait
        if (WIFSIGNALED(status))
        { // child process was terminated by a signal
            if (WTERMSIG(status) == SIGINT)
            { // the number of the signal that caused the child process to terminate
                printf("Job [%d] (%d) terminated by signal %d\n",
                       pid2jid(pid), pid, WTERMSIG(status));
                deletejob(job_table, pid);
            }
        }
        else if (WIFSTOPPED(status))
        { // the child process was stopped by delivery of a signal;
            job = getjobpid(job_table, pid);
            job->state = ST;
            printf("Job [%d] (%d) stopped by signal %d\n",
                   pid2jid(pid), pid, WSTOPSIG(status));
        }
        else
            deletejob(job_table, pid);
    }
    if (pid == -1 && errno != ECHILD)
        unix_error("waitpid error");
    return;
}

void waitfg(pid_t pid)
{
    job_t* job;
    job = getjobpid(job_table,pid);
    if(job){
        //sleep
        while(pid==fgpid(job_table))
            ;
    }
    return;
}