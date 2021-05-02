#include "types.h"
#include "stat.h"
#include "user.h"
#include "syscallinfostruct.h"
#include <stddef.h>
//*** this is not currently being used !!! could not figure out how to use self defined .c programs? could use more work?
//some sort of problem with makefile i think
//this functions is currently defined in the testing program syscalltest.c
//so that it can actually be used by the testing program
/*
void initsyscallinfostruct(struct syscallinfostruct *s){
	int counts[numsyscalls] = {-1};
	char *names[numsyscalls] = {"unused", "fork", "exit", "wait", "pipe",
        "read","kill", "exec", "fstat", "chdir",
        "dup", "getpid", "sbrk", "sleep", "uptime",
        "open", "write", "mknod", "unlink", "link",
        "mkdir", "close", "getsyscallinfo", "getallsyscalls", "getsyscallinfostruct"};

    memmove(s->counts, counts, sizeof(counts));
    int i;
    for(i = 0; i < numsyscalls; i++){
    	//s->names[i] = (char *) malloc(sizeof(names[i]));
    	memmove(s->names[i], names[i], sizeof(names[i]));
    }
}*/