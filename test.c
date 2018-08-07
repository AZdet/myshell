#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
int test(){
    exit(1);
}
int main(){
    int num;
    mode_t mode;
    int ret = scanf("%o", &mode);
    test();
    printf("mode is %d(%o)\n", mode, mode);
    printf("ret value is %d\n", ret);
}