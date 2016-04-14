#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

void fork_procs(struct tree_node *root)
{
	int i, status;
	pid_t pid[root->nr_children];
	printf("PID = %ld, name %s, starting...\n",
			(long)getpid(), root->name);
	change_pname(root->name);
	for ( i = 0; i < root->nr_children; i++ ) {
		pid[i] = fork();
		if ( pid[i] < 0 ) {
			perror("fork error");
			exit(1);
		}
		if ( pid[i] == 0 ) {
			fork_procs(root->children+i);
			exit(1);
		}
	}	

	wait_for_ready_children(root->nr_children);
	raise(SIGSTOP);
	printf("PID = %ld, name = %s is awake\n",
		(long)getpid(), root->name);
	
	while(i--) {
		kill(pid[i], SIGCONT);
		wait(&status);
		explain_wait_status(pid[i], status);
	}
	printf("I am %s and I have finished\n", root->name);
	exit(0);
}

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	struct tree_node *root;

	if (argc < 2){
		fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
		exit(1);
	}

	root = get_tree_from_file(argv[1]);

	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		fork_procs(root);
		exit(1);
	}

	wait_for_ready_children(1);


	show_pstree(pid);

	kill(pid, SIGCONT);

	wait(&status);
	explain_wait_status(pid, status);

	return 0;
}
