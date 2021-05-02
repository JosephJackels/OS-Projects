#ifndef _SYSCALLINFOSTRUCT_H_
#define _SYSCALLINFOSTRUCT_H_

#define numsyscalls 25
struct syscallinfostruct {
	int counts[numsyscalls];
	char *names[numsyscalls];
};

//void initsyscallinfostruct(struct syscallinfostruct*);


#endif