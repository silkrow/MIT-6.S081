/* pingpong.c
 * created by: Erkai Yu
 * "ping-pong" a byte between two processes over a pair of pipes, one for each direction. 
 * The parent sends a byte to the child; the child prints "<pid>: received ping", writes the byte on the pipe to parent, and exit; the parent should read the byte and print "<pid>: received pong", and exit.
 * Use pipe, fork, read, write, getpid.
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
	int p1[2];
	int p2[2];
	if (-1 == pipe(p1) || -1 == pipe(p2)) exit(1);
	char* message = malloc(1); // The one byte message for pingpong.

	if (fork() == 0){
		read(p1[0], message, 1);
		fprintf(1, "%d: received ping\n", getpid());
		write(p2[1], message, 1);
		free(message);
		exit(0);
	}
	else{
		*message = 'a';
		write(p1[1], message, 1);
		read(p2[0], message, 1);
		wait(0);
		fprintf(1,"%d: received pong\n", getpid());
		free(message);
	}

	exit(0);
}
