#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include "job.h"
#include "test.h"
#define MAX_LINE 80   /* The maximum length command */
#define MAX_CMD_LEN 7 /* The maximum length of a command name */

char Internal_CMDS[][MAX_CMD_LEN + 1] = {"cd", "clr", "dir", "echo",
                                         "environ", "exit", "help", "pwd", "quit", "time", "umask",
                                         "bg", "fg", "jobs", "exec", "set", "shift", "test", "unset"};

void init();
void setpath(char *newpath);                                                     /*设置搜索路径*/
int readCmd();                                                                   /*读取用户输入，并做好相应的处理，使得子命令相互分开*/
int executeCmd(int cmd_idx, int *ptr_argc, char *argv[], char *envp[]);          /* 执行命令，分为内部命令和外部命令 */
int is_internal_cmd(char *cmd);                                                  /*判断内部命令*/
int do_internal_cmd(char **cmd_argv, int *ptr_argc, char *argv[], char *envp[]); /*执行内部命令*/
int do_bgfg(char *argv[]);                                                       /*执行fg或bg命令*/
void waitfg(pid_t pid);                                                          /*等待pid的进程，直到它不再是前台进程*/

int is_pipe;        /*判断管道命令*/
int is_io_redirect; /*判断重定向*/
int infile;         /* 重定向中对应的输入文件描述符，默认值为-1，为-1时使用stdin */
int outfile;        /* 重定向中对应的输出文件描述符，默认值为-1，为-1时使用stdout */
int is_background;  /*判断后台命令*/
int is_error;       /*判断错误命令*/

//int normal_cmd(char *cmd, int cmdlen, int infd, int outfd, int fork); /*执行普通命令*/

char cmd[MAX_LINE + 1];                  // 记录一次输入shell的总命令，+ 1 for '\0'
char *cmd_argvs[MAX_LINE][MAX_LINE + 1]; // cmd_argvs[i][j]为第i个命令中的第j个参数，其中第0个参数就是命令本身, +1 for NULL
int argcs[MAX_LINE];                     // argcs[i]为第i个命令的长度
int num_cmd = 0;                         // 总命令中子命令的个数

job_t job_table[MAXJOBS]; // 记录所有job

void sigint_handler(int sig);  // ctrl+c信号处理
void sigtstp_handler(int sig); // ctrl+z信号处理
void sigchld_handler(int sig); // CHLD信号处理

int should_run = 1; /* flag to determine when to exit program */
void prompt();

int main(int argc, char *argv[], char *envp[])
{

    int i;
    int tmp_in, tmp_out;
    /* shell 初始化 */
    init();
    /* 进入循环 */
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

        // 读入命令，有错误则从头等待下一条输入
        readCmd();
        if (is_error)
            continue;
        // 备份stdin和stdout的文件描述符
        tmp_in = dup(0);
        tmp_out = dup(1);

        if (infile == -1)
            infile = dup(tmp_in); // infile为默认值，使用stdin，否则已经设置好

        for (i = 0; i < num_cmd; i++)
        {
            // 把infile的内容作为输入，或者为第一条命令中的重定向输入文件或stdin，或者为中间命令中前一条命令输出对应的输入管道
            dup2(infile, 0);
            close(infile);
            if (i == num_cmd - 1) // 到达最后一条命令，设置输出文件
            {
                if (outfile == -1)
                    outfile = dup(tmp_out); // outfile为默认值，使用stdout，否则已经设置好
            }
            else
            {
                int fdpipe[2]; // 设置中间命令间的管道
                pipe(fdpipe);
                infile = fdpipe[0];
                outfile = fdpipe[1];
            }
            // 把outfile的内容作为输出
            dup2(outfile, 1);
            close(outfile);

            // 完成IO定向，执行子命令
            executeCmd(i, &argc, argv, envp);

            // 恢复stdin, stdout的初始值
            dup2(tmp_in, 0);
            dup2(tmp_out, 1);
        }
    }
    return 0;
}

int is_internal_cmd(char *cmd)
{
    int i, len = sizeof(Internal_CMDS) / sizeof(Internal_CMDS[0]);
    for (i = 0; i < len; i++)
    {
        if (!strcmp(Internal_CMDS[i], cmd))
            return 1;
    }
    return 0;
}

