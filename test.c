#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test.h"
#define MAX_LEN 100
// 采用递归下降的方法处理test的表达式
// 语法：expr := !expr | term -o expr | term -a expr
//      term := -f file | int1 -eq int2 | ...
char **terms;
int idx = 1;
char *getNextStr()
{
    static int over = 0;
    if (over){
        fprintf(stderr, "invalid test expression\n");
        exit(1);
    }
    if (terms[idx])
        return terms[idx++];
    else{
        over = 1;
        return NULL;
    }
        
}
char *checkNextStr()
{
    return terms[idx];
}
int eval_term()
{
    char *str, *str2, *str3;
    int ret;
    struct stat buf;
    str = getNextStr();
    if (!strcmp(str, "-d"))  // 利用stat函数或者access函数得到文件相关的信息
    {
        str2 = getNextStr();
        ret = stat(str2, &buf);
        if (ret)
        {
            perror("stat in test");
            exit(1);
        }
        return S_ISDIR(buf.st_mode);
    }
    else if (!strcmp(str, "-f"))
    {
        str2 = getNextStr();
        ret = stat(str2, &buf);
        if (ret)
        {
            perror("stat in test");
            exit(1);
        }
        return S_ISREG(buf.st_mode);
    }
    else if (!strcmp(str, "-r"))
    {
        str2 = getNextStr();
        if (!str2)
        {
            fprintf(stderr, "invalid test expression\n");
            exit(1);
        }
        return !access(str2, R_OK);
    }
    else if (!strcmp(str, "-s"))
    {
        str2 = getNextStr();
        stat(str2, &buf);
        return buf.st_size > 0;
    }
    else if (!strcmp(str, "-w"))
    {
        str2 = getNextStr();
        if (!str2)
        {
            fprintf(stderr, "invalid test expression\n");
            exit(1);
        }
        return !access(str2, W_OK);
    }
    else if (!strcmp(str, "-x"))
    {
        str2 = getNextStr();
        if (!str2)
        {
            fprintf(stderr, "invalid test expression\n");
            exit(1);
        }
        return !access(str2, X_OK);
    }
    else if (!strcmp(str, "-b"))
    {
        str2 = getNextStr();
        ret = stat(str2, &buf);
        if (ret)
        {
            perror("stat in test");
            exit(1);
        }
        return S_ISBLK(buf.st_mode);
    }
    else if (!strcmp(str, "-c"))
    {
        str2 = getNextStr();
        ret = stat(str2, &buf);
        if (ret)
        {
            perror("stat in test");
            exit(1);
        }
        return S_ISCHR(buf.st_mode);
    }
    else if (!strcmp(str, "-e"))
    { 
        str2 = getNextStr();
        if (!str2)
        {
            fprintf(stderr, "invalid test expression\n");
            exit(1);
        }
        return !access(str2, F_OK);
    }
    else if (!strcmp(str, "-L"))
    {
        str2 = getNextStr();
        ret = stat(str2, &buf);
        if (ret)
        {
            perror("stat in test");
            exit(1);
        }
        return S_ISLNK(buf.st_mode);
    }
    else if (!strcmp(str, "-n"))
    {
        str2 = getNextStr();
        return strlen(str2) > 0;
    }
    else if (!strcmp(str, "-z"))
    {
        str2 = getNextStr();
        return strlen(str2) == 0;
    }
    else
    {
        str2 = getNextStr();
        if (!str2)
        {
            return !strcmp(str, "");   // 字符串比较用strcmp, 整数比较使用atoi转换后比较
        }
        else
        {
            str3 = getNextStr();
            if (!strcmp(str2, "-eq"))
            {
                return atoi(str) == atoi(str3); 
            }
            else if (!strcmp(str2, "-ge"))
            {
                return atoi(str) >= atoi(str3); 
            }
            else if (!strcmp(str2, "-gt"))
            {
                return atoi(str) > atoi(str3); 
            }
            else if (!strcmp(str2, "-le"))
            {
                return atoi(str) <= atoi(str3); 
            }
            else if (!strcmp(str2, "-lt"))
            {
                return atoi(str) < atoi(str3); 
            }
            else if (!strcmp(str2, "-ne"))
            {
                return atoi(str) != atoi(str3); 
            }
            else if (!strcmp(str2, "="))
            {
                return !strcmp(str, str3);
            }
            else if (!strcmp(str2, "!="))
            {
                return strcmp(str, str3);
            }
        }
    }
    return 0;
}
int eval_expr()
{
    int ret1, ret2, res, not = 0;
    char *str;
    if (!strcmp(checkNextStr(), "!"))
    {
        not = 1;
        getNextStr();
    }
    ret1 = eval_term();
    if (ret1 == -1)
    {
        fprintf(stderr, "Error: invalid test expression\n");
        return -1;
    }
    str = getNextStr();
    if (!str)
    {
        res = ret1;
    }
    else if (!strcmp(str, "-a"))
    {
        ret2 = eval_expr();
        if (ret2 == -1)
        {
            fprintf(stderr, "Error: invalid test expression\n");
            return -1;
        }
        res = ret1 && ret2;
    }
    else if (!strcmp(str, "-o"))
    {
        ret2 = eval_expr();
        if (ret2 == -1)
        {
            fprintf(stderr, "Error: invalid test expression\n");
            return -1;
        }
        res = ret1 || ret2;
    }
    else
    {
        fprintf(stderr, "Error: invalid test expression\n");
        return -1;
    }
    return (not) ? !res : res;
}

int do_test(char **cmd_argv)
{
    terms = cmd_argv;
    return eval_expr();
}
