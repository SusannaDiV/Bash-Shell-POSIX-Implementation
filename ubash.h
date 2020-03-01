#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#define BUFMAXSIZE 1024

int openInputFile();
int openOutputFile();
int seeIfPipe(char* comm);
int withoutPipeExecute(char** commTokens);
int parseCommandLine(char* com_line, char** comds);
void prompt();
void parsePipe(char* comm);
void withPipeExecute(char* comm);
void simpleCommandExecute(char** commTokens);
void parseCommand(char* comm, char** commTokens);
void changeDir(char** commTokens, char* curr_dir);
void parseRedirection(char* comm, char** commTokens);

int n_pipe, red_input, red_output;
char curr_dir[BUFMAXSIZE];
char directory[BUFMAXSIZE];
char* com_pipe[BUFMAXSIZE];
char* infile;
char* outfile;