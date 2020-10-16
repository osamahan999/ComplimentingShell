#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>

#define MAXLINE 80 /* The maximum length of a command line */
#define MAXARGS 40 /* The maximum # of tokens in command line */
#define READ_END 0 //pipe read end
#define WRITE_END 1 //pipe write end

int main(void) {
    const int SIZE = 4096;
    const char *name = "OS";
    const char EXIT_COMMAND[] = "exit\n";
    const char delim[] = " ";


    char cmdline[MAXLINE];
    char *args[MAXARGS]; 
    int shm_fd; // shared memory file descriptor
    void *memPtr; //memory pointer
    int pipe_fd[2];

    /**
     * Parent creates the memory object and maps it pre kernel comes up 
     **/
    



    printf("Hi Osama Welcome to Your Shell!\n"); /*replace w/ name*/
    while(1) {
        printf("Osama>");/* prompt-replace FirstName and L3SID */
        fflush(stdout);
        int hasAmp = 0;

        for (int i = 0; i < MAXARGS; i++) { //clear out args arr
            args[i] = NULL;
        }

        
        

        fgets(cmdline, MAXLINE, stdin);
        if (strcmp(EXIT_COMMAND, cmdline) == 0) exit(0); //checks to see if exit command input
        if (strcmp("\n", cmdline) == 0) {
            printf("An empty command, that's alright I sometimes have trouble thinking of things \nto put in here too. Don't worry about it too much just take your time!\n");
            continue;
        }

        int argLength = 0; //after the increments end, it will tell you where the NULL is in the args arr
        char *ptr = strtok(cmdline, delim);


        while(ptr != NULL)
        {
            /**
             * If has ampersand set bool to true */
            if (strcmp(ptr, "&\n") == 0) {
                hasAmp = 1; 
                ptr = strtok(NULL, delim);

            }
            else {
                args[argLength] = ptr;
                argLength++;

                ptr = strtok(NULL, delim);
            }
            
        }

        /**
         * Remove \n from the last arg */
        char temp[strlen(args[argLength - 1])];
        sprintf(temp, "%s", args[argLength - 1]);
        temp[strcspn(temp, "\n")] = 0;       
        args[argLength - 1] = temp;

        
        //set last arg to NULL
        args[argLength] = NULL;
        


        shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666); //creates shared memory
        ftruncate(shm_fd, SIZE); //sets size of shared mem obj
        memPtr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0); //memory map shared mem


        int pid = fork();


        //check if failed
        if (pid < 0) {
            fprintf(stderr, "Fork Failed");
            return 1;
        }


        // no &
        if (!hasAmp) {

            struct timeval start, end;

            if (pid != 0) { // parent

                int status;
                wait(&status);
                
                if (WIFEXITED(status)) {
                    // printf("Child process existed with status %d \n", WEXITSTATUS(status));
                }

                gettimeofday(&end, NULL);

                char *timePtr = strtok(memPtr, ".");
                int startSeconds = atoi(timePtr);
                int startMSeconds = atoi(strtok(NULL, "."));

                int totalElapsedSeconds = end.tv_sec - startSeconds;
                int totalElapsedMSec = end.tv_usec - startMSeconds; 

                // printf("Total elapsed time Sec.Msec %1d.%06d \n", totalElapsedSeconds, totalElapsedMSec);

              
                shm_unlink(name);

                
            } else { // child

                gettimeofday(&start, NULL);

                char str[256];

                sprintf(str, "%1ld.%06ld", start.tv_sec, start.tv_usec); //stores it as string because I had a hard time finding out how to store floats in shared mem lol
                sprintf(memPtr,"%s", str);

                int counter;
                int needPipe = 0;

                //checcks for pipe command; first index cannot be that or else its just useless computing
                for (counter = 1; counter < argLength; counter++) {
                    if (strcmp(args[counter], "|") == 0) {
                        needPipe = 1;
                        break;
                    }
                }

                if (needPipe) {
                    
                    //initiates left and right args arrays with their last values being null
                    char *argsLeft[counter + 1];
                    argsLeft[counter] = NULL;
                    char *argsRight[argLength - counter];
                    argsRight[argLength-counter-1] = NULL;


                    //create left and right
                    int j, k;

                    for (j = 0; j < counter; j++) {
                        argsLeft[j] = args[j];
                        
                    }
                    for (k = 0; k < argLength - counter - 1; k++) {
                        argsRight[k] = args[counter + k + 1];
                    } 

                    

                    if (pipe(pipe_fd) == -1) {
                        fprintf(stderr, "pipe failed\n");
                        return 1;
                    }

                    int pid2 = fork();

                    if (pid2 < 0) { //faiilure
                        fprintf(stderr, "Fork failed\n");
                        return 1;

                    } 
                    
                    if (pid2 == 0) { //grandchild
                        close(pipe_fd[READ_END]);

                        dup2(pipe_fd[WRITE_END], STDOUT_FILENO); //stdout fd changed to pipe_fd wwrite end
                        close(pipe_fd[WRITE_END]);

                        printf("Wow piping so soon, wine and dine me first!\n");
                        if (strcmp(args[0], "ls") == 0) printf("What a beautiful directory, but not as beautiful as you!\n");
                        if (strcmp(args[0], "sudo") == 0) printf("Sudo sounds like psuedo, which is the opposite of the positive effect you have on my mood!\n");
                        
                        if (execvp(argsLeft[0], argsLeft) == -1) { //i think here the stdout should go into the pipefd write end
                            printf("Error : execvp did not execute successfully.\n");    
                            exit(EXIT_FAILURE);
                        }      
                        
                        if (strcmp(args[0], "clear") == 0) printf("You might have cleared the shell, but you will never erase OUR history!\n");

                        

                    } else { //child of shell
                        close(pipe_fd[WRITE_END]);

                        dup2(pipe_fd[READ_END], STDIN_FILENO); //stdin changed to read end of pipefd
                        close(pipe_fd[READ_END]);

                
                        if (execvp(argsRight[0], argsRight) == -1) {
                            printf("Error : execvp did not execute successfully.\n");    
                            exit(EXIT_FAILURE);
                        }   

                    }   

                } else { //no need for piping just run the command 
                    
                    if (strcmp(args[0], "ls") == 0) printf("What a beautiful directory, but not as beautiful as you!\n");
                    if (strcmp(args[0], "sudo") == 0) printf("Sudo sounds like psuedo, which is the opposite of the positive effect you have on my mood!\n");
                    // if (strcmp(args[0], "ls") == 0) printf("What a beautiful directory, but not as beautiful as you!\n");


                    if (execvp(args[0], args) == -1) {

                        printf("Error : execvp did not execute successfully.\n");    
                        exit(EXIT_FAILURE);
                    }
                    
                    if (strcmp(args[0], "clear") == 0) printf("You might have cleared the shell, but you will never erase OUR history!\n");


                }
            }


        } else { // ampersand just run the command  no waiting from parent

            if (pid == 0)  {
                if (execvp(args[0], args) == -1) {
                        printf("Error : execvp did not execute successfully.\n");    
                        exit(EXIT_FAILURE);
                    }            
                }
        }



    }

    return 0;
}
