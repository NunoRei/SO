#include "body.h"

/* Variavel global, Numero de comandos que constam no ficheiro*/
int Ncomandos;
int dest;

void handler(int signo) {
	close(dest);
	unlink("copia.nb");
	exit(1);
}

/* Cat do ficheiro na diretoria tmp para a diretoria onde se encontra o nb, apaga a copia, 
   e faz o rename para o nome do nb */
int fileCopy (char* file, char* nd) {
	dest = open("copia.nb",O_CREAT|O_RDWR,0666);
	signal(SIGINT,handler);
	if(dest) {
		int stat;
		pid_t pid;
			if ((pid=fork()) == 0) {
				char* cmds[3] = {"cat",file,0}; 
				close(1);
				dup(dest);
				execvp("cat",cmds);
			}
			else {
				waitpid(pid,&stat,0);
				if (stat!=0) exit(EXIT_FAILURE);
			}
		    if ((pid=fork())==0) {
		    	char* re[3] = {"rm",file,NULL}; 
				execvp("rm",re);
			}
			else {
				waitpid(pid,&stat,0);
				if (stat!=0) exit(EXIT_FAILURE);
			}
			if ((pid=fork())==0) {
		    	char* re[4] = {"mv","copia.nb",nd,NULL}; 
				execvp("mv",re);
			}
			else {
				waitpid(pid,&stat,0);
				if (stat!=0) exit(EXIT_FAILURE);
			}
	}
	else {
		exit(EXIT_FAILURE);
	}

	close(dest);
	return 0;
}

/* Percorre o ficheiro original, cria uma copia na diretoria tmp, e le do oiriginal linha a linha, 
   executando os comandos */
int parseFile(char* path) {
	
	Ncomandos = commandCount(path);

	char* cmds[Ncomandos]; 
	ssize_t file = open(path,O_RDWR,0666);
	ssize_t copia = open("/tmp/copia.nb",O_CREAT|O_RDWR,0666);

	if (copia && file) {
		 char buf[1024];
		 int i=0;
		 int k=Ncomandos; 
		 ssize_t n;
		 while ((n=readln(file,buf,sizeof(buf)))>0 && k>0) { 
		 	if (iscommand(buf) == 0) {  
		 		char* arg = malloc(sizeof(buf));  
		 		strcpy(arg,buf);				
		 		int l=0;
		 		for(l=0; arg[l] != '\n' && arg[l] != '\0';l++);  
		 		arg[l] = '\0';           
		 		cmds[i] = malloc(sizeof(strlen(arg)));  
		 		cmds[i] = arg;  			   
		 		write(copia,buf,n);			   
		 		write(copia,">>>\n",4);		   
		 		execCommands(copia,cmds,arg); 
		 		write(copia,"<<<\n",4);   	  
		 		i++;
		 		k--;
		 	}

		 	else {
		 		if (k>0) { 
		 			if (!strcmp(buf,">>>\n")) {
		 				n = readln(file,buf,sizeof(buf));
		 				while (strcmp(buf,"<<<\n")) {
		 					n = readln(file,buf,sizeof(buf));
		 				}	
		 			}
		 			else {
		 				write(copia,buf,n);
		 			}	
		 		}
		 	}
		 }
	}

	else {
		exit(EXIT_FAILURE);
	}

	close(file);
	close(copia);
    for (int j=0; j<Ncomandos && cmds[j] != NULL;j++) { 
    	free(cmds[j]);
    }

	return 0;
}

/* Verificar se a linha em questao e um comando, se comeca por $ */
int iscommand(char* s) {
	int i = 0;
	int r = 0;
	if (s[i] != '$') r = -1;
	return r;
}

/* Testa se um comando requer como argumentos o output do comando anterior, se e comando do tipo $| sort */
int cmdHasargs(char* s) {
	int i = 0;
	int r = 0;
	while(s[i] != '\0') {
		if (s[i] == '|') r = -1;
		i++;
	}
	return r;
}

/* Funcao de execucao de Comandos,recebe descritor de ficheiro, o comando (char* c), e o array de comandos ja guardados (char** cmds)  
	cmds = {"$ ls","$| sort","$| head -1"};
	cmds = {"$ ls","$| sort"};
	cmds = {"$ ls"};
 */

