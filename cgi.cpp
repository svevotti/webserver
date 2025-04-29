#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>

int main( void )
{
	int fd[2];
	char fixed_str[] = "forgeeks.org";
    char buffer[100];

	if (pipe(fd) == -1) {
        fprintf(stderr, "Pipe Failed");
        return 1;
    }
	pid_t pid = fork();
	if ( pid == 0 ) //child
	{
		// pid_t child_pid = getpid();
		printf( "This is being printed from the child process %d\n", getpid());

		close(fd[0]); // Close reading
		dup2(fd[1], STDOUT_FILENO); //redirect stdout to the writing end
		close(fd[1]);
		const char *args[] = {"/usr/bin/python3", "script.py", NULL};
		if (execv(args[0], (char *const *)args) == -1) {
			perror("execv failed"); // Print error if execv fails
			return 1;
		}
	}
	else //parent
	{	
		pid_t parent_pid = getppid();
		printf( "This is being printed in the parent process:\n"
		        " - the process identifier (pid) of the child is %d for parent id %d\n", pid, parent_pid );
		char concat_str[100];
		close(fd[1]); // Close write parent
		ssize_t nbytes;
        while ((nbytes = read(fd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[nbytes] = '\0'; // Null-terminate the string
            printf("Parent received: %s", buffer);
        }
		
        close(fd[0]); // Close the read end
	}

	return 0;
}