#include "body.h"

int main(int argc, char** argv) {
	if (argc>1) {
		int stat;
		pid_t pid; 
		if ((pid=fork())==0) {
			parseFile(argv[1]);
		}
		else {
			waitpid(pid,&stat,0);
			printf("EXIT: %d\n",stat);
			if (stat == 0) {
				fileCopy("/tmp/copia.nb",argv[1]); 
			}
			else {
				if((pid = fork())==0) {
					char* re[3] = {"rm","/tmp/copia.nb",NULL}; 
					execvp("rm",re);						  				
				}
				else {
					waitpid(pid,&stat,0);
				}
			}
		}
	}
	return 0;
}