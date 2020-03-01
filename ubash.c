#include "ubash.h"

int withoutPipeExecute(char** commTokens) {
    pid_t pid;
    pid = fork();
    if(pid < 0) { 
        perror("Processo figlio non creato\n");
        return -1; 
    }
    else if(pid==0) { 
        int fdin, fdout;
        if(red_input) {
            fdin = openInputFile();
            if(fdin == -1)  exit(-1);
        }
        if(red_output) {
            fdout = openOutputFile();
            if(fdout == -1) exit(-1); 
        }
        if((execvp(commTokens[0], commTokens)) < 0) {
            perror("Errore esecuzione comando\n");
            exit(-1);
        }
        exit(0);
    }
    int status;
    waitpid(pid, &status, WUNTRACED);               
    if(!WIFEXITED(status))        printf("Il figlio non e' uscito normalmente, ha exit status: %d\n", WEXITSTATUS(status));
    return 0;
}

void simpleCommandExecute(char** commTokens) {
    for(int i = 0; commTokens[i] != NULL; ++i) {
        char *chr = commTokens[i];
        if(*chr == '$') {
            if (getenv(chr + 1) != NULL)      commTokens[i] = getenv(chr + 1); 
            else                              printf("Non esiste tale variabile d'ambiente\n"); 
        }
    }
    if(strcmp(commTokens[0], "cd\0") == 0)    changeDir(commTokens, curr_dir);
    else if((strcmp(commTokens[0], "quit\0") == 0) || (strcmp(commTokens[0], "exit\0") == 0))   exit(0); 
    else                                      withoutPipeExecute(commTokens);
    free(commTokens);
}

int parse_cmd_line(char* com_line, char** comds) {
    int n_com = 0;
    char* token = strtok(com_line, "\n");
    while(token!=NULL) {
        comds[n_com++] = token;
        token = strtok(NULL, "\n");
    }
    return n_com;
}

void parseCommand(char* comm, char** commTokens) {
    int tok = 0;
    char* token = strtok_r(comm, " ", &comm);
    while(token!=NULL) {
        commTokens[tok++] = token;
        token = strtok_r(NULL, " ", &comm);
    }
}

void parse_for_piping(char* comm) {
    int tok = 0;
    char* token;
    char* copia = strdup(comm);
    token = strtok_r(copia, "|", &copia);
    while(token!= NULL) {
        com_pipe[tok++] = token;
        token = strtok_r(NULL, "|", &copia);
    }
    n_pipe = tok;
}

void withPipeExecute(char* comm) {
    int pid, fdin, fdout, status, i, j; 
    n_pipe = 0, pid = 0;
    parse_for_piping(comm);
    int* pipes = (int* )malloc(sizeof(int)*(2*(n_pipe - 1)));
    for(i = 0; i<2*n_pipe-3; i+=2) {
        if(pipe(pipes + i) < 0 ) {             
            perror("Errore apertura pipe\n");
            return;
        }
    }
    for(i = 0; i < n_pipe; ++i) {
        char** commTokens = malloc((sizeof(char)*BUFMAXSIZE)*BUFMAXSIZE);
        parseRedirection(strdup(com_pipe[i]), commTokens);
        pid = fork();
        if(pid < 0)        perror("Errore fork\n");
        else if(pid == 0) {
            if(red_output) { 
                fdout = openOutputFile(); 
                if(fdout == -1)               exit(-1); 
            }
            else if(i < n_pipe - 1)           dup2(pipes[2*i + 1], 1);
            if(red_input) { 
                fdin = openInputFile(); 
                if(fdin == -1)                exit(-1); 
            }
            else if(i > 0)                    dup2(pipes[2*i - 2], 0);
            for(j = 0; j<2*n_pipe-2; ++j)     close(pipes[j]);
            if(execvp(commTokens[0], commTokens) < 0) {
                perror("execvp - Parsing error (cd non puo' essere usato con altri comandi)\n");
                exit(-1);
            }
        }
    }
    for(i = 0; i < 2*n_pipe - 2; ++i)         close(pipes[i]);
    for(i = 0; i < n_pipe ; ++i) {
        waitpid(pid, &status, WUNTRACED);
        if(!WIFEXITED(status))                printf("Il figlio non e' uscito normalmente, ha exit status: %d\n", WEXITSTATUS(status));
    }        
}

int seeIfPipe(char* comm) {
    red_input = red_output = 0; 
    int pipe_present = 0, foundi = 0, foundo = 0;
    for(int i = 0 ; comm[i] ; i++) {
        if(comm[i] == '|' && comm[i+1] == ' ' && comm[i+2] == '|') {
            printf("Parsing error: comando vuoto\n"); 
            return 2;
        }
        if((comm[i] == '<' || comm[i] == '>')&& comm[i+1] == ' ') { 
            printf("Parsing error: non e' specificato il file per la redirezione dell'I/O\n");
            return 2;
        }
        if(comm[i] == '|')    pipe_present = 1;
        if(comm[i] == '<') { 
            ++foundi; 
            red_input = 1; 
        }
        if(comm[i] == '>') { 
            ++foundo; 
            red_output = 1; 
        }
    }
    if (foundi>1 || foundo>1) {
        printf("Parsing error: troppe redirezioni nel comando\n");
        return 2;
    }
    if(pipe_present)         return 1;
    else                     return -1;
}

