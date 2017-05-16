/*
Sam Cook
CIS 3207
Shell lab
3/7/17
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <termcap.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>


#define BUFFERSIZE 200

char buffer[BUFFERSIZE];
char cwd[BUFFERSIZE];
char *prompt;
char *a = ">";

int saved_stdout;
int saved_stdin;


//begin utility functions

struct info{
    char inName[BUFFERSIZE];
    char outName[BUFFERSIZE];

    int outFlag;
    int inFlag;
    int outAppFlag; //for the '>>' appending redirection

    int infd;
    int outfd;
    int pipes;

    int saved_stdout;
    int saved_stdin; 
};

char **tokenize(char *line, const char c){ //splits argument into an array and returns it

    int bufsize = BUFFERSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

     if (!tokens) {
       fprintf(stderr, "allocation error\n");
       exit(EXIT_FAILURE);
     }

    token = strtok(line, &c); //tokenizes using " " as a delimiter
    while (token != NULL) {
       tokens[position] = token;
       position++;

        if (position >= bufsize) { //realloc if needed
            bufsize += BUFFERSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));

            if (!tokens) {
                fprintf(stderr, "allocation error\n");
                exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, " ");
  }
  tokens[position] = NULL; //null terminates
  return tokens;
}

int hasPrefix(char const *p, char const *q) //determins "prefix" of argument, aka the command itself
{
    int i = 0;
    for(i = 0;q[i];i++)
    {
        if(p[i] != q[i])
            return -1;
    }
    return 0;
}

char lastChar(char *string){ //minus two here because when buffer is passed, it has a space or something

    char c = string[(strlen(string) - 2)]; //at the end, so this solves that. Aka 2nd to last char
    return c;
}

int parse (char *input, char c, struct info storage){
    int i;
    char p;
    int count = 0;
    for (i = 0; i < strlen(input); i++){
        p = input[i];
        if(p == c){
            input[i] = ' '; //set > or < to ' ' so it can be tokenize by tokenize function
            count++;
            if (p == c & input[i+1] == c)
                storage.outAppFlag = 1;
            
        }
    }
    return count; //should probably only use to parse for i/o redirection
}

//begin builtin commands

int echo(char **input){ //prints to stdout the tokenized input, ignoring the first elt

    int i = 1;          //which is "echo"
    while(input[i] != NULL){
        printf("%s", input[i]);
        printf(" ");
        i++;
    }
    return 0;
}

void help(){
    char *pass[] = {"more", "readme", 0}; //argv[], passes command name and the name of the readme file to print

    int pid = fork();
    if (pid < 0){
        fprintf(stderr, "fork error: %s\n", strerror(errno)); //forking error, print errno
    }
    else if (pid > 0){
        int status;
        waitpid(pid, &status, 0);
    }
    else if (pid == 0){ //child
        execvp("more", pass); //execs the more command, which lists the contents of the readme file
    }
}

void enterPause(){ //pauses the shell until the user presses the 'enter' key

    printf("Press enter to resume the shell when ready.\n");
    fflush(stdin);
    getchar();
}

int cd(char *pth){ //changes directory

    char path[BUFFERSIZE];
    strcpy(path,pth);

    char cwd[BUFFERSIZE];

    if(pth[0] != '/') //adds a '/' for the sys call
    {// true for the dir in cwd
        getcwd(cwd,sizeof(cwd));
        strcat(cwd,"/");
        strcat(cwd,path);
        int success=chdir(cwd); //makes sys call and sets it to T/F
        if(success!=0)
            printf("not found\n"); 
    }
    else{//true for dir w.r.t. /
        int success=chdir(pth);
	if(success!=0)
        printf("not found\n");
    }

    return 0;
}

void listenviron(char **envp){ //prints the environment variable

    int i = 0;
	/*printf("%d\n",argc);
	for(int i=0;i<argc;i++)
	{
		printf("%s\n",argv[i]);
	}*/
	i = 1;
  	char *s = *envp;
	while(s) //goes through and lists the details of the environment variable
	{
    		printf("%s\n", s);
    		s = *(envp+i);
            i++;
  	}
}

void pwd(){ //prints current working directory 
    char mycwd[BUFFERSIZE];
    getcwd(mycwd, sizeof(mycwd));
    printf("Current working directory: %s\n", mycwd);
}

void dir(){
    DIR *mydir;
    struct dirent *myfile;
    struct stat mystat;
    char buf[512];
    prompt=getcwd(cwd,sizeof(cwd));
    mydir = opendir(prompt);
    while((myfile = readdir(mydir)) != NULL)
    {
        sprintf(buf, "%s/%s", prompt, myfile->d_name);
        stat(buf, &mystat);
        printf("%lld",mystat.st_size);
        printf(" %s\n", myfile->d_name);
    }
    closedir(mydir);
}

void clr(){

    printf("\033[H\033[J"); //escape code to clear
}

void normalExec(char *buffer){
    char **arg = tokenize(buffer, ' ');
    //char **p = arg + 1; //creates an offset pointer, since arg[0] is the command name
    //I forget why I needed this but leaving it in case I come back in a little a remember

    if(lastChar(buffer) == '&'){ //should run program in the background
        int pid = fork();
        if(pid < 0)
            fprintf(stderr, "fork error: %s\n", strerror(errno)); //forking error, print errno
        else if( pid > 0){
            //parent but we're runnning in the background
        }
        else{
            execvp(arg[0], arg);
        }
    }

    else{ //business as usual, no background running
        int pid = fork();   
        if (pid < 0){
            fprintf(stderr, "fork error: %s\n", strerror(errno)); //forking error, print errno
        }
        else if (pid > 0){
            int status;
            waitpid(pid, &status, 0);
        }
        else{ //child
            execvp(arg[0], arg); //execs the given command
        }
    }
}

