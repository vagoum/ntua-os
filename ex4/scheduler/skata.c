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
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */


pid_t curr;
int myitems,ids;
typedef enum {LOW, HIGH} priority;

struct noe_y{
	struct noe_y * next;
	pid_t pid;
	int id;
	char * name;
	priority pr;

};
typedef struct noe_y noe_t;
noe_t *head,*current;
void insert(pid_t pid,const char * name)
{
	myitems++;
	ids++;

	noe_t* noe= malloc(sizeof(noe_t));
	noe->pr = LOW;
	noe->pid=pid;
	noe->name=strdup(name);
	noe->id = ids;
	noe_t* tmp =head;
	while(tmp->next!=NULL) tmp=tmp->next;
	tmp->next=noe;
}
void remove_l(pid_t pid)
{
	noe_t* temp=head;
	while(temp->next!=NULL && temp->next->pid!=pid) temp=temp->next;
	if(temp->next==NULL)
		return;
	if(current == temp->next)
		{
			if (current->next==NULL)
				current=head->next;
			else
				current=current->next;
		}
	noe_t* fordel= temp->next;
	temp->next=temp->next->next;
	free(fordel);
	myitems--;
	if(myitems==0 )
		exit(1);	
}

/* Print a list of all tasks currently being scheduled.  */
static void
sched_print_tasks(void)
{
	noe_t *temp;
	int i;

	for (temp = head->next, i = 0; i < myitems ; temp=temp->next, i++) {
		printf("Name:%s \t ID: %d PID: %ld Priority: %d", temp->name, temp->id, (long)temp->pid, temp->pr);
		if ( temp->pid == current->pid ) printf(" CURRENT ");
		puts("\n");
	}
}

/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */
static int
sched_kill_task_by_id(int id)
{
	noe_t *temp = head;
	while ( temp->next!=NULL&& temp->next->id != id ) temp=temp->next;
	if(temp->next==NULL)
		return 1;
	kill(temp->next->pid, SIGKILL);

        remove_l(temp->next->pid);
	printf("Killed process with id: %d", id);
	return 0;
}


/* Create a new task.  */
static void
sched_create_task(char *executable)
{
        pid_t pid = fork();
	if ( pid != 0 ) insert(pid, executable);
	
	if ( pid == 0 ) {
		char *newargv[] = {executable, NULL, NULL, NULL};
		raise(SIGSTOP);
		execve(executable, newargv, NULL );
	}

}


static void
sched_set_low_prio(int id)
{
    noe_t *temp = head;
    while (temp->next!=NULL && temp->next->id != id) temp = temp->next;
    temp->next->pr = LOW;
}

static void
sched_set_high_prio(int id)
{
    noe_t *temp = head;
    while (temp->next!=NULL && temp->next->id != id) temp = temp->next;
    temp->next->pr = HIGH;
}


/* Process requests by the shell.  */
static int
process_request(struct request_struct *rq)
{
	switch (rq->request_no) {
		case REQ_PRINT_TASKS:
			sched_print_tasks();
			return 0;

		case REQ_KILL_TASK:
			return sched_kill_task_by_id(rq->task_arg);

		case REQ_EXEC_TASK:
			sched_create_task(rq->exec_task_arg);
			return 0;

                case REQ_HIGH_TASK:
			sched_set_high_prio(rq->task_arg);
			return 0;

		case REQ_LOW_TASK:
			sched_set_low_prio(rq->task_arg);

		default:
			return -ENOSYS;
	}
}

/* 
 * SIGALRM handler
 */

static void
sigalrm_handler(int signum)
{
	if(kill(curr,SIGSTOP)<0)
		perror("stop error");
}

/* 
 * SIGCHLD handler
 */
