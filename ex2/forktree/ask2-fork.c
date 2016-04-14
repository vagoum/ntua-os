#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

void fork_procs(void)
{
	int status;
	change_pname("A");
	printf("A: Sleeping...\n");

	pid_t b = fork();
	if( b < 0){
		perror("B"); 
		exit(1);
	}
	if (b == 0) {
		change_pname("B");
		printf("B: Sleeping...\n");
		pid_t noe = fork();
		if(noe < 0) {
		perror("mi kaneis lathi");
		exit(1);
		}
		if(noe == 0)
		{
			change_pname("D");
			printf("D: Sleeping... \n");
			sleep(SLEEP_PROC_SEC);
			printf("D: Exiting...\n");
			exit(13);
		}
	
		noe =wait(&status);
		explain_wait_status(noe, status);
		sleep(SLEEP_PROC_SEC);
		printf("B: Exiting...\n");
		exit(19);
	}
		
	pid_t noe2 = fork();
	if(noe2<0){perror(""); exit(1);}
	if (noe2==0)
		{
			change_pname("C");
			printf("C: Sleeping...\n");
			sleep(SLEEP_PROC_SEC);
			printf("C: Exiting...\n");
			exit(17);
		}
		
	pid_t pid = waitpid(-1, &status, 0);
	explain_wait_status(pid, status);
	
	pid =wait(&status);
	explain_wait_status(pid, status);

	printf("A: Exiting...\n");
	sleep(SLEEP_PROC_SEC);
	exit(16);
}
int main(void)
{
	pid_t pid;
	int status;

	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		fork_procs();
		exit(1);
	}


	sleep(SLEEP_TREE_SEC);
	sleep(SLEEP_TREE_SEC);

	show_pstree(getpid());


	pid = wait(&status);
	explain_wait_status(pid, status);

	return 0;
}