void pipehandler(){ //to create a pipe
    int pipefd[2];
    pipe(pipefd);

    int pid = fork();
    if(pid == 0){
        dup2(pipefd[0], 0);
        close(pipefd[1]);

    }
}

void rd(char ** input, struct info storage){

    printf(input[1]);

    if(storage.inFlag){  // '<'
        storage.saved_stdin = dup(STDIN_FILENO); //to switch stdin back later
        storage.infd = open(input[1], O_RDONLY, 0666); 
        dup2(storage.infd, STDIN_FILENO);
        close(storage.infd);
    }

    if(storage.outFlag){ // '>'
        /*storage.*/saved_stdout = dup(STDOUT_FILENO); //to switch stdout back later
        printf("rd saved stdout: %d\n", /*storage.*/saved_stdout);
        storage.outfd = open(input[1], O_CREAT|O_WRONLY| O_TRUNC, 0666);
        dup2(storage.outfd, STDOUT_FILENO);
        close(storage.outfd);
    }
    else if(storage.outAppFlag){ //>>
        /*storage.*/saved_stdout = dup(STDOUT_FILENO); //to switch stdout back later
        storage.outfd = open(input[1], O_CREAT|O_APPEND|O_WRONLY, 0666);
        dup2(storage.outfd, STDOUT_FILENO);
        close(storage.outfd);
    }
}

void rdclose(struct info storage){
    if(storage.inFlag){
        printf("rdclose saved stdin: %d\n", storage.saved_stdin);
        dup2(storage.saved_stdin, STDIN_FILENO);
        close(storage.saved_stdin);
    }
    if(storage.outFlag){
        printf("rdclose saved stdout: %d\n", /*storage.*/saved_stdout);
        dup2(/*storage.*/saved_stdout, STDOUT_FILENO);
        close(/*storage.*/saved_stdout);
    }
    else if(storage.outAppFlag){
        dup2(storage.saved_stdout, STDOUT_FILENO);
        close(storage.saved_stdout);
    }
}

int main ( int argc, char **argv,char ** envp ) {

    FILE *fp;
    if (argc == 2){ //command line input from a file
        fp = fopen(argv[1], "r");
    }

    char *tok;
    tok = strtok (buffer," ");

    struct info myinfo;
    myinfo.inFlag = 0;
    myinfo.outFlag = 0;
    myinfo.outAppFlag = 0;
    myinfo.pipes = 0;

    while(1)
    {
        myinfo.inFlag = 0;
        myinfo.outFlag = 0;
        myinfo.outAppFlag = 0;
        myinfo.pipes = 0;

        if(argc == 1){ //no command line argument
            bzero(buffer, BUFFERSIZE);
            prompt=getcwd(cwd,sizeof(cwd));
            printf("%s%s",prompt,a);
            fgets(buffer, BUFFERSIZE, stdin);
        }
        else if (argc ==2) {//command line arg exists
            if(fgets(buffer, BUFFERSIZE, fp) == NULL) 
                break;
            else{
                //do nothing because f was already got
            }
            
        }
       
//parse for i/o redirection
        if (parse(buffer, '>', myinfo)) 
            myinfo.outFlag = 1; 
        if(parse(buffer, '<', myinfo))
            myinfo.inFlag = 1; 
        if(parse(buffer, '|', myinfo))
            myinfo.pipes = 1;

//call function to change input/output
        if(myinfo.inFlag || myinfo.outFlag)
            rd(tokenize(buffer, ' '), myinfo);


//begin function calls
        if(hasPrefix(buffer,"cd") == 0)
        {
            tok = strchr(buffer,' '); //use something more powerful
            if(tok) {
                char *tempTok = tok + 1;
                tok = tempTok;
                char *locationOfNewLine = strchr(tok, '\n');
                if(locationOfNewLine) {
                    *locationOfNewLine = '\0';
                }
                cd(tok);
            }
            else{ //no argument, prints current directory
                pwd();
            }
        }
        else if(hasPrefix(buffer,"dir") == 0){ //rest of builtins follow
            dir();
        }
        else if(hasPrefix(buffer,"environ") == 0){
            listenviron(envp);
        }
        else if(hasPrefix(buffer, "echo") == 0){
            echo(tokenize(buffer,  ' '));
        }
        else if(hasPrefix(buffer, "clr") == 0){
            clr();
        }
        else if(hasPrefix(buffer, "pause") == 0){
            enterPause();
        }
        else if(hasPrefix(buffer, "help") == 0){
            help();
        }
        else if (hasPrefix(buffer, "pwd") == 0){
            pwd();
        }
        else if(hasPrefix(buffer,"quit") == 0){
            break; //exits from loop and from shell
        }
        else{//means that we're not a builtin, should fork & exec
            normalExec(buffer);
        }

    
        if(myinfo.inFlag || myinfo.outFlag)
            rdclose(myinfo);

    }

    return 0; // Indicates that everything went well.
}