void next_elem()
{
	int counter=myitems;
	noe_t *temp=current;
	while(temp->next!=NULL && temp->next->pr!=HIGH) {temp=temp->next;counter--;}
	if(temp->next==NULL)
	{
		temp=head;
		while(temp->next!=NULL && temp->next->pr!=HIGH && counter) {temp=temp->next;counter--;}
		if(counter)
			current=temp->next;
		else
			{
				if(current->next!=NULL)
					current=current->next;
				else
					current=head->next;
			}
	}
	else
		current=temp->next;
	
}
static void
sigchld_handler(int signum)
{
	int p,status;
	noe_t *temp = head;
//	assert(0 && "Please fill me!");
	for(;;){
		p=waitpid(-1,&status,WUNTRACED | WNOHANG);
		if(p==0)
			break;
		explain_wait_status(p,status);
		if(WIFEXITED(status) || WIFSIGNALED(status))
		{
			remove_l(p);
			if(p==curr){
				next_elem();
				curr=current->pid;
				if (kill(current->pid, SIGCONT) < 0) {
					perror("cont error");
				}
					alarm(SCHED_TQ_SEC);
			}
			
		}
		if(WIFSTOPPED(status))
		{

			//if(current->next!=NULL)
			//	current=current->next;
			//else
			//	current=head->next;
			next_elem();
			curr=current->pid;
			if(kill(current->pid,SIGCONT)<0)
				{perror("cont error");exit(2);}
			alarm(SCHED_TQ_SEC);
		}
	}
}



/* Disable delivery of SIGALRM and SIGCHLD. */
static void
signals_disable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
		perror("signals_disable: sigprocmask");
		exit(1);
	}
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void
signals_enable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
		perror("signals_enable: sigprocmask");
		exit(1);
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

static void
do_shell(char *executable, int wfd, int rfd)
{
	char arg1[10], arg2[10];
	char *newargv[] = { executable, NULL, NULL, NULL };
	char *newenviron[] = { NULL };

	sprintf(arg1, "%05d", wfd);
	sprintf(arg2, "%05d", rfd);
	newargv[1] = arg1;
	newargv[2] = arg2;

	raise(SIGSTOP);
	execve(executable, newargv, newenviron);

	/* execve() only returns on error */
	perror("scheduler: child: execve");
	exit(1);
}

/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
static pid_t
sched_create_shell(char *executable, int *request_fd, int *return_fd)
{
	pid_t p;
	int pfds_rq[2], pfds_ret[2];

	if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
		perror("pipe");
		exit(1);
	}

	p = fork();
	if (p < 0) {
		perror("scheduler: fork");
		exit(1);
	}

	if (p == 0) {
		/* Child */
		close(pfds_rq[0]);
		close(pfds_ret[1]);
		do_shell(executable, pfds_rq[1], pfds_ret[0]);
		assert(0);
	}
	/* Parent */
	close(pfds_rq[1]);
	close(pfds_ret[0]);
	*request_fd = pfds_rq[0];
	*return_fd = pfds_ret[1];
	return p;
}

static void
shell_request_loop(int request_fd, int return_fd)
{
	int ret;
	struct request_struct rq;

	/*
	 * Keep receiving requests from the shell.
	 */
	for (;;) {
		if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
			perror("scheduler: read from shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}

		signals_disable();
		ret = process_request(&rq);
		signals_enable();

		if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
			perror("scheduler: write to shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	int nproc, i;
	myitems = 0;
	ids=0;
	pid_t p;
	/* Two file descriptors for communication with the shell */
	static int request_fd, return_fd;

	/* Create the shell. */
	p = sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);
	/* TODO: add the shell to the scheduler's tasks */


	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */

	nproc = argc; /* number of proccesses goes here */
	pid_t pid;
	head= malloc(sizeof(noe_t));
	head->next=NULL;
	//Add shell
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
	insert(p, "shell");
	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc-1);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	if (nproc == 0) {
		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
		exit(1);
	}
	current=head;
	next_elem();
	if(kill(current->pid,SIGCONT)<0)
		perror("cont error");
	curr=current->pid;
//	if(head->next->next!=NULL)
	//	current=head->next->next;
	//else
	//	current=head->next;
//	kill(curr->pid,SIGSTOP);
        alarm(SCHED_TQ_SEC);


	shell_request_loop(request_fd, return_fd);

	/* Now that the shell is gone, just loop forever
	 * until we exit from inside a signal handler.
	 */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
