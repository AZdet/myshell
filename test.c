#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "test.h"
//
//
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
    if (!strcmp(str, "-d"))
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
        // S_ISREG
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
        // access
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
        // access
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
        // access
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
        //S_ISBLK
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
        //S_ISCHR
    }
    else if (!strcmp(str, "-e"))
    { // exists
        // access
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
        // S_ISLNK
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
            return !strcmp(str, "");
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

// int main()
// {   

//     printf("res: %d\n", do_test());
// }