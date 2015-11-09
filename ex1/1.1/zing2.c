#include <unistd.h>
#include <stdio.h>

void zing() {
	char *name = getlogin();
	printf("Welcome to the machine, %s!\n",name);
}


