#include <stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/time.h>

int main(){

	int i = 0;
	for(i = 0; i < 100; i++){
	
		struct timeval begin, end;

		gettimeofday(&begin, NULL);
		long startTime = begin.tv_usec;

		printf("Start time: %ld\n", startTime);	

		pid_t pid = fork();//spawn child

		if(pid == 0){ //child process
			static char *argv[] = {};
			pid_t pid = fork();
			execv("loop2", argv);
			exit(127);//should only exit if there was an error executing function
		}
		else{ //parent process
			waitpid(pid, 0, 0); //wait for child to exit
		}	
	}
	return 0;
}
