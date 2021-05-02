#include "types.h"
#include "user.h"

int main(int argc, char const *argv[])
{
	if(argc != 2){
		printf(1, "supply one arguement, file to be created/overwritten ex kernalfilewrite test.txt");
	}

	//setlogfile(argv[1]);
	return 0;
}