/* find.c
 * created by: Erkai Yu
 * A simple version of the UNIX find program: find all the files in a directory tree with a specific name. 
 * Use recursion to allow find to descend into sub-directories.
 * Should not recurse into "." and ".."
 * Use C strings.
 * Use strcmp() to compare strings.
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
filename(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return filename.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  buf[strlen(p)] = '\0';
  return buf;
}

void find(char *path, char *name){
	char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;

	if((fd = open(path, 0)) < 0){
    	fprintf(2, "find: cannot open %s\n", path);
    	return;
  	}

  	if(fstat(fd, &st) < 0){
    	fprintf(2, "find: cannot stat %s\n", path);
    	close(fd);
    	return;
  	}
	switch (st.type){
		case T_FILE:
			if (!strcmp(filename(path), name))
				printf("%s\n", path);
			break;
		case T_DIR:
			if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
				printf("find: path too long\n");
				break;
			}		
			strcpy(buf, path);
			p = buf+strlen(buf);
			*p++ = '/';
			while(read(fd, &de, sizeof(de)) == sizeof(de)){
				if (de.inum == 0)
					continue;
				memmove(p, de.name, DIRSIZ);
				p[DIRSIZ] = 0;
				if (stat(buf, &st) < 0){
					printf("find: cannot stat %s\n", buf);
					continue;
				}
				if (strcmp(filename(buf), ".")&&strcmp(filename(buf), ".."))
					find(buf, name);
			}
			break;
	}
	close(fd);

}

int main(int argc, char *argv[]){
	if (argc!=3) exit(1); // Here assume that the only valid way of usage is in form find xxx xxx.
	find(argv[1], argv[2]);	
	exit(0);
}