void execCommands(int f, char** cmds, char* c) {

	char** argv = chainedInstructions(cmds,c);
	int npipes = (instructionCount(argv) - 1); 
	
	if (npipes > 0) { // Trata-se de uma instrucao de comandos encadeados
		pid_t pid;
		int status;
		int pipearray[2*npipes];
		for (int i = 0; i<npipes; i++) {
			if (pipe(pipearray + (i*2)) < 0) {
				exit(EXIT_FAILURE);
			}
		}
		int j = 0;
		int v = 0;
		while(argv[v] != NULL) {
			char** cmdsv = formarrayofArgs(argv[v]); 
			pid = fork();
			if (pid == 0) {
				// Nao e o primeiro comando a ser executado
				if(v!=0) {
					if (dup2(pipearray[j-2],0) < 0) {
						perror("Erro no dup2");
						exit(EXIT_FAILURE);
					}
				}
				// Nao e o ultimo a ser executado
				if (argv[v+1] != NULL) {
					if (dup2(pipearray[j+1], 1) < 0) {
						perror("Erro no dup2");
						exit(EXIT_FAILURE);
					}
				}
				// Se for o ultimo a ser executado, direciona para o ficheiro
				if (argv[v+1] == NULL) {
					if (dup2(f,1) < 0) {
						perror("Erro no dup2");
						exit(EXIT_FAILURE);
					}
				}

				for (int i=0; i < 2*npipes;i++) {
					close(pipearray[i]);
				}

				execvp(cmdsv[0],cmdsv);
				_exit(1);

			}
			else if (pid < 0) {
				exit(EXIT_FAILURE);
			}

			for (int j=0; j<Ncomandos && cmdsv[j] != NULL;j++) { 
    			free(cmdsv[j]);
    		}
			free(cmdsv);

			v++; 
			j+=2; 
		}

		for (int i = 0; i< 2 * npipes; i++) { 
			close(pipearray[i]);
		}
		
		for(int i = 0; i < (2*npipes) + 1 ; i++) { 
        	waitpid(pid,&status,0);               
        	if (status!=0) _exit(1);
  		}

    }
    else { // Comando unico
		char** a = formarrayofArgs(c);
		int pid;
		if ((pid = fork()) == 0) {	
			close(1);
			dup2(f,1);
			execvp(a[0],a);
			_exit(1);
		}
		else {
			int stat;
			waitpid(pid,&stat,0); 
			for (int j=0; j<Ncomandos && a[j] != NULL;j++) { 
    			free(a[j]);
    		}
    		free(a);
		}
	}
	free(argv);
}

/* Funcao que le uma linha do ficheiro */
ssize_t readln(int fildes, void *buf, ssize_t nbyte) {
	char *p = buf;
	ssize_t nblidos = 0;
	while (nblidos<nbyte) {
		if(nblidos == nbyte)break;
		ssize_t n = read(fildes,p,1);
		if (n<=0) break;
		nblidos+=n;
		if(*p=='\n') break;
		p++;
	}
	if (*p == '\0') {
		*p = '\n';
	}
	p++;
	*p = '\0';
	return nblidos;
}

/* Obter o comando, isto e recebe "$ ls" ou "$| head -1", retorna "ls" e "head -1" */
char* getCmd (char* c) {
	char* cmd = malloc(sizeof(strlen(c)));
	strcpy(cmd,c);
	while (*cmd != ' ') {
		cmd++;
	}
	cmd++;
	return cmd;
}

/* Obter o Comando anterior apartir dos comandos ja guardados no array de comandos ao Comando que necessite do seu output */
char* getPreviousCmd(char** cmds, char* c) {
	int i = 0;
	while((strcmp(cmds[i],c))!=0) i++;
	if (i>0) return cmds[i-1];
	else return cmds[i];
}

/* Forma os argumentos num array de strings, isto e recebe "$| head -1", retorna {"head","-1",NULL} */ 
char** formarrayofArgs(char* s) {
	
	int n = 0;
	int i = 0;
	char** args = malloc(Ncomandos*sizeof(strlen(s)+1));

	while (s[i] != '\0' && s[i] != '\n') {
		if (s[i] != '$' && s[i] != '|' && s[i] != ' ') {
			char* arg = malloc(sizeof(strlen(s)));
			int j=0;
			while (s[i] != ' ' && s[i] != '\n' && s[i] != '\0') {
				arg[j] = s[i];
				i++;
				j++;
			}
			arg[j] = '\0';
			args[n] = malloc(sizeof(strlen(arg)+1));
			args[n] = arg;
			n++;
		}
		else {
			i++;
		}
	}
	args[n] = NULL;
	return args;
}

/* Junta as instrucoes encadeadas num array de Strings, isto e recebe {"$ ls","$| sort"} e o comando "$| sort, 
   retorna {"ls","sort"}
*/ 
char** chainedInstructions (char** cmds, char* cmd) {

	char** out = malloc(Ncomandos*sizeof(strlen(cmd)+1));
	char** outreversed = malloc(Ncomandos*sizeof(strlen(cmd)+1));
	int j = 0;
	char* aux = malloc(Ncomandos*sizeof(strlen(cmd)+1));
	strcpy(aux,cmd);
	while (cmdHasargs(aux) == -1) {
		out[j] = getCmd(aux);
		strcpy(aux,getPreviousCmd(cmds,aux));
		j++;
	}
	strcpy(aux,getCmd(aux));
	out[j] = malloc(sizeof(strlen(aux)));
	strcpy(out[j],aux);
	int k = 0;
	while (j>=0) {
		outreversed[k] = out[j];
		k++;
		j--;
	}
	outreversed[k] = NULL;
	return outreversed;
}

/* Conta o numero de instrucoes de um array de Strings */ 
int instructionCount(char** cmds) {
	int i = 0;
	while(cmds[i] != NULL) i++;
	return i;
}

/* Conta o numero de comandos que constam no ficheiro */ 
int commandCount(char* path) {
	ssize_t file = open(path,O_RDWR,0666);
	ssize_t n;
	int count=0;
	char buf[1024];
	while((n=read(file,buf,1024))>0) {
		int i=0;
		while (buf[i] != '\0') {
			if (buf[i] == '$') count++;
			i++;
		}
	}
	close(file);
	return count;
}
