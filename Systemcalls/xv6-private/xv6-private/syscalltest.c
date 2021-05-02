#include "types.h"
#include "stat.h"
#include "user.h"
#include "syscallinfostruct.h"
#include <stddef.h>
//this is a userprogram, it tests the system calls getsyscallinfo, getallsyscalls, and getsyscallinfostruct

//getsyscallinfo sets a pointer to the amount of times it has been called
//getallsyscalls sets a pointer to the amount of times ANY system call has been called
//getsyscallinfostruct sets the count property of a syscallinfo struct (defined in syscallinfostruct.h)
//to the cuurent count property of another syscallinfo struct that only the kernel has direct access to

//indentprinter is a function to print a string with an indent of num char's of the value indent
//can be used like: 
//	indentprinter(' ', 5, "mystring");
//to print the string mystring preceded by 5 spaces
//mystring could also be variable of type char*
void
indentprinter(char indent, int num, char * string)
{
	int i; 
	for(i = 0; i < num; i++){
		printf(1, "%c", indent);
	}
	printf(1, "%s", string);
}

void initsyscallinfostruct(struct syscallinfostruct *s){
	int counts[] = {-1};
	char *name[] = {"unused", "fork", "exit", "wait", "pipe",
        "read","kill", "exec", "fstat", "chdir",
        "dup", "getpid", "sbrk", "sleep", "uptime",
        "open", "write", "mknod", "unlink", "link",
        "mkdir", "close", "getsyscallinfo", "getallsyscalls", "getsyscallinfostruct"};

    memmove(s->counts, counts, sizeof(counts));
    int i;
    for(i = 0; i < numsyscalls; i++){
    	s->names[i] = name[i];
    }
}

//main function for this test
int
main(int argc, char *argv[]){
	
	int syscallinfo = 0;//int set by getsyscallinfo that returns the amount of times getsyscallinfo has been called
	int allsyscalls = 0;
	
	struct syscallinfostruct s;
	initsyscallinfostruct(&s);//sets names varibale to sys functions names, sets all counts to -1
	char *headers[] = {"Name", "System Call Number", "Count"};

	if(getsyscallinfo(&syscallinfo) == 0 && getallsyscalls(&allsyscalls) == 0 && getsyscallinfostruct(&s) == 0){
		printf(1, "\nThe custom system call getsyscallinfo() has been called: %d time(s).\n", syscallinfo);
		printf(1, "\nThere have been: %d system calls of any type.\n", allsyscalls);
		
		int i;
		int padmid, padright;
		int headmidpad, headrightpad;
		
		printf(1, "\n%s", headers[0]);
		headmidpad = 21 - strlen(headers[0]);
		
		indentprinter(' ', headmidpad, headers[1]);
		headrightpad = 3;

		indentprinter(' ', headrightpad, headers[2]);
		printf(1, "\n");
		
		for(i = 1; i < numsyscalls; i++){
			padmid = headmidpad + strlen(headers[0]) - strlen(s.names[i]);
			padright = headrightpad - 1 + strlen(headers[1]);
			if(i > 9){
				padright--;
			}
			printf(1, "%s", s.names[i]);
			indentprinter(' ', padmid, "");
			printf(1, "%d", i);
			indentprinter(' ', padright, "");
			printf(1, "%d\n", s.counts[i]);
		}
	}
	
	exit();
}