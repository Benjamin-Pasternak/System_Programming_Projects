#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h> 
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
# include <sys/types.h>
#include <errno.h>
#define BUFFER_SIZE 50

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int is_directory(const char *path){
	struct stat statbuf;
	if (stat(path, &statbuf) != 0)
		return 0;
	return S_ISDIR(statbuf.st_mode);
}



//Method to wrap the text according to the user input
int wrap(int read_fd, int write_fd, unsigned page_width){
	int bytes_read = 0;
	int word_counter = 0;
	int word_starting_position = 0;
	int current_line_count = 0;
	char buf[BUFFER_SIZE];
	int has_previous = 0;
	int has_previous_new_line = 0;
	int status_flag = 0;
	char *temp;
	
	bytes_read = read(read_fd, &buf, BUFFER_SIZE);
	
	//The loop will run until we reach the end of the file
	while(bytes_read != 0){	
		
		//Writing to the output
		for(int i = 0; i < bytes_read; i++){
			
			//Check if it is a white-space
			if(isspace(buf[i])){
				
			    //Check if it is a new line
			    if(buf[i] == '\n' || buf[i] == '\r'){
					
					//If the previous buffer has the paragraph break
					if(has_previous_new_line > 1){
						has_previous_new_line++;
						
					}

					//If this is a paragraph break
					else if(has_previous_new_line == 1){
						//Adding the new line for paragraph break
						if(write(write_fd, "\n", 1) == -1){
							printf("Error writing at line 114 in source code.\n");
						}
						
						has_previous_new_line++;						
					}
				
					//If we have previou word left
					else if(has_previous == 1){

						has_previous_new_line = 0;
						
						//If the length of the word is larger than the page width
						if(word_counter >= page_width && current_line_count == 0){
							
							status_flag = 1;
						
							//Writing the word on the current line
							if(write(write_fd, temp, word_counter) == -1){
								printf("Error writing at line 132 in source code.\n");
							}
								
							//Incrementing the line counter
							current_line_count = word_counter + current_line_count;
					
							//Adding the new line
							if(write(write_fd, "\n", 1) == -1){
								printf("Error writing at line 138 in source code.\n");
							}
							
							has_previous_new_line = 1;	
							current_line_count = 0;
						}						
						
						//Add the previous word to the output with the new line
						else if(current_line_count + word_counter > page_width){
					
							//Adding the new line
							if(write(write_fd, "\n", 1) == -1){
								printf("Error writing at line 114 in source code.\n");
							}
						
							if(word_counter > page_width){
								status_flag = 1;
							}						
					
							//Writing the word on the new line
							if(write(write_fd, temp, word_counter) == -1){
								printf("Error writing at line 117 in source code.\n");
							}
						
							//Updating the line counter
							current_line_count = word_counter;
					
							//Adding the new line
							if(write(write_fd, "\n", 1) == -1){
								printf("Error writing at line 123 in source code.\n");
							}
							
							has_previous_new_line = 1;
							current_line_count = 0;
						}

						//Otherwise writing on the current line
						else{
						
							if(word_counter > page_width){
								status_flag = 1;
							}
						
							//Writing the word on the current line
							if(write(write_fd, temp, word_counter) == -1){
								printf("Error writing at line 132 in source code.\n");
							}
								
							//Incrementing the line counter
							current_line_count = word_counter + current_line_count;
					
							//Adding the new line
							if(write(write_fd, "\n", 1) == -1){
								printf("Error writing at line 138 in source code.\n");
							}
							
							has_previous_new_line = 1;	
							current_line_count = 0;				
						}

						has_previous = 0;
						free(temp); 	
					}

					//If the length of the word is larger than the page width
					else if(word_counter >= page_width && current_line_count == 0){

						has_previous_new_line = 0;
						
						//Writing the word on the current line
						if(write(write_fd, &buf[word_starting_position], word_counter) == -1){
							printf("Error writing at line 132 in source code.\n");
						}
								
						//Incrementing the line counter
						current_line_count = word_counter + current_line_count;
					
						//Adding the new line
						if(write(write_fd, "\n", 1) == -1){
							printf("Error writing at line 138 in source code.\n");
						}
							
						has_previous_new_line = 1;	
						current_line_count = 0;
					}
				
					//Add the word to the output with the new line
					else if(current_line_count + word_counter > page_width){
						
						has_previous_new_line = 0;

						//Adding the new line
						if(write(write_fd, "\n", 1) == -1){
							printf("Error writing at line 114 in source code.\n");
						}
					
						if(word_counter > page_width){
							status_flag = 1;
						}
					
						//Writing the word on the new line
						if(write(write_fd, &buf[word_starting_position], word_counter) == -1){
							printf("Error writing at line 117 in source code.\n");
						}
							
						//Updating the line counter
						current_line_count = word_counter;
					
						//Adding the new line
						if(write(write_fd, "\n", 1) == -1){
							printf("Error writing at line 123 in source code.\n");
						}
						
						has_previous_new_line = 1;	
						current_line_count = 0;
					}
				
					//Otherwise writing on the current line
					else{
						
						has_previous_new_line = 0;
					
						if(word_counter > page_width){
							status_flag = 1;
						}
					
						//Writing the word on the current line
						if(write(write_fd, &buf[word_starting_position], word_counter) == -1){
							printf("Error writing at line 132 in source code.\n");
						}
							
						//Incrementing the line counter
						current_line_count = word_counter + current_line_count;
					
						//Adding the new line
						if(write(write_fd, "\n", 1) == -1){
							printf("Error writing at line 138 in source code.\n");
						}
						
						has_previous_new_line = 1;
						current_line_count = 0;
					}

					//Updating the new word start position
					word_starting_position = i + 1;
				
					//Updating the word counter
					word_counter = 0;
					
				}
				
				//White-spaces except new lines
				else{
					has_previous_new_line = 0;

					//Check if it is the first character in the new word
					if(word_counter == 0){
						//Do nothing
					}				

					//Check if we have previous word left
					else if(has_previous == 1){
						
						//If the length of the word is larger than the page width
						if(word_counter >= page_width && current_line_count == 0){
						
							//Writing the word on the current line
							if(write(write_fd, temp, word_counter) == -1){
								printf("Error writing at line 132 in source code.\n");
							}
								
							//Incrementing the line counter
							current_line_count = word_counter + current_line_count;
					
							//Adding the new line
							if(write(write_fd, "\n", 1) == -1){
								printf("Error writing at line 138 in source code.\n");
							}
							
							has_previous_new_line = 1;
							current_line_count = 0;
						}

						//Add the previous word to the output with the new line
						else if(current_line_count + word_counter + 1 > page_width){

							//Adding the new line
							if(write(write_fd, "\n", 1) == -1){
								printf("Error writing at line 114 in source code.\n");
							}

							if(word_counter > page_width){
								status_flag = 1;
							}
					
							//Writing the word on the new line
							if(write(write_fd, temp, word_counter) == -1){
								printf("Error writing at line 117 in source code.\n");
							}
								
							//Updating the line counter
							current_line_count = word_counter;
					
							//Adding the new line
							if(write(write_fd, " ", 1) == -1){
								printf("Error writing at line 123 in source code.\n");
							}
								
							current_line_count = current_line_count + 1;
						}

						//Otherwise writing on the current line
						else{

							if(word_counter > page_width){
								status_flag = 1;
							}
						
							//Writing the word on the current line
							if(write(write_fd, temp, word_counter) == -1){
								printf("Error writing at line 132 in source code.\n");
							}
								
							//Incrementing the line counter
							current_line_count = word_counter + current_line_count;
					
							//Adding the new line
							if(write(write_fd, " ", 1) == -1){
								printf("Error writing at line 138 in source code.\n");
							}
							
							current_line_count = current_line_count + 1;				
						}

						has_previous = 0;
						free(temp);	
					}

					//If the length of the word is larger than the page width
					else if(word_counter >= page_width && current_line_count == 0){
						
						//Writing the word on the current line
						if(write(write_fd, &buf[word_starting_position], word_counter) == -1){
							printf("Error writing at line 132 in source code.\n");
						}
								
						//Incrementing the line counter
						current_line_count = word_counter + current_line_count;
					
						//Adding the new line
						if(write(write_fd, " ", 1) == -1){
							printf("Error writing at line 138 in source code.\n");
						}
							
						has_previous_new_line = 1;	
						current_line_count = 0;
					}
					
					//Add the word to the output with the new line
					else if(current_line_count + word_counter + 1 > page_width){
					
						//Adding the new line
						if(write(write_fd, "\n", 1) == -1){
							printf("Error writing at line 173 in source code.\n");
						}
					
						if(word_counter > page_width){
							status_flag = 1;
						}

						//Writing the word on the new line
						if(write(write_fd, &buf[word_starting_position], word_counter) == -1){
							printf("Error writing at line 178 in source code.\n");
						}
							
						//Updating the line counter
						current_line_count = word_counter;
					
						//Adding the white-space
						if(write(write_fd, " ", 1) == -1){
							printf("Error writing at line 186 in source code.\n");
						}
							
						current_line_count = current_line_count + 1;
					}
				
					//Otherwise writing on the current line
					else{
						if(word_counter > page_width){
							status_flag = 1;
						}
					
						//Writing the word on the current line
						if(write(write_fd, &buf[word_starting_position], word_counter) == -1){
							printf("Error writing at line 197 in source code.\n");
						}					
					
						//Incrementing the line counter
						current_line_count = word_counter + current_line_count;

					
						//Adding the white-space
						if(write(write_fd, " ", 1) == -1){
							printf("Error writing at line 205 in source code.\n");
						}
			
						current_line_count = current_line_count + 1;
					}
				
					//Updating the new word start position
					word_starting_position = i + 1;
				
					//Updating the word counter
					word_counter = 0;
				
					//Check for consecutive white-spaces
					for(int k = i; k < bytes_read; k++){

						//If there are no more whitespaces
						if(buf[k] != '\n' && buf[k] != '\r' && !isspace(buf[k])){
						 	i = k - 1;
							word_starting_position = k;
							break;
						}
					}

				}
			}
			
			//Otherwise continue to read
			else{
				has_previous_new_line = 0;
				word_counter++;
				
				//Check if we have previous temp array
				if(has_previous == 1){
					
					//Reallocating the temp array
					char *p = realloc(temp, word_counter);
					if (!p) return 1;
					
					temp = p;
					
					//Adding the element to the temp array
					temp[word_counter - 1] = buf[i];			
				}


				//Check if it is the last byte in the buffer
				else if(i == BUFFER_SIZE - 1 && has_previous == 0){

					has_previous = 1;

					temp = malloc(word_counter);

					//Copying the current word into the temp array
					int tempCounter = 0;

					for(int m = word_starting_position; m < bytes_read; m++){
						temp[tempCounter] = buf[m];
						tempCounter++;											
					}
				}

			}

		}
		
		bytes_read = read(read_fd, &buf, BUFFER_SIZE);

		if(bytes_read != 0){
			word_starting_position = 0;
		}
	}

	//If we have reached at the end of the file
	if (word_counter != 0){
			
		//If we have previou word left
		if(has_previous == 1){

			//If the length of the word is larger than the page width
			if(word_counter >= page_width && current_line_count == 0){
							
				status_flag = 1;
						
				//Writing the word on the current line
				if(write(write_fd, temp, word_counter) == -1){
					printf("Error writing at line 132 in source code.\n");
				}
								
				//Incrementing the line counter
				current_line_count = word_counter + current_line_count;
								
				has_previous_new_line = 1;	
				current_line_count = 0;
			}
			//Add the previous word to the output with the new line
			else if(current_line_count + word_counter > page_width){
					
				//Adding the new line
				if(write(write_fd, "\n", 1) == -1){
					printf("Error writing at line 114 in source code.\n");
				}	
						
				if(word_counter > page_width){
					status_flag = 1;
				}							
					
				//Writing the word on the new line
				if(write(write_fd, temp, word_counter) == -1){
					printf("Error writing at line 117 in source code.\n");
				}
						
				//Updating the line counter
				current_line_count = word_counter;
							
				has_previous_new_line = 1;
				current_line_count = 0;
			}
			//Otherwise writing on the current line
			else{
						
				if(word_counter > page_width){
					status_flag = 1;
				}
						
				//Writing the word on the current line
				if(write(write_fd, temp, word_counter) == -1){
					printf("Error writing at line 132 in source code.\n");
				}
								
				//Incrementing the line counter
				current_line_count = word_counter + current_line_count;
							
				has_previous_new_line = 1;	
				current_line_count = 0;				
			}

			word_counter = 0;
			free(temp);
		}

		//If the length of the word is larger than the page width
		else if(word_counter >= page_width && current_line_count == 0){
							
			status_flag = 1;
						
			//Writing the word on the current line
			if(write(write_fd, temp, word_counter) == -1){
				printf("Error writing at line 132 in source code.\n");
			}
								
			//Incrementing the line counter
			current_line_count = word_counter + current_line_count;
							
			has_previous_new_line = 1;	
			current_line_count = 0;
		}
			
		//Add the word to the output with the new line
		else if(current_line_count + word_counter > page_width){
					
			//Adding the new line
			if(write(write_fd, "\n", 1) == -1){
				printf("Error writing at line 114 in source code.\n");
			}
					
			if(word_counter > page_width){
				status_flag = 1;
			}
					
			//Writing the word on the new line
			if(write(write_fd, &buf[word_starting_position], word_counter) == -1){
				printf("Error writing at line 117 in source code.\n");
			}
							
			//Updating the line counter
			current_line_count = word_counter;
						
			has_previous_new_line = 1;	
			current_line_count = 0;
		}
				
		//Otherwise writing on the current line
		else{
					
			if(word_counter > page_width){
				status_flag = 1;
			}
					
			//Writing the word on the current line
			if(write(write_fd, &buf[word_starting_position], word_counter) == -1){
				printf("Error writing at line 132 in source code.\n");
			}
							
			//Incrementing the line counter
			current_line_count = word_counter + current_line_count;
							
			has_previous_new_line = 1;
			current_line_count = 0;
		}
	}
	
	//Closing the file descriptors
	close(read_fd);
	close(write_fd);
	
	return status_flag;
}


