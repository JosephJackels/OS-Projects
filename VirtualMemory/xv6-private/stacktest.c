#include "types.h"
#include "stat.h"
#include "user.h"

//this test show the current stack top address, creates enough variables to cause the stack to grow backwards a page,
//showing that the new top has been moved down a page because the process has started using a new page

void stackGrowPage(){
	int b[2000];
	int i;
	for(i = 0; i < 2000; i++){
		b[i] = i;
	}
	printf(1, "b[0] = %d &b[0] = 0x%x\n b[final] = %d, &b[final] = 0x%x\n", b[0], &b[0], b[i - 1], &b[i - 1]);
	//return b[i-1];
}

int main(int argc, char const *argv[])
{
	/* code */
	printstack();
	stackGrowPage();
	printstack();
	exit();
}
