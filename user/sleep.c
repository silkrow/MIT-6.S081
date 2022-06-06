/* sleep.c
 * created by: Erkai Yu
 * Pause for a user-specified number of ticks.
 * If user forgets to pass an argument, sleep should print an error message.
 * The command-line argument is passed as a string, shall be converted with atoi(see user/ulib.c).
 * Use the system call sleep.
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
	if (argc == 1) { // User forgets to pass an argument. Print an error message.
		char *message = "Error: sleep, no argument received.\n";
		write(1, message, strlen(message));
		exit(1);
	}
	sleep(atoi(argv[1]));
	exit(0);
}
