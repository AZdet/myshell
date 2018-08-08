#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_CMD_LEN 10
char Internal_CMDS[][MAX_CMD_LEN + 1] = {"cd", NULL};


int is_internal_cmd(char *cmd){
    int i, len = sizeof(Internal_CMDS) / sizeof(Internal_CMDS[0]);
    for (i = 0; Internal_CMDS[i] != NULL; i++){
        if (!strcmp(Internal_CMDS[i], cmd))
            return 1;
    }
    return 0;
}

int main(){
    // char *a = getenv("PATH");
    // char b[] = "world";
    // a = strcat(a, ":");
    // printf("%s\n", strcat(a, b));
    // char str[MAX_CMD_LEN+1];
    // scanf("%s", str);
    // printf("%d\n", is_internal_cmd(str));
    puts("\033[H\033[J");
}