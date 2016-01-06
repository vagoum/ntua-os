#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "tree.h"
#include "proc-common.h"


#define  SLEEP_PROC_SEC 10
#define  SLEEP_TREE_SEC 3


void recursive_create(struct tree_node *node,int pip) {	

	int res, i ;
	change_pname(node->name);
	

	if ( node->nr_children == 0 ) {
		res = atoi(node->name);
		if ( write(pip, &res, sizeof(res))!=sizeof(res)  ) {
			perror("child write");
			exit(1);
		}
		exit(0);
	}

	int noe[2], status ;

	if ( pipe(noe) <  0 ) {
		perror("pipe create");
		exit(1);
	}
	
	for ( i = 0 ; i < 2; i++ ) {
		pid_t pid = fork();
	

		if ( pid < 0 ) {
			perror("fork");
			exit(1);
		}
		if ( pid == 0 ) {
			recursive_create(node->children+i,noe[1]);
		}	

	}
	int num[2];
	for ( i = 0 ; i < 2; i++ ) {
		
		pid_t pid=wait(&status);
		explain_wait_status(pid, status);
		if ( read(noe[0],&num[i], sizeof(int))!=sizeof(int)   ) {
			perror("read from child pipe");
			exit(1);
		} 
		
	}	
	switch(strcmp(node->name,"+")) {
		case 0:
			res = num[1]+num[0];
			break;
		default:
			res = num[1]*num[0];
	//	default:
	//		exit(1);
	//		break;
	}
//	fprintf(stderr,"%d ",res);
	if ( write(pip, &res, sizeof(res))!=sizeof(res)   ) {
		perror("bad write to parent pipe");
		exit(1);
	}
	exit(0);

}

int main(int argc, char *argv[])
{
	struct tree_node *root;
	int fd[2], status, temp;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
		exit(1);
	}
	if(pipe(fd)<0)
	{
		perror("pipe");
		exit(1);
	}
	root = get_tree_from_file(argv[1]);
//	print_tree(root);

	pid_t pid = fork();

	if ( pid < 0 ) {
		perror("main:fork");
		exit(1);
	}
	if ( pid == 0 ) {
	//	close(fd[0]);
		recursive_create(root,fd[1]);
		exit(1);
	}
//	if ( close(fd[1]) < 0 ) {
//		perror("main:close");
//	}
	pid = wait(&status);
	explain_wait_status(pid, status);

	if(read(fd[0], &temp, sizeof(int))!=sizeof(int)){
		perror("read pipe"); 
	}
	fprintf(stdout, "%d \n",temp);

	return 0;
}	
