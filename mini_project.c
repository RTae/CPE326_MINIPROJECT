#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

char * getCommad(char *pid){
    FILE *fp;
    char path[1035];
    char searchCommand[] = "ps -o cmd fp";
    strcat(searchCommand, pid);

    fp = popen(searchCommand, "r");
    if(fp == NULL){
        printf("Failed to retrieve command from pid\n");
        exit(1);
    }

    while (fgets(path, sizeof(path), fp) != NULL ){
        printf("%s", path);
        pid = path;
    }

    pclose(fp);
    return pid;
}

int main (int argc, char * argv[]){
    char *stringCommand;
    char *token;
    int i = 1;

    stringCommand = getCommad(argv[1]);
    token = strtok(stringCommand, "");

    while ( token != NULL)
    {
        argv[i] = token;
        printf("%s\n", argv[i]);
        i++;
        token = strtok(NULL, " ");
        argc++;
    }
    struct user_regs_struct regs;
    int count = 0;
    int status;
    long long temp = 0;
    int isCall = 0;
    pid_t tracee = fork();
    if(tracee == -1){
        perror("fork");
    } else if (tracee == 0){
        printf("child (pid:%d)\n",(int)getpid());
        ptrace(PTRACE_TRACEME,0, NULL, NULL);
        printf("%s\n", argv[1]);
        if(execvp(argv[1], argv+1)==-1){
            perror("exec");
        }
    }else {
        printf("parent (pid:%d)\n", (int)getpid());
        wait(&status);
        while( status == 1407 ){
            ptrace(PTRACE_GETREGS, tracee, NULL, &regs);
            if(!isCall){
                if(temp != regs.orig_rax){
                    printf("SystemCall %lld called with %lld %lld, %lld\n")

                    temp = regs.orig_rax;
                    count++;
                }
            isCall = 1;
            } else{
                isCall = 0;
                ptrace(PTRACE_SYSCALL, tracee, NULL, NULL);
                wait(&status);
            }
        }
    }
    printf("Total number of system calls = %d\n", count);
    return 0;
}