/*
Input:
	dirpath = the path to the directory
	cumulative path = the path as we traverse throught the directories contents 
	page_width = page_width as defined in main 
Return:
	void 
*/
int  directory(char *dirpath, char *cumulative_path, unsigned page_width){
	// opeining the directory like he demonstrated in class
	DIR *dirp = opendir(dirpath);
	struct dirent *de;
	// while the file is readable 
	while ((de = readdir(dirp))){

		// if the directory is either . or .. or .DS_Store (macosx proprietary file that is hidden) then skip
		if (strcmp(".", de->d_name) == 0 || strcmp("..", de->d_name) == 0 || strcmp(".DS_Store", de->d_name) == 0 || is_directory(de->d_name)){ //.DS_Store
			continue;
		}
		else{
			// checking if file was already wrapped. 
			if (strstr(de->d_name, "wrap.") != NULL || de->d_name[0] == '.') continue; 
			// adding a / to the end of temp which holds the path up until this point 
			char *new_path = concat(cumulative_path, "/");
			// this is the partial path of the file we are wrapping, currently only to stdout 
			char * new_path1 = concat(new_path, de->d_name);
            free(new_path);
			// opeining the file in readonly
			int read_fd = open(new_path1, O_RDONLY);
			free(new_path1);

			char *write_path = concat(cumulative_path, "/");
			char *a = concat(write_path, "wrap.");
            free(write_path);
			char *b = concat(a, de->d_name);
            free(a);
			mode_t mode = S_IRWXU;
			int write_fd = open(b, O_WRONLY | O_TRUNC | O_CREAT, mode);
			// perror here
			free(b);
			// calling your wrap function 
			wrap(read_fd, write_fd, page_width);
			//close(read_fd);
			close(write_fd);
		}
	}
	// close the directory
	closedir(dirp);
	return EXIT_SUCCESS;
}

int isFileExistsAccess(const char *path)
{
    // Check for file existence
    if (access(path, F_OK) == -1)
        return 0;

    return 1;
}

// method to run the program
int main(int argc, char **argv){

	// checking for missing paramiters. 
	if (argc != 3){
		perror("Please include all paramiters. form $./ww 20 folder_name\n");
		exit(1);
	}

	unsigned page_width = atoi(argv[1]);
	if(page_width < 0){
		return (EXIT_FAILURE);		
	}	
	
	int read_fd;
	int write_fd;
	
	//If a valid file is not present
	if(argc == 2 || argv[2][0] == '.' || strcmp(argv[2], "wrap.") == 0){
	 	read_fd = 0;
		write_fd = 1;
	}
	// checking if the input string is a directory 
	else if (is_directory(argv[2])){

		if (isFileExistsAccess(argv[2])==1)
		directory(argv[2], argv[2], page_width);
	}
	//If a valid file is present
	else {
		read_fd = open(argv[2], O_RDONLY);
		write_fd = 1;
		wrap(read_fd, write_fd, page_width);
		close(read_fd);
	}
	
	//Unable to open file
	if(read_fd == -1){
		perror("Unable to open the file\n");
	}
		
	return EXIT_SUCCESS;
}
