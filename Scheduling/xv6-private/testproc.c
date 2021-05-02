#include "types.h"
#include "user.h"
//this program should be called via multifork.c
//as part of testing the scheduler for this project
int main(int argc, char *argv[]){
	//check that paramaters are given properly
	/*if(argc != 2){
		printf(2,"Error with arguments in created testproc\n");
		exit();
	}
	int tickets = atoi(argv[1]);
	//printf(1, "Process: %d with tickets: %d\n", getpid(), tickets);
	if(tickets < 1){
		printf(2, "Negative or zero amount of tickets given! it will be set to 1\n");
		tickets = 1;
	}

	//set ticket level for process
	if(settickets(tickets) < 0){
		printf(2, "settickets error\n");
		exit();
	}*/
	
	//float counter = 0;
	for(int i = 1; i < 500; i++){
		yield();
	}
	//printf(1, "Count for process: %d, with tickets: %d, finished with value: %d\n\n", getpid(), tickets, counter);
	exit();
}