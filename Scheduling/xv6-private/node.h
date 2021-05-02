#include "param.h"

struct node {
	struct spinlock lock;
	int pid[NPROC];
	int used[NPROC];
	int tickets[NPROC];
};