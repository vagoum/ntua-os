#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>


#ifndef BUF_SIZE
#define BUF_SIZE 10
#endif


char const *usage = "Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)]\n";

int main(int argc , char ** argv)
{
	int fd_write;
	char* filename="fconc.out";
	if (argc < 3){
		perror(usage);
		exit(1);
	}
	if (argc > 3) {
		filename = argv[argc-1];
		argc--;
	}
	fd_write = open (filename , O_WRONLY | O_CREAT |O_TRUNC , S_IRUSR | S_IWUSR);
	if (fd_write == -1)
	{
		perror("open");
		return 1;
	}
	int i;
	for (i=1; i<argc; i++)
	{
		int fd_read;
		fd_read = open(argv[i], O_RDONLY);
		if (fd_read == -1)
		{
			perror("open");
			return 1;
		}
		char buff[BUF_SIZE];
		ssize_t nread;
		while(1) {
	         	nread = read(fd_read, buff, BUF_SIZE);
			buff[nread] = 0;
			ssize_t temp = 0;
			size_t written = 0;
			do {	
				temp = write(fd_write, buff + written, nread - written);
				written += temp;
			}while(written < nread);	
			
			//written = write(fd_write,buff,nread);	
		if (nread == -1 ) {
			perror("read");
			return 1;
		}
		if (nread == 0 ) break;
		}
		if (close(fd_read) < 0) {
			perror("close");
			return 1;
		}
		
	}
//	char *eof = EOF;
//	write(fd_write,eof,1);
	close (fd_write);
	return 0;
}
