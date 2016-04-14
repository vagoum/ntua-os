#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */


/*
 * SIGALRM handler
 */
pid_t curr;
int myitems;
struct noe_y{
	struct noe_y * next;
	pid_t pid;
	char * name;
	enum priority{HIGHT,LOW};

};
typedef struct noe_y noe_t;
noe_t *head,*tail;
void insert(pid_t pid,const char * name)
{
	myitems++;
	noe_t* noe= malloc(sizeof(noe_t));
	noe->pid=pid;
	noe->name=strdup(name);
	noe_t* tmp =head;
	while(tmp->next!=NULL) tmp=tmp->next;
	tmp->next=noe;
	tail=noe;
}
void remove_l(pid_t pid)
{
	noe_t* temp=head;
	while(temp->next->pid!=pid) temp=temp->next;
	noe_t* fordel= temp->next;
	temp->next=temp->next->next;
	free(fordel);
	myitems--;
	if(myitems==0)
		exit(1);	
}
static void
sigalrm_handler(int signum)
{
	if(kill(curr,SIGSTOP)<0)
		perror("stop error");
}

/* 
 * SIGCHLD handler
 */
node_t* getHight()
{
	int count = myitems+1;
	noe_t tmp =head;
	while(tmp->priority!= HIGHT && count != 0)
	{
		count--;
		tmp=tmp->next;
	}
	if(count)
		return tmp;
	return NULL;
}
static void
sigchld_handler(int signum)
{
	int p,status;
//	assert(0 && "Please fill me!");
	for(;;){
		p=waitpid(-1,&status,WUNTRACED | WNOHANG);
		if(p==0)
			break;
		explain_wait_status(p,status);
		if(WIFEXITED(status) || WIFSIGNALED(status))
		{
			if(kill(head->pid,SIGCONT)<0)
				perror("cont error");
			noet_t tmp =getHight();
			if(tmp!=NULL)
				head=tmp;
			curr=head->pid;
			head=head->next;
			tail=tail->next;
			remove_l(curr);
			alarm(SCHED_TQ_SEC);
		}
		if(WIFSTOPPED(status))
		{

			if(kill(head->pid,SIGCONT)<0)
				perror("cont error");
			noet_t tmp =getHight();
			if(tmp!=NULL)
				head=tmp;
			curr=head->pid;
			head=head->next;
			tail=tail->next;
			alarm(SCHED_TQ_SEC);
		}
	}
}

/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void
install_signal_handlers(void)
{
	sigset_t sigset;
	struct sigaction sa;

	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigaddset(&sigset, SIGALRM);
	sa.sa_mask = sigset;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction: sigchld");
		exit(1);
	}

	sa.sa_handler = sigalrm_handler;
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		perror("sigaction: sigalrm");
		exit(1);
	}

	/*
	 * Ignore SIGPIPE, so that write()s to pipes
	 * with no reader do not result in us being killed,
	 * and write() returns EPIPE instead.
	 */
	if (signal(SIGPIPE, SIG_IGN) < 0) {
		perror("signal: sigpipe");
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	int nproc,i;
	myitems=0;
	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */
	nproc = argc; /* number of proccesses goes here */
	pid_t pid;
	head= malloc(sizeof(noe_t));
//	tail= malloc(sizeof(noe_t));
	//tail=head;
	head->next=NULL;
	for(i=1;i<nproc;i++)
	{
		pid=fork();
		if(pid)insert(pid,argv[i]);
		if(pid==0){
			char *newargv[] = {argv[i],NULL,NULL,NULL};
			raise(SIGSTOP);
			execve(argv[i],newargv,NULL);
			}
	}
	head=head->next;
	free(tail->next);
	tail->next=head;
	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc-1);

	//perror("a");
	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	if (nproc == 0) {
		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
		exit(1);
	}
	noet_t tmp =getHight();
	if(tmp!=NULL)
		head=tmp;
	if(kill(head->pid,SIGCONT)<0)
		perror("cont error");
	tmp =getHight();
	if(tmp!=NULL)
		head=tmp;
	curr=head->pid;
	head=head->next;
	tail=tail->next;
	alarm(SCHED_TQ_SEC);

	/* loop forever  until we exit from inside a signal handler. */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
