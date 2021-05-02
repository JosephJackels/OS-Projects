#include "types.h"
#include "user.h"

int main(int argc, char *argv[])
{
	//takes logfile then
	//takes amount of processes followed by tickets for each process
	//argv[0] = function name, [1] = logfile, [2] = proc count, [3] = proc1 tickets...
	int proccount = atoi(argv[1]);
	
	int i;
	
	char *tickets[proccount];

	if( (argc < 3) || ((argc - 2) != proccount))
	{
		printf(2, "Use of program: prompt$multifork numprocesses proc[1]tickets ... proc[numprocesses]tickets\n\n");
		printf(2, "EX\nmultifork 2 10 15\nWould create a program that should fork twice(this process is just a controlle) where the first child is given 10 tickets and the second child is given 15\n");
		exit();
	}
	for(i = 2; i < argc; i++){
		tickets[i - 2] = argv[i];
	}
	printf(1, "The given count of procedures is: %d, %d children will be created to exec testproc with given amount of tickets\n", proccount, proccount);
	
	int pid=getpid();
	
	printf(1, "The parent pid is: %d\n\n", pid);
	printf(1, "%d children will now be created\n", proccount);
	int child;
	for(i = 0; i < proccount; i++){
		
		child = fork();
		if(child > 0){
			printf(1, "Child number: %d created. PID: %d\n", i + 1, child);
			//int childtickets = atoi(tickets[i])
			settickets(child, atoi(tickets[i]));
		}
		if(child < 0){
			printf(2, "Fork error, exiting now.");
			exit();
		}
		if(child == 0){
			//printf(1, "Child number: %d created from parent: %d with child pid: %d and ticket count: %s\n", i + 1, pid, getpid(), tickets[i]);
			char *args[] = {"testproc", tickets[i]};
			exec("testproc", args);
			
			//above should not return
			//so this should only print and exit if something goes wrong with execution
			printf(2, "execute failed\n");
			exit();
		}
	}
	//wait for all children to finish before exiting
	int wpid;
	while((wpid = wait()) > 0 && wpid != pid){
		//do nothing
	}
	exit();
}