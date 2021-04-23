#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <sys/stat.h>

#ifndef WWPATH
#define WWPATH "/Users/benjaminpasternak/Desktop/assignment3/ww"
#endif

/**
 * NOTE!!!!!!!!!!!
 * RUN FORMAT SHOULD BE:
 * ./wcat pagewidth text_file/directory seperated by spaces 
 * ex. ./wcat 20 test1.txt test2 test3.txt
 */

int is_directory(const char *path){
	struct stat statbuf;
	if (stat(path, &statbuf) != 0)
		return 0;
	return S_ISDIR(statbuf.st_mode);
}
/**
 * Input format = ./forking_around 20 text1.txt ...
 * 
 */

int main(int argc, char** argv)
{
    int numFiles = argc - 2;  // null + argc + int = 3
    //printf("argc = %d\n", argc);
    // there aren't enough arguments 
    if (numFiles < 1) 
    {
        printf("Not enough arguments\n");
    }



    int i; 
    int is_err = 0; // if any error is encountered that is not terminal, this flag is set 
    for (i = 0; i < numFiles; i++){
        
        // for adding new line after every paragraph except for the last
        //if (i != 0) puts("\n");

        int fd[2]; // fd are for the pipe
        pid_t child; 
        pipe(fd); // open the pipe so that we can read and write 
        child = fork(); // creating the child 

        if (child == 0){ // true if in child 
            //puts("in child");
            
            dup2(fd[1], 1); // setting write end to stdout
            // checking if file is directory part II 
            if (is_directory(argv[2+i])){
                is_err = 1; // encountered error, must return EXIT_FAILURE
                fprintf(stderr, "File is a directory, does not compute...\n"); // writing to stderr 
                //perror("dir");
              	exit(EXIT_FAILURE);
                //continue;  
            } else { // if not a directory execute the program 
                close(fd[0]);
                close(fd[1]);
                execl(WWPATH, WWPATH , argv[1], argv[2+i], NULL); // run ww with following params 
                exit(EXIT_FAILURE); // set exit status upon failure
            }
        } else if (child > 0){
            
            close(fd[1]);// we do not need the write end here, so we close 
            
            // we want to collect the exit status of the child process here in the parent
            int wstatus;
            child = wait(&wstatus);
            // if the exit status is not success then we set error and continue processing 
            if (WEXITSTATUS(wstatus) != EXIT_SUCCESS) {
                is_err = 1;
            } else {
                // if the exit status was a success we write from pipe to stdout 
                // and insert \n when appropriate 
                char buf[100];
                int r;
                int is_empty = 0;
                int count = 0; 
                //printf("  woof\n");
                while ((r = read(fd[0], buf, 100)) >= 0){

                    if (r == -1){ // error with read 
                        perror("read"); 
                        // not sure if exit(1); here
                    } else if (r == 0 ){
                        if (count == 0){
                            //fprintf(stderr, "broke...%s \n", argv[2+i]); // writing to stderr 
                            //printf(" Broke File: %s", argv[2+i]);
                            is_empty = 1;
                            break;
                        } else {
                            break;
                        }
                    }else {
                        // write piped input to stdout 
                        write(0, buf, r);
                    }
                    count ++;
                }
                if (is_empty != 1) puts("\n");
            }
            close(fd[0]);

        } else { // there was an error in fork (returns -1)
            perror("fork");
            // or abort(); 
            exit(1);
        }
        

    }
    // if a non terminal error was encountered, return exit failure 
    if (is_err == 1) return EXIT_FAILURE;

    return EXIT_SUCCESS;


    

    

}

