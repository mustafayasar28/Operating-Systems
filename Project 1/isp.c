#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
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
     	 
         char *paramv[MAX_COMMAND_SIZE];
         int paramc = 0;
         /* Get the command input from the user*/
         char str[MAX_COMMAND_SIZE];


         printf("isp$ ");
         fgets(str, MAX_COMMAND_SIZE, stdin);
		 struct timeval start, end;
     	 gettimeofday(&start, NULL);
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

         /* Every parameter can be accessed by paramv[i], where
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
		             
					
					if ( strcmp( newParams[0], "producer" ) == 0 ) {
             		 	char* args[] = {"producer", newParams[1], NULL};
             		 	execv( "producer", args);
             		 } else {
		         		 execv(path, newParams);
             		 } /* Execute the command */
					
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
						
		             if ( strcmp( newParams[0], "consumer" ) == 0 ) {
             		 	char* args[] = {"consumer", newParams[1], NULL};
             		 	execv( "consumer", args);
             		 } else {
		         		 execv(path, newParams);
             		 } /* Execute the command */
		             return -1;
		         }
		         
		         close(fd[READ]);
             	 close(fd[WRITE]);

             	 while((wpid = wait(&status)) > 0); /* Parent waits here for all its children to terminate */
             	 
             	 
             } else if ( mode == 2 ) {
             	 int numberOfBytesTransferred = 0;
             	 int writeReadCount = 0;
                 int leftPipe[2];
				 int rightPipe[2]; 
				 
				 pipe(leftPipe); /* Output of leftChild will go to this pipe */
				 pipe(rightPipe); /* Input of rightChild will be taken from this pipe */
				       	 
             	 leftChild = fork();
             	 
             	 if ( leftChild != 0 && rightChild != 0 ) {
             	 	/* Only the main (parent) process executes here */
             	
             	 	/* First read the output that was written by the left Child from the leftPipe */
					
					close(leftPipe[WRITE]); /* Main process does not write to the leftPipe */
					
					char reading_buf[ 100000 ];
					
					/* Read and re-direct the input */
					
					while ( read( leftPipe[READ], reading_buf, byteCount ) != 0 ) {
						numberOfBytesTransferred += byteCount;
						writeReadCount++;
						write( rightPipe[WRITE], reading_buf, byteCount );
					}
					
					close( rightPipe[WRITE] ); /* Writing finished, close it so that the right child can read.*/
             	 }
       
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
		            
		            
		            /* Execute the first part of the command, and sent the 
		            	output to the main process via leftPipe
		            */
		            
		            
		            /* Duplicate leftPipe to the terminal. Any writes to standart output
		            	will in fact to be sent to the leftPipe.
		            	
		            	WHenever left child (this process) writes data to standart output,
		            	the data will go to the leftPipe through the write-end descriptor.
		            	
		            */
		            dup2(leftPipe[WRITE], WRITE); 
		            
		            close(leftPipe[READ]); /* Close the read end */
		            close(leftPipe[WRITE]);
		            close(rightPipe[READ]);
		            close(rightPipe[WRITE]);
		            
		            if ( strcmp( newParams[0], "producer" ) == 0 ) {
             		 	char* args[] = {"producer", newParams[1], NULL};
             		 	execv( "producer", args);
             		 } else {
		         		 execv(path, newParams);
             		 } /* Execute the command */

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
             		
             		 dup2(rightPipe[READ], READ);
             		 
             		 close( rightPipe[WRITE] ); /* Right child does not write to the rightPipe */
             		 close( leftPipe[READ] );
             		 close( leftPipe[WRITE] );
             		 close( rightPipe[READ] ); 
             		 
             		 if ( strcmp( newParams[0], "consumer" ) == 0 ) {
             		 	char* args[] = {"consumer", newParams[1], NULL};
             		 	execv( "consumer", args);
             		 } else {
		         		 execv(path, newParams);
             		 } /* Execute the command */
		             return -1;
		        }
		      
             	
             	while((wpid = wait(&status)) > 0); /* Parent waits here for all its children to terminate */
             	close( rightPipe[WRITE] ); /* Right child does not write to the rightPipe */
             	close( leftPipe[READ] );
             	close( leftPipe[WRITE] );
             	close( rightPipe[READ] ); 
             	
             	printf("\n\ncharacter-count: %d\n", numberOfBytesTransferred);
             	printf("read-call-count: %d\n", writeReadCount);
             	printf("write-call-count: %d\n", writeReadCount);
             }
         }
         for ( int i = 0; i < paramc; i++ )
             free(paramv[ i ]);
             
         printf("\n");
         gettimeofday(&end, NULL);
         printf("Total time = %f seconds\n", (double) (end.tv_usec - start.tv_usec) / 1000000 + (double) (end.tv_sec - start.tv_sec));
     }
     return 0;
}
