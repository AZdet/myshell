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
int is_pipe(char* cmd,int cmdlen); 		/*解析管道命令*/
int is_io_redirect(char* cmd,int cmdlen);   /*解析重定向*/
int normal_cmd(char* cmd,int cmdlen,int infd,int outfd,int fork);  /*执行普通命令*/
/*其他函数……. */

char cmd[MAX_LINE + 1]; // + 1 for '\0'，记录总的命令
char *ptr_subcmd[MAX_LINE];  // 在parse之后，记录每个子命令开始的位置
int subcmd_len[MAX_LINE]; //　配合上面的指针，在parse之后，记录每个子命令的长度

void init(){
    /*设置命令提示符*/
    printf("myshell>");
    fflush(stdout);
    /*设置默认的搜索路径*/
    setpath("/bin:/usr/bin");  
    /*……*/
}

int readcommand(){
    fgets(cmd, MAX_LINE, stdin);
    int len = strlen(cmd);
    char *tok;
    int cmd_len, idx =0;
    // structure of a command
    // cmd args [| cmd args]* [[< filename] [> filename]  [>> filename]]* [&]  
    tok = strtok(cmd, " ");
    ptr_subcmd[idx++] = tok;
    while (tok){
        tok = strtok(NULL, " ");
        if (!tok)
            break;
        if (strcmp(tok, "|") && strcmp(tok, "<") && strcmp(tok, ">")  // tok is a subcmd 
        && strcmp(tok, ">>") && strcmp(tok, "&")) {
            ptr_subcmd[idx++] = tok;
        }
    }
}

/*
bg fg jobs
cd clr dir echo exec exit environ help pwd quit time umask
set shift test  unset
*/
int main(void){
    char *args[MAX_LINE/2 + 1]; /* command line arguments */
    int should_run = 1; /* flag to determine when to exit program */
    init();
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
       readcommand();
    }
    return 0;
}
