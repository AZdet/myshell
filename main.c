#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#define MAX_LINE 80 /* The maximum length command */
#define MAX_CMD_LEN 7
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
int readcommand();				/*读取用户输入*/
int is_internal_cmd(char* cmd,int cmdlen); /*解析内部命令*/
int do_pipe(char* cmd,int cmdlen); 		/*解析管道命令*/
int io_redirect(char* cmd,int cmdlen);   /*解析重定向*/
int normal_cmd(char* cmd,int cmdlen,int infd,int outfd,int fork);  /*执行普通命令*/
/*其他函数……. */

char cmd[MAX_LINE + 1]; // + 1 for '\0'，记录总的命令
char *argvs[MAX_LINE][MAX_LINE + 1];   // argvs[i][j]为第i个命令中的第j个参数，其中第0个参数就是命令本身, +1 for NULL
int argcs[MAX_LINE]; // argcs[i]为第i个命令的长度
int num_cmd = 0;

int is_pipe;
int is_io_redirect;
FILE *infile;
FILE *outfile;
int is_background;
int is_error;


// void init(){
//     /*设置命令提示符*/
//     printf("myshell>");
//     fflush(stdout);
//     /*设置默认的搜索路径*/
//     setpath("/bin:/usr/bin");  
//     /*……*/
// }

int readCommand(){
    char *tok, *next_tok;
    int num_arg = 0;
    printf(">");
    fgets(cmd, MAX_LINE, stdin);
    //int len = strlen(cmd);
    is_pipe = is_io_redirect = is_background = is_error = 0;
    infile = outfile = NULL;
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
                    infile = fopen(next_tok, "r");
                    if (!infile){
                        fprintf(stderr, "Error: can not open file %s\n", next_tok);
                        is_error = 1;
                        break;
                    }
                }
                else if (!strcmp(tok, ">")){
                    outfile = fopen(next_tok, "w");
                    if (!outfile){
                        fprintf(stderr, "Error: can not open file %s\n", next_tok);
                        is_error = 1;
                        break;
                    }
                }
                else if (!strcmp(tok, ">>")){
                    outfile = fopen(next_tok, "a");
                    if (!outfile){
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

int executeCommand(){
    int i;
    if (is_pipe){
        do_pipe();
    }
    
}
/*
bg fg jobs
cd clr dir echo exec exit environ help pwd quit time umask
set shift test  unset
*/
int main(void){
    //char *args[MAX_LINE/2 + 1]; /* command line arguments */
    int should_run = 1; /* flag to determine when to exit program */
    // init();
    // while (should_run) {
    //     /**
    //     * After reading user input, the steps are: 
    //     *内部命令：
    //     *…..
    //     *外部命令：
    //     * (1) fork a child process using fork()
    //     * (2) the child process will invoke execvp()
    //     * (3) if command included &, parent will invoke wait()
    //     *…..
    //     */
    //    readcommand();
    // }
    readcommand();
    return 0;
}
