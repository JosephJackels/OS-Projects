#include "types.h"
#include "user.h"
#include "pstat.h"
#include <stddef.h>
#include "param.h"
int main(int argc, char *argv[])
{
	printf(1, "Test of pstat struct\n\n");
	struct pstat p;
	int *i;
	if(getpinfo(&p) == 0)
	{
		printf(1, "Successfuly got pinfo\n\n");
		for(i = p.inuse; i < &p.inuse[NPROC]; i++){
			ptrdiff_t index = i - p.inuse;
			printf(1, "Index: %d\n", index);
			printf(1, "Inuse: %d\n", p.inuse[index]);
			printf(1, "Pid: %d\n", p.pid[index]);
			printf(1, "Hticks: %d\n", p.hticks[index]);
			printf(1, "Lticks: %d\n", p.lticks[index]);
			printf(1, "Tickets: %d\n", p.tickets[index]);
			printf(1, "Queue: %d\n", p.queue[index]);
			printf(1, "\n");
		}
	} else {
		printf(1, "Could not get pinfo\n");
	}
	exit();
}