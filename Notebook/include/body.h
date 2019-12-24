#ifndef _BODY_
#define _BODY_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>     /*chamadas ao sistema: defs e decls essenciais*/
#include <sys/wait.h>   /*chamadas wait*() e macros relacionadas*/
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>

/* Cat do ficheiro na diretoria tmp para a diretoria onde se encontra o nb, apaga a copia, 
   e faz o rename para o nome do nb */
int fileCopy (char* file, char* nd);
/* Percorre o ficheiro original, cria uma copia na diretoria tmp, e le do oiriginal linha a linha, 
   executando os comandos */
int parseFile(char* path);
/* Verificar se a linha em questao e um comando, se comeca por $ */
int iscommand(char* s);
/* Testa se um comando requer como argumentos o output do comando anterior, se e comando do tipo $| sort */
int cmdHasArgs(char* s);
/* Funcao de execucao de Comandos,recebe descritor de ficheiro, o comando (char* c), e o array de comandos ja guardados (char** cmds)  
	cmds = {"$ ls","$| sort","$| head -1"};
	cmds = {"$ ls","$| sort"};
	cmds = {"$ ls"};
 */
void execCommands(int f, char** args, char* c);
/* Funcao que le uma linha do ficheiro */
ssize_t readln(int fildes, void *buf, ssize_t nbyte);
/* Obter o comando, isto e recebe "$ ls" ou "$| head -1", retorna "ls" e "head -1" */
char* getCmd (char* cmd);
/* Obter o Comando anterior apartir dos comandos ja guardados no array de comandos ao Comando que necessite do seu output */
char* getPreviousCmd(char** args, char* c);
/* Forma os argumentos num array de strings, isto e recebe "$| head -1", retorna {"head","-1",NULL} */ 
char** formarrayofArgs(char* s);
/* Junta as instrucoes encadeadas num array de Strings, isto e recebe {"$ ls","$| sort"} e o comando "$| sort, 
   retorna {"ls","sort"}
*/ 
char** chainedInstructions (char** is, char* i);
/* Conta o numero de instrucoes de um array de Strings */ 
int instructionCount(char** args);
/* Conta o numero de comandos que constam no ficheiro */
int commandCount(char* path);
/* Signal */
void handler(int signo);

#endif 