void sigint_handler(int sig)
{
    pid_t pid = fgpid(job_table);

    if (pid != 0)
    {
        kill(-pid, sig);
        printf("pid: %d, int\n", pid);
    }
    else
    {
        printf(" catch Ctrl+C without any jobs\n");
        prompt();
        fflush(stdout);
    }
    return;
}
void sigtstp_handler(int sig)
{
    pid_t pid = fgpid(job_table);

    if (pid != 0)
    {
        kill(-pid, sig);
    }
    else
    {
        printf(" catch Ctrl+Z without any jobs\n");
        prompt();
        fflush(stdout);
    }
    return;
}
void sigchld_handler(int sig)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) // 等当前进程所有的子进程
    {
        if (WIFSTOPPED(status))
        {
            //change state if stopped
            job_t *job = getjobpid(job_table, pid);
            job->state = ST;
            int jid = pid2jid(job_table, pid);
            printf("Job [%d] (%d) Stopped by signal %d\n", jid, pid, WSTOPSIG(status));
        }
        else if (WIFSIGNALED(status))
        {
            //delete is signaled
            int jid = pid2jid(job_table, pid);
            printf("Job [%d] (%d) terminated by signal %d\n", jid, pid, WTERMSIG(status));
            deletejob(job_table, pid);
        }
        else if (WIFEXITED(status))
        { // child terminated normally
            //exited
            int jid = pid2jid(job_table, pid);
            printf("Job [%d] (%d) done\n", jid, pid);
            prompt();
            deletejob(job_table, pid);
        }
        
    }
    return;
}

void waitfg(pid_t pid)
{
    job_t *job;
    job = getjobpid(job_table, pid);
    if (job)
    {
        //sleep
        while (pid == fgpid(job_table))
            sleep(1);
    }
    return;
}

void prompt(){
    printf("%s >", getcwd(NULL, 0));
    fflush(stdout);
}
// typedef void handler_t(int);
// handler_t *Signal(int signum, handler_t *handler)
// {
//     struct sigaction action, old_action;

//     action.sa_handler = handler;
//     sigemptyset(&action.sa_mask); /* block sigs of type being handled */
//     action.sa_flags = SA_RESTART; /* restart syscalls if possible */

//     if (sigaction(signum, &action, &old_action) < 0)
// 	    perror("Signal error");
//     return (old_action.sa_handler);
// }

void init()
{
    signal(SIGINT, sigint_handler);   /* ctrl-c */
    signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
    signal(SIGCHLD, sigchld_handler); /* SIGCHLD */
    printf("myshell\n");              /* 打印shell初始化信息 */
    fflush(stdout);
    initjob_table(job_table); /* 初始化job table */
    //setpath("/bin:/usr/bin"); /*　设置搜索路径　*/
}

/* 设置PATH环境变量 */
void setpath(char *newpath)
{
    char *path = getenv("PATH");
    path = strcat(path, ":");
    setenv("PATH", strcat(path, newpath), 1);
}

int do_bgfg(char *argv[])
{
    pid_t pid;
    job_t *job;
    int jid;
    if (!argv[1])
    {
        fprintf(stderr, "Error: bg or fg takes one argument!\n");
        is_error = 1;
        return 1;
    }
    if (argv[1][0] == '%') // 输入job号
    {
        jid = atoi(&argv[1][1]);
        job = getjobjid(job_table, jid);
        if (job == NULL)
        {
            printf("%s: No such job\n", argv[1]);
            return 1;
        }
        else
        {
            pid = job->pid;
        }
    }
    else if (isdigit(argv[1][0])) // 输入pid号
    {
        pid = atoi(argv[1]);
        job = getjobpid(job_table, pid);
        if (job == NULL)
        {
            printf("(%d): No such process\n", pid);
            return 1;
        }
    }
    else
    {
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        return 1;
    }

    kill(-pid, SIGCONT); // 让可能被停止的进程继续工作

    if (!strcmp(argv[0], "fg"))
    {
        job->state = FG;
        waitfg(job->pid);
    }
    else
    {
        job->state = BG;
        printf("[%d] (%d) %s\n", job->jid, job->pid, job->cmdline);
    }
    return 0;
}

