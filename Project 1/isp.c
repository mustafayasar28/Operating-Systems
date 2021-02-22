#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_COMMAND_SIZE 200
#define MAX_NUMBER_OF_PARAMS 10
#define MAX_PARAM_LENGTH 20

#define READ 0
#define WRITE 1

static char *duplicateStr( char* str ) {
     char* otherStr = malloc( strlen(str) + 1 );
     if ( otherStr != NULL ) {
         strcpy(otherStr, str);
     }
     return otherStr;
}

int main(int argc, char *argv[])
{
     /* byteCount is the number of bytes to read/write in one system
call */

     const int byteCount = atoi(argv[1]);

     /*
         If mode == 1, normal communication mode is used
         IF mode == 2, tapped communication mode is used
     */

     const int mode = atoi(argv[2]);


     pid_t parentId = getpid();

     while ( getpid() == parentId ) {
         char *paramv[200];
         int paramc = 0;
         /* Get the command input from the user*/
         char str[MAX_COMMAND_SIZE];


         printf("isp$ ");
         fgets(str, MAX_COMMAND_SIZE, stdin);

         if ( (strlen(str) > 0) && (str[ strlen(str) - 1] == '\n') ) {
             str[ strlen(str) - 1] = '\0';
         }

         /* str is the command to be executed. */

         /* divide the command into parameters. */
         char* params = strtok(str, " ");
         while ( params != NULL ) {
             paramv[ paramc++ ] = duplicateStr(params);
             params = strtok(NULL, " ");
         }

         paramv[paramc] = NULL;

         /* Every parameter can be accessed by params[i], where
             min i = 0, max i = paramc

             I.e. number of parameters is counter.

             paramv[0] = command name
             paramv[1] = first parameter (if any)
             paramv[2] = second parameter (if any)
             ...
             paramv[ paramc ] = NULL;
         */


         /* Create the child process. The command will be executed
             by this child process.
         */

         /* Check if the parameters contain | character */
         /* Search paramv until paramc */
         char* search = "|";
         bool found = false;
         int i = 0;
         while( paramv[i] ) {
             if (strcmp(paramv[i], search) == 0) {
                 found = true;
                 break;
             }
             i++;
         }

         /* found = true only if the parameters contain | */
         /* found = false, otherwise */


         /* If !found, create one child,
             else, create two child.
         */

         if ( !found ) {
             pid_t pid;

             pid = fork();    /* for the child process */

             if ( pid < 0 ) {
                 fprintf(stderr, "Fork failed");
                 return 1;
             } else if ( pid == 0 ) {    /* child process executes here */
                 char path[] = "/bin/";
                 strcat(path, paramv[0]);
                 int validation = execv(path, paramv);

                 if ( validation == -1 ) {
                     fprintf(stderr, "Invalid input\n");
                 }

                 for (int i = 0; i < paramc; i++){
                     free( paramv[paramc] );
                 }
             } else {    /* parent process executes here */
                 wait(NULL); /* Wait for the child process to complete */
             }
         } else {
             pid_t leftChild = 1;
             pid_t rightChild = 1;
             pid_t wpid;
             int status = 0;
             
             if (mode == 1) { /* We are in the normal mode */ 
				 	/* Create the pipe here */
				 int fd[2]; /* File Descriptors, fd[READ] for read, fd[WRITE] for write */
				 
				 pipe(fd); /* Pipe is created */


				 leftChild = fork();

				 if ( leftChild != 0 ) {
				     rightChild = fork();
				 }
				 
				 if ( leftChild == 0 && rightChild != 0 ) { /* Left Child executes here */
		             char* search = "|";
		             int i = 0;
		             int index;
		             while( paramv[i] ) {
		                 if (strcmp(paramv[i], search) == 0) {
		                     index = i;
		                     break;
		                 }
		                 i++;
		             }

		             char* newParams[ 10 ];

		             for (int i = 0; i < index; i++) {
		                 newParams[i] = duplicateStr(paramv[i]);
		             }

		             newParams[index] = NULL;

		             char path[] = "/bin/";
		             strcat(path, paramv[0]);

		             dup2(fd[WRITE], WRITE); /* Duplicate the write end */

		             close(fd[READ]); /* Close the unused read end */
		             
		             close(fd[WRITE]); /* Close file descriptor since it is a copy */

		             execv(path, newParams);

		             return -1;
             	}
             	
             	if ( leftChild != 0 && rightChild == 0 ) { /* Right Child executes here */

		             /* Find index of '|' and create new array containing elements after '|' */
		             char* search = "|";
		             int i = 0;
		             int index;
		             while( paramv[i] ) {
		                 if (strcmp(paramv[i], search) == 0) {
		                     index = i;
		                     break;
		                 }
		                 i++;
		             }

		             char* newParams[ 10 ];
		             for ( int i = 0; i < paramc - index - 1; i++ ) {
		                 newParams[i] = duplicateStr( paramv[index + 1 + i]);
		             }

		             newParams[ paramc - index - 1] = NULL;

		             char path[] = "/bin/";
		             strcat(path, newParams[0]);

		             dup2(fd[READ], READ);
		             close(fd[WRITE]);
		             close(fd[READ]);

		             execv(path, newParams);
		             return -1;
		         }
		         
		         close(fd[READ]);
             	 close(fd[WRITE]);

             	 while((wpid = wait(&status)) > 0); /* Parent waits here for all its children to terminate */
             } else if ( mode == 2 ) {
             
             	leftChild = fork();

				 if ( leftChild != 0 ) {
				     rightChild = fork();
				 }
				 
				 if ( leftChild == 0 && rightChild != 0 ) { /* Left Child executes here */
		             
		             return -1;
             	}
             	
             	if ( leftChild != 0 && rightChild == 0 ) { /* Right Child executes here */

		             return -1;
		        }
             }
         }
         for ( int i = 0; i < paramc; i++ )
             free(paramv[ i ]);
     }
     return 0;
}
