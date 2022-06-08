/* xargs.c
 * created by: Erkai Yu
 * Read lines from the standard input and run a command for each line, supplying the line as arguments to the command.  
 * Use fork and exec to invoke the command on each line of input. Use wait in the parent to wait for the child to complete the command. 
 * To read individual lines of input, read a character at a time until a newline ('\n') appears.
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[]){
	char* arg[MAXARG];
	if (argc <= 1) {
		fprintf(2, "usage: xargs [...]\n");
		exit(1);
	}
	char cmd[512];
	strcpy(cmd, argv[1]); // The command to run, maximum character length of this command is set to 512..
	for (int i=0; i<argc-1; i++){
		arg[i]=malloc(strlen(argv[i+1])*sizeof(char));
		strcpy(arg[i], argv[i+1]);
	}
	int narg = argc-1; 
	char buf;
	int w;
	char s[1024]; // Here assume that the maximum length of each argument is within 1024 characters.
	//strcpy(s, "");
	for (int i=0; i<1024; i++) s[i]='\0';
	while (read(0, &buf, 1)){ // Parsing and run. 
		if (buf == ' '){
			if (strlen(s)>0){
				if (narg == MAXARG) exit(1);
				arg[narg]=malloc(sizeof(char)*strlen(s));
				strcpy(arg[narg], s);
				narg++;
				//strcpy(s,"");
				for (int i=0; i<1024; i++) s[i]='\0';
			}
		}
		else if(buf == '\n'){
			if (strlen(s)>0){
				if (narg == MAXARG) exit(1);
				arg[narg]=malloc(sizeof(char)*strlen(s));
				strcpy(arg[narg], s);
				narg++;
				//strcpy(s,"");
				for (int i=0; i<1024; i++) s[i]='\0';
				if (fork() == 0){
					exec(cmd, arg);
				}
				else {
					wait(&w);
					if (w){
						exit(1);
					}
					for (int i = argc - 1; i < narg; i++)
						free(arg[i]);
					narg = argc-1;
				}
			}
			else {
				if (fork() == 0){
					exec(cmd, arg);
				}	
				else {
					wait(&w);
					if (w) {
						exit(1);
					}
					for (int i = argc - 1; i < narg; i++)
						free(arg[i]);
					narg = argc-1;
				}
			}
		}
		else {
			s[strlen(s)]=buf;
			s[strlen(s)+1]='\0';
		}
	}
	for (int i=0; i<argc-1; i++)
		free(arg[i]);
	exit(0);
}
