/* primes.c
 * created by: Erkai Yu
 * Use pipe and fork to set up the pipeline. 
 * For each prime number, arrange to create one process that reads from its left neighbor over a pipe and writes to its right neighbor over another pipe.
 * Close file descriptors that a process doesn't need.
 * Once the first process reaches 35, it should wait until the entire pieline terminates, including all childrem, grandchildren, &c. Thus the main primes process should only exit after all the output has been printed, and after all the other primes processes have exited.
 * read returns zero when the write-side of a pipe is closed.
 * It's simplest to directly write 32-bit (4-byte) ints to the pipes, rather than using formatted ASCII I/O.
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
	int p[2]; 
	if (-1 == pipe(p)) exit(1);
	for (int i = 2; i <= 35; i++)
		write(p[1], &i, sizeof(int)); // Push 2 - 35 into the pipe. 
	int* message = malloc(sizeof(int));
	int l_fd = p[0]; 
	int r_fd;
	close(p[1]);
	while (fork() == 0){ 
		if (read(l_fd, message, sizeof(int))){
			printf("prime %d\n", *message);
			pipe(p);
			r_fd = p[1];
			int d = *message;
			while (read(l_fd, message, sizeof(int))){
				if ((*message) % d){
					write(r_fd, message, sizeof(int));
				}
			}
			close(r_fd);
			close(l_fd);
			l_fd = p[0];
		}
		else exit(0);
	}
	wait(0);
	free(message);
	exit(0);
}
