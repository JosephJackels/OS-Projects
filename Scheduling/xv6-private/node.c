#include <stddef.h>
#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "node.h"
//#include <stdlib.h>

//node structure
/*
struct node
{
	struct spinlock lock
	int pid[NPROC]
	int used[NPROC]
	int tickets[NPROC]
}	
*/

//adds a node to end of a queue
int
add(struct node *queue, int pid, int tickets)
{
	//find an empty spot
	int *n;
	for(n = queue->used; n < &queue->used[NPROC]; n++){
		if(*n == 0){
			//found a slot!
			ptrdiff_t index = n - queue->used;
			queue->pid[index] = pid;
			queue->used[index] = 1;
			queue->tickets[index] = tickets;
			return 0;
		}
	}

	//could not find a slot
	return 1;
}

//removes a node from anywhere in a queue
int
remove(struct node *queue, int pid){
	
	int *n;
	for(n = queue->pid; n < &queue->pid[NPROC]; n++){
		if(*n == pid){
			//found the slot to remove
			ptrdiff_t index = n - queue->used;
			queue->pid[index] = 0;
			queue->used[index] = 0;
			queue->tickets[index] = 0;
			return 0;
		}
	}
	//could not find
	return 1;
}

//updates the ticket field of a node in a queue
int
updatetickets(struct node *queue, int pid, int tickets){
	int *n;
	for(n = queue->pid; n < &queue->pid[NPROC]; n++){
		if(*n == pid){
			ptrdiff_t index = n - queue->pid;
			queue->tickets[index] = tickets;
			return 0;
		}
	}
	//not found
	return 1;
}

//moves a node from one queue to another
int
movequeue(struct node *from, struct node *to, int pid){
	int *n;
	int tickets;
	for(n = from->pid; n < &from->pid[NPROC]; n++){
		if(*n == pid){
			//found it
			ptrdiff_t index = n - from->pid;
			tickets = from->tickets[index];
			if(remove(from, pid) == 0 && add(to, pid, tickets) == 0){
				return 0;
			}
			//error removing or adding?
			return 1;

		}
	}
	//not found
	return 2;
}

//returns the sum of ticket field in a queue
int
getticketsum(struct node *queue){
	int count = 0;

	for(int i = 0; i < NPROC; i++){
		if(queue->used[i] == 1){
			count += queue->tickets[i];
		}
	}

	return count;
}
//returns 0 if a pid is in a queue, 1 if not;
int
isinqueue(struct node *queue, int pid){
	
	int *n;
	for(n = queue->pid; n < &queue->pid[NPROC]; n++){
		if(*n == pid){
			return 0;
		}
	}
	return 1;
}

int
isempty(struct node *queue){
	int *n;
	for(n = queue->used; n < &queue->used[NPROC]; n++){
		if(*n == 1){
			return 1;
		}
	}

	return 0;
}

int getused(struct node *queue){
	int count = 0;
	int *n;
	for(n = queue->used; n < &queue->used[NPROC]; n++){
		if(*n == 1){
			count++;
		}
	}
	return count;
}

void printqueue(struct node *queue, char *name, int winnerpid){
	
	//print ticket total
	//print item, tickets
	//print winner, tickets
	int winnerindex = -1;
	int totaltickets = getticketsum(queue);
	cprintf("Queue: %s\n", name);
	cprintf("Total Tickets: %d\n", totaltickets);
	cprintf("Contents:\n");
	for(int i = 0; i < NPROC; i++){
		if(queue->used[i] == 1){
			cprintf("PID: %d, Tickets: %d\n", queue->pid[i], queue->tickets[i]);
			if(queue->pid[i] == winnerpid){
				winnerindex = i;
			}
		}
	}
	
	int winnertickets = queue->tickets[winnerindex];
	int chance = 100 * winnertickets / totaltickets;
	cprintf("Winner: %d, Tickets: %d out of %d = %d percent chance\n", winnerpid, winnertickets, totaltickets, chance);
}