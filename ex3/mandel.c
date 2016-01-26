/*
 * mandel.c
 *
 * A program to draw the Mandelbrot Set on a 256-color xterm.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <semaphore.h>
#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000
/***************************
 * Compile-time parameters *
 ***************************/
/*
*
*
*/
/*
 * Output at the terminal is is x_chars wide by y_chars long
*/
int y_chars = 50;
int x_chars = 90;


/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
*/
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;
	
/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;
int NTHREADS;
sem_t *noe;
/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
int last_line=0;
typedef struct {
	int first;
	int second;
}argsm;
void compute_mandel_line(int line,int *color_val)
{
	/*
	 * x and y traverse the complex plane.
	 */
	double x, y;

	int n;
	int val;

	/* Find out the y value corresponding to this line */
	y = ymax - ystep * line;

	/* and iterate for all points on this line */
	for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {

		/* Compute the point's color value */
		val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
		if (val > 255)
			val = 255;

		/* And store it in the color_val[] array */
		val = xterm_color(val);
		color_val[n] = val;
	}
}

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
	int i;
	
	char point ='@';
	char newline='\n';

	for (i = 0; i < x_chars; i++) {
		/* Set the current color, then output the point */
		set_xterm_color(fd, color_val[i]);
		if (write(fd, &point, 1) != 1) {
			perror("compute_and_output_mandel_line: write point");
			exit(1);
		}
		reset_xterm_color(1);
	}

	/* Now that the line is done, output a newline character */
	if (write(fd, &newline, 1) != 1) {
		perror("compute_and_output_mandel_line: write newline");
		exit(1);
	}
}
void compute_and_output_mandel_line(argsm *arg)
{
	int line=(*arg).second;
	int fd=(*arg).first;
	int color_val[x_chars];
	int i;

	for(i=line;i<y_chars;i+=NTHREADS){
		compute_mandel_line(i, color_val);
		sem_wait(&noe[(line+NTHREADS)%NTHREADS]);
		output_mandel_line(fd, color_val);
		sem_post(&noe[(line+1+NTHREADS)%NTHREADS]);
	}
}
/*
An den soy aresei o kodikas moy, fae to pouli m
An sou aresei tote pali fae to poyli m :)
*/
int main(int argc ,char *argv[])
{
	if(argc<2)
	exit(0);
	NTHREADS=atoi(argv[1]);
	int line;
	noe = malloc(NTHREADS*sizeof(sem_t));
	pthread_t threads[NTHREADS];
	xstep = (xmax - xmin) / x_chars;
	ystep = (ymax - ymin) / y_chars;
	// initialize semaphore
	for(line=0;line<NTHREADS;line++)
	sem_init(&noe[line],0,0); 

	sem_post(&noe[0]);

	for (line = 0; line < NTHREADS; line++) {
		argsm* arg=malloc(sizeof(argsm));
		arg->first=1; //file desctiptr
		arg->second=line;

		if(pthread_create(&threads[line],NULL,compute_and_output_mandel_line,arg)<0){
			printf("error");exit(42);
		}

	}

	for(line=0;line<NTHREADS;line++){
	pthread_join(threads[line],NULL);
	}

	reset_xterm_color(1);
	return 0;
}
