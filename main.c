#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#define MAX_LINE 80 /* The maximum length command */
#define MAX_CMD_LEN 7
#define MAX_JOBS 30
typedef enum {
// basic functionality
    cd,
    clr,
    dir,
    echo,   
    environ,
    help,
    pwd,
    quit,
    time,
    umask,
// manage processes
    bg,
    fg, 
    jobs,
    exec,
// shell programming
    set,
    shift,
    test,
    unset
} 
COMMEND;

char Internal_CMDS[][MAX_CMD_LEN+1] = {"cd","clr","dir","echo", 
"environ","help","pwd","quit","time","umask",
"bg","fg", "jobs","exec","set","shift","test","unset"};


void init();  
void setpath(char* newpath);   	/*设置搜索路径*/
int readCmd();				/*读取用户输入*/
int is_internal_cmd(char* cmd); /*解析内部命令*/
int do_pipe(char* cmd,int cmdlen); 		/*解析管道命令*/
int io_redirect(char* cmd,int cmdlen);   /*解析重定向*/
int normal_cmd(char* cmd,int cmdlen,int infd,int outfd,int fork);  /*执行普通命令*/
/*其他函数……. */

char cmd[MAX_LINE + 1]; // + 1 for '\0'，记录总的命令
char *argvs[MAX_LINE][MAX_LINE + 1];   // argvs[i][j]为第i个命令中的第j个参数，其中第0个参数就是命令本身, +1 for NULL
int argcs[MAX_LINE]; // argcs[i]为第i个命令的长度
int num_cmd = 0;

char *job_table[MAX_JOBS];  // 所有后台工作的信息，格式为“job_num  pid  cmd”
int job_num_in_use[MAX_JOBS];
int job_num = 0;

int is_pipe;
int is_io_redirect;
int infile;  // 
int outfile; // 
int is_background;
int is_error;

int should_run = 1; /* flag to determine when to exit program */


// void init(){
//     /*设置命令提示符*/
//     printf("myshell>");
//     fflush(stdout);
//     /*设置默认的搜索路径*/
//     setpath("/bin:/usr/bin");  
//     /*……*/
// }
int do_internal_cmd(char **argv){
    int i;
    if (!strcmp(argv[0], "cd")){
        
    }
    else if (!strcmp(argv[0], "cls")){

    }
    else if (!strcmp(argv[0], "dir")){
        
    }
    else if (!strcmp(argv[0], "echo")){
        
    }
    else if (!strcmp(argv[0], "environ")){
        
    }
    else if (!strcmp(argv[0], "help")){
        system("cat readme | more");
    }
    else if (!strcmp(argv[0], "pwd")){
        char *pwd = getcwd(NULL, 0);
        printf("%s\n", pwd);
        free(pwd);
    }
    else if (!strcmp(argv[0], "quit")){
        should_run = 0;
    }
    else if (!strcmp(argv[0], "time")){
        
    }
    else if (!strcmp(argv[0], "umask")){
        
    }
    else if (!strcmp(argv[0], "bg")){
        
    }
    else if (!strcmp(argv[0], "fg")){
        
    }
    else if (!strcmp(argv[0], "jobs")){
        
    }
    else if (!strcmp(argv[0], "exec")){
        
    }
    else if (!strcmp(argv[0], "set")){
        
    }
    else if (!strcmp(argv[0], "shift")){
        
    }
    else if (!strcmp(argv[0], "test")){
        
    }
    else if (!strcmp(argv[0], "unset")){
        
    }

}

int readCmd(){
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
    tok = strtok(cmd, " ");   // strtok会自动加入'\0'
    argvs[num_cmd][num_arg++] = tok;
    while (tok){
        tok = strtok(NULL, " ");
        if (!tok)
            break;
        if (!strcmp(tok, "|")){  // 结束当前指令，开始下一条子命令
            argvs[num_cmd][num_arg] = NULL;
            argcs[num_cmd] = num_arg;
            num_cmd++;
            num_arg = 0;
        }
        else if (!strcmp(tok, "<") || !strcmp(tok, ">") 
        || !strcmp(tok, ">>") || !strcmp(tok, "&")) {   // 达到命令的末尾部分
            next_tok = strtok(NULL, " ");
            if (!strcmp(tok, "&")){
                is_background = 1;
                if (next_tok){
                    fprintf(stderr, "Error: unexpected token after &\n");
                    is_error = 1; 
                }
                break;   // 到＆后，总的命令应该停止
            }
            else{
                if (!next_tok){
                    fprintf(stderr, "Error: unexpected end after %s\n", tok);
                    is_error = 1;
                    break;
                }
                else if (!strcmp(tok, "<")){
                    //infile = fopen(next_tok, "r");
                    infile = open(next_tok, O_RDONLY);
                    if (infile == -1){
                        fprintf(stderr, "Error: can not open file %s\n", next_tok);
                        is_error = 1;
                        break;
                    }
                }
                else if (!strcmp(tok, ">")){
                    // outfile = fopen(next_tok, "w");
                    outfile = open(next_tok, O_WRONLY | O_CREAT | O_TRUNC);
                    if (outfile == -1){
                        fprintf(stderr, "Error: can not open file %s\n", next_tok);
                        is_error = 1;
                        break;
                    }
                }
                else if (!strcmp(tok, ">>")){
                    //outfile = fopen(next_tok, "a");
                    outfile = open(next_tok, O_WRONLY | O_CREAT | O_APPEND);
                    if (outfile == -1){
                        fprintf(stderr, "Error: can not open file %s\n", next_tok);
                        is_error = 1;
                        break;
                    }
                }
            }
        }
        else{  // 仍在同一条子命令中，继续添加参数
            argvs[num_cmd][num_arg++] = tok;
        }
        
    }
}

int executeCmd(int cmd_idx){
    if (is_internal_cmd(argvs[cmd_idx][0])){
        do_internal_cmd(argvs[cmd_idx]);
    }
    else{
        pid_t pid = fork();
        if (pid < 0){
            perror("fork");
            is_error = 1;
        }
        else if (!pid){   // child process
            // 添加job_table记录, should be in main process
            // int tmp_job_num = (job_num + 1) % MAX_CMD_LEN;
            // job_num = ;
            // job_num_in_use[job_num] = 1;

            execvp(argvs[cmd_idx][0], argvs[cmd_idx]);
            // 删除job_table记录

        }
        else{  // parent process
            if (!is_background){
                waitpid(pid, NULL, NULL);
            }
        }
    }
}

/*
bg fg jobs
cd clr dir echo exec exit environ help pwd quit time umask
set shift test  unset
*/
int main(void){
    //char *args[MAX_LINE/2 + 1]; /* command line arguments */
    
    int i;
    int tmp_in, tmp_out;
    //init();
    while (should_run) {
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
        for (i = 0; i < num_cmd; i++){
            dup2(infile, 0);
            close(infile);
            if (i == num_cmd - 1){
                if (outfile == -1)
                    outfile = dup(tmp_out);
            }
            else{
                int fdpipe[2];
                pipe(fdpipe);
                infile = fdpipe[0];
                outfile = fdpipe[1];
            }
            dup2(outfile, 1);
            close(outfile);

            // 完成IO定向，执行子命令
            executeCmd(i);
            if (is_error)
                break;

            dup2(tmp_in, 0);
            dup2(tmp_out, 1);

        }
    }
    return 0;
}