void parseRedirection(char* comm, char** commTokens) {
    red_input = red_output = 0;
    infile = outfile = NULL;
    int i, tok = 0;
    char* token;
    char* copia = strdup(comm);
    for(i = 0 ; comm[i] ; i++) {
        if(comm[i] == '<')       red_input = 1;
        if(comm[i] == '>')       red_output = 1;
    }
    if(red_input == 1) {
        char** input_redi_cmd = malloc((sizeof(char)*BUFMAXSIZE)*BUFMAXSIZE);
        token = strtok_r(copia, "<", &copia);
        while(token!=NULL) {
            input_redi_cmd[tok++] = token;
            token = strtok_r(NULL, "<", &copia);
        }
        copia = strdup(input_redi_cmd[tok - 1]);
        token = strtok_r(copia, "< |\n", &copia);
        infile = strdup(token);
        parseCommand(input_redi_cmd[0], commTokens);
        free(input_redi_cmd);    
    }
    if(red_output == 1) {
        char** output_redi_cmd = malloc((sizeof(char)*BUFMAXSIZE)*BUFMAXSIZE);
        token = strtok_r(copia, ">", &copia);
        while(token!=NULL) {
            output_redi_cmd[tok++] = token;
            token = strtok_r(NULL, ">", &copia);
        }
        copia = strdup(output_redi_cmd[tok - 1]);
        token = strtok_r(copia, "> |\n", &copia);
        outfile = strdup(token);
        parseCommand(output_redi_cmd[0], commTokens);
        free(output_redi_cmd);    
    }
    if(red_input == 0 && red_output == 0 )    parseCommand(strdup(comm), commTokens);
}


void changeDir(char** commTokens, char* curr_dir) {
    if(commTokens[1] == NULL || strcmp(commTokens[1], "~\0") == 0 || strcmp(commTokens[1], "~/\0") == 0) {
        if (chdir("/home")!=0)                  perror("Errore chdir\n");
    }
    else if (commTokens[2]!=NULL)               printf("Parsing error: troppi argomenti\n");
    else if(chdir(commTokens[1]) == 0) {     
        if (getcwd(curr_dir, BUFMAXSIZE)==NULL) perror("Errore getcwd\n"); 
    }
    else                                        perror("Error executing cd command");  
}

int openInputFile() {
    int fd = open(infile, O_RDONLY, S_IRWXU);
    if (fd < 0)      perror(infile);
    dup2(fd, STDIN_FILENO);
    close(fd);
    return fd;
}

int openOutputFile() {
    int fd = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
    if(fd < 0)       perror(outfile);
    dup2(fd, STDOUT_FILENO);
    fflush(stdout);
    close(fd);
    return fd;
}

void prompt() {
    if (getcwd(directory, BUFMAXSIZE-1)==NULL)   printf("Errore getcwd\n"); 
    strcpy(curr_dir, directory);
    printf("%s$ ", curr_dir);
}

int main() {
    while(1) {
        prompt();
        char** comds = malloc((sizeof(char)*BUFMAXSIZE)*BUFMAXSIZE); 
        for(int j = 0; j < BUFMAXSIZE; ++j)         comds[j] = '\0';
        char* comm = malloc(sizeof(char)*BUFMAXSIZE);
        if (fgets(comm, sizeof(char)*BUFMAXSIZE, stdin)==NULL) printf("Errore fgets\n");
        int n_com = parse_cmd_line(comm, comds); 
        for(int i = 0; i < n_com; ++i) {
            n_pipe = 0;
            infile = outfile = NULL;
            char** commTokens = malloc((sizeof(char)*BUFMAXSIZE)*BUFMAXSIZE);
            for(int j = 0; j < BUFMAXSIZE; ++j)     commTokens[j] = '\0';
            if (seeIfPipe(strdup(comds[i])) == 2)   continue;
            else if(seeIfPipe(strdup(comds[i])) == -1) {
                if(red_input == 0 || red_output == 0) {
                    parseCommand(strdup(comds[i]), commTokens);
                    simpleCommandExecute(commTokens);
                }
                else { 
                    parseRedirection(strdup(comds[i]), commTokens); 
                    simpleCommandExecute(commTokens); 
                }
            }
            else    withPipeExecute(comds[i]);
        }
        if(comds)    free(comds);
        if(comm)     free(comm);
    }
    return 0;
}