int do_internal_cmd(char **cmd_argv, int *ptr_argc, char *argv[], char *envp[])
{
    int i, ret;
    if (!strcmp(cmd_argv[0], "cd"))
    {
        char *cur_path;
        if (!cmd_argv[1])
        {
            cur_path = getcwd(NULL, 0);
            printf("current path is: %s\n", cur_path);
            free(cur_path);
        }
        else
        {
            if (cmd_argv[2])
            {
                fprintf(stderr, "Error: cd only takes one argument!\n");
                is_error = 1;
                return 1;
            }
            ret = chdir(cmd_argv[1]);
            cur_path = getcwd(NULL, 0);
            setenv("PWD", cur_path, 1);
        }
        if (ret)
        {
            perror("chdir");
            return 1;
        }
    }
    else if (!strcmp(cmd_argv[0], "clr"))
    {
        if (cmd_argv[1])
        {
            fprintf(stderr, "Error: cls takes no argument!\n");
            is_error = 1;
            return 1;
        }
        printf("\033c"); // 特殊的terminal code，可以通过man console_codes查看详情
    }
    else if (!strcmp(cmd_argv[0], "dir"))
    {
        if (!cmd_argv[1])
        {
            fprintf(stderr, "Error: dir takes one argument!\n");
            is_error = 1;
            return 1;
        }
        else if (cmd_argv[2])
        {
            fprintf(stderr, "Error: dir takes only one argument!\n");
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
            fprintf(stderr, "Error: echo takes one argument!\n");
            is_error = 1;
            return 1;
        }
        else if (cmd_argv[2])
        {
            fprintf(stderr, "Error: echo takes only one argument!\n");
            is_error = 1;
            return 1;
        }
        puts(cmd_argv[1]);
    }
    else if (!strcmp(cmd_argv[0], "environ"))
    {
        if (cmd_argv[1])
        {
            fprintf(stderr, "Error: help takes no argument!\n");
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
            fprintf(stderr, "Error: exit takes one argument!\n");
            is_error = 1;
            return 1;
        }
        else if (cmd_argv[2])
        {
            fprintf(stderr, "Error: exit takes only one argument!\n");
            is_error = 1;
            return 1;
        }
        int ret_code, ret;
        ret = sscanf(cmd_argv[1], "%d", &ret_code);
        if (ret != 1)
        {
            fprintf(stderr, "Error: invalid return code after exit!\n");
            is_error = 1;
            return 1;
        }
        exit(ret_code);
    }
    else if (!strcmp(cmd_argv[0], "help"))
    {
        if (cmd_argv[1])
        {
            fprintf(stderr, "Error: help takes no argument!\n");
            is_error = 1;
            return 1;
        }
        system("cat readme | more");
    }
    else if (!strcmp(cmd_argv[0], "pwd"))
    {
        if (cmd_argv[1])
        {
            fprintf(stderr, "Error: pwd takes no argument!\n");
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
            fprintf(stderr, "Error: quit takes no argument!\n");
            is_error = 1;
            return 1;
        }
        should_run = 0;
    }
    else if (!strcmp(cmd_argv[0], "time"))
    {
        if (cmd_argv[1])
        {
            fprintf(stderr, "Error: time takes no argument!\n");
            is_error = 1;
            return 1;
        }
        time_t rawtime;
        struct tm *timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        printf("%s", asctime(timeinfo));
    }
    else if (!strcmp(cmd_argv[0], "umask"))
    {
        if (!cmd_argv[1])
        {
            mode_t mode = umask(0000);
            printf("%o\n", mode);
            umask(mode);
            return 0;
        }
        else if (cmd_argv[2])
        {
            fprintf(stderr, "Error: umask takes only one argument!\n");
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
            fprintf(stderr, "Error: invalid umask mode!\n");
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
            fprintf(stderr, "Error: set takes two arguments!\n");
            is_error = 1;
            return 1;
        }
        else if (cmd_argv[3])
        {
            fprintf(stderr, "Error: set takes only two arguments!\n");
            is_error = 1;
            return 1;
        }
        int ret = setenv(cmd_argv[1], cmd_argv[2], 1); // overwrite = 1
        if (ret)
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
        if (cmd_argv[1])
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
            fprintf(stderr, "Error: unset takes one argument!\n");
            is_error = 1;
            return 1;
        }
        else if (cmd_argv[2])
        {
            fprintf(stderr, "Error: unset takes only one argument!\n");
            is_error = 1;
            return 1;
        }
        int ret = unsetenv(cmd_argv[1]);
        if (ret)
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
    prompt();
    while (fgets(cmd, MAX_LINE, stdin) == NULL || !strcmp(cmd, "\n"))
        prompt();
    ;
    if (strlen(cmd) > 0)
        cmd[strlen(cmd) - 1] = '\0';
    //memset(cmd_argvs, 0, sizeof(cmd_argvs)); //sizeof(char) * MAX_LINE * (MAX_LINE + 1));
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
    cmd_argvs[num_cmd][num_arg] = NULL;
    num_cmd++;
}

int executeCmd(int cmd_idx, int *ptr_argc, char *argv[], char *envp[])
{
    sigset_t mask;
    int ret;
    if (is_internal_cmd(cmd_argvs[cmd_idx][0])) // 内部命令直接执行
    {
        do_internal_cmd(cmd_argvs[cmd_idx], ptr_argc, argv, envp);
    }
    else
    {
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL); // 屏蔽SIGCHLD以防止race condition

        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            is_error = 1;
        }
        else if (!pid)
        { // child process

            // 把子进程从默认的foreground group里放到和自己pid一样的新的group里
            setpgid(0, 0);
            sigprocmask(SIG_UNBLOCK, &mask, NULL); // 将SIGCHLD解锁
            ret = execvp(cmd_argvs[cmd_idx][0], cmd_argvs[cmd_idx]); // 子进程exec执行命令
            if (ret < 0)
            {
                perror("execvp");
                exit(1);
            }
            exit(0);
        }
        else
        { // parent process
            JOBSTATE state = (is_background) ? BG : FG;
            addjob(job_table, pid, state, cmd_argvs[cmd_idx][0]); // 先把新的job加入到job table里
            sigprocmask(SIG_UNBLOCK, &mask, NULL);                // 然后解锁SIGCHLD

            if (is_background) //　子进程是后台进程
            {
                printf("[%d] (%d) %s\n", pid2jid(job_table, pid), pid, cmd_argvs[cmd_idx][0]);
            }
            else // 子进程是前台进程
            {
                waitfg(pid); // 等待子进程结束,直到子进程不再是前台进程了
            }
        }
    }
}