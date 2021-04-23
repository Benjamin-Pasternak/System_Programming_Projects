#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>

#ifndef DEBUG
#define DEBUG 0
#endif

volatile int dir_thread_count = 0; 

//Linked list to store the word frequency count
struct word_count {
	char *word;
	double count;
	struct word_count *next;
};

//A function to check if the word is in the correct format
void check_word(char *word) {

	for(char *ptr = word; *ptr; ++ptr){
		
		*ptr = tolower(*ptr);
	
	}

	for(char *ptr = word; *ptr; ++ptr){
		
		//remove all punctuations
		if(!ispunct(*ptr)){
			*word++ = *ptr;
		}
		
	}
	
	//Adding the null byte
	*word = '\0';
}

//A function to compare two words with the length of the first word
int compare_words(struct word_count *a, struct word_count *b){
	return strcmp(a->word, b->word);
}

//A function to add the word or update the count of the word to the linked list
struct word_count *word_enqueue(struct word_count *head, char *word, unsigned count){
	struct word_count *ptr = (struct word_count*)malloc(sizeof(struct word_count));
	if(!ptr) return NULL;
	
	check_word(word);
	
	ptr->word = word;
	ptr->count = count;
	ptr->next = NULL;
	
	if(head == NULL || compare_words(ptr, head) < 0){
		
		if(DEBUG) { printf("The word %s is enqueued to the head\n", word); }	
		
		ptr->next = head;
		return ptr;
	}
	
	else if(compare_words(ptr, head) == 0){
		if(DEBUG) { printf("The count of the word: %s is increased to %f\n", ptr->word, head->count + 1); } 
		head->count = head->count + 1;
		free(ptr->word);
		free(ptr);
		return head;
	}

	else{
		struct word_count *current = head;
		
		while(current->next != NULL && compare_words(ptr, current->next) > 0){
			
			current = current->next;
		}
	
		//If we find a similar word then we increase the count
		if(current->next != NULL && compare_words(ptr, current->next) == 0){
				
			if(DEBUG) { printf("The count of the word: %s is increased to %f\n", ptr->word, current->next->count + 1); } 
			current->next->count = current->next->count + 1;
			free(ptr->word);
			free(ptr);
			return head;
		}

		ptr->next = current->next;
		current->next = ptr;
		
		if(DEBUG) { printf("The word %s is enqueued\n", word); }
		
		return head;
	}
}

//A function to dequeue the word linked list
struct word_count *word_dequeue(struct word_count *head){
	struct word_count *temp = head;
	
	head = head->next;
	
	free(temp->word);
	free(temp);
	
	return head;
}

//A struct that has the storage of wfd count of all files
struct wfd_repository {
	int number_of_files;
	struct wfd_per_file *head;
};

//A metohd to initalize the wfd_repository
int wfd_repository_init(struct wfd_repository *storage) {
	storage->number_of_files = 0;
	storage->head = NULL;

	return 0;
}

//A struct that has the linked list of the words and the number of words in that file
struct wfd_per_file {
	char *file_name;
	int number_of_words;
	struct word_count *head;
	struct wfd_per_file *next;
};

//A method to initialize the wfd_repository
int wfd_per_file_init(struct wfd_per_file *storage){
	storage->file_name= NULL;
	storage->number_of_words = 0;
	storage->head = NULL;
	storage->next = NULL;
	
	return 0;
}

//A method to enqueue the wfd_per_file
struct wfd_per_file *wfd_enqueue(struct wfd_per_file *head, struct wfd_per_file *temp){

	if(DEBUG) { printf("In the wfd_enqueue function\n"); }
	
	struct wfd_per_file *ptr = head;
	
	if(ptr == NULL){
		if(DEBUG) { printf("The wfd_repository is added to the head\n"); }
		ptr = temp;
		return ptr;
	}
	
	while(ptr->next != NULL){
		ptr = ptr->next;
	}
	
	ptr->next = temp;
	return head;
}

//A method to dequeue the wfd_repository
struct wfd_per_file *wfd_dequeue(struct wfd_per_file *head){

	if(head == NULL){
		return NULL;
	}
	
	struct wfd_per_file *temp = head;
	
	head = head->next;
	
	struct word_count *temp_word_ptr = temp->head;
	
	while(temp_word_ptr != NULL){
		temp_word_ptr = word_dequeue(temp_word_ptr);
	}
	
	free(temp->file_name);
	free(temp);
	
	free(temp_word_ptr);	

	return head;
}

//A method to destroy the wfd repository
int wfd_repository_destroy(struct wfd_repository *storage) {
	
	struct wfd_per_file *temp = storage->head;
	
	while(temp != NULL){
		temp = wfd_dequeue(temp);
	}

	free(temp);

	return 0;
}

//A struct that stores the jsd comparison results of two files
struct jsd_results {
	char *file1;
	char *file2;
	int combined_word_count;
	pthread_mutex_t lock;
	double jsd;
};

int jsd_results_init(struct jsd_results *result){
	result->file1 = NULL;
	result->file2 = NULL;
	pthread_mutex_init(&result->lock, NULL);
	result->combined_word_count = 0;

	return 0;	
}

//Struct for file node
struct file_node{
	char *file_name;
	struct file_node *next;
};

//Struct for file queue
struct file_queue{
	struct file_node *front, *rear;
	int size;
	int open;
	pthread_mutex_t lock;
	pthread_cond_t read_ready;
};

//Initialize for file queue
int file_init(struct file_queue *Q){
	Q->front = NULL;
	Q->rear = NULL;
	Q->size = 0;
	pthread_mutex_init(&Q->lock, NULL);
	pthread_cond_init(&Q->read_ready, NULL);
	
	return 0;
}

//Enqueue for file queue
int file_enqueue(struct file_queue *Q, char *file){
	
	pthread_mutex_lock(&Q->lock);
	
	struct file_node *newNode = malloc(sizeof(struct file_node));
	newNode->file_name = malloc(sizeof(char) * (strlen(file) + 1));
	strcpy(newNode->file_name, file);
	newNode->next = NULL;
	
	//If the queue is empty
	if(Q->rear == NULL){
		Q->front = newNode;
		Q->rear = newNode;
	}
	
	//Else adding the node to the queue
	else{
		Q->rear->next = newNode;
		Q->rear = newNode;
	}	
	
	++Q->size;
	
	pthread_cond_signal(&Q->read_ready);
	
	pthread_mutex_unlock(&Q->lock);

	if(DEBUG) { printf("The file is enqueued\n"); }
	
	return 0;
}

//Struct for file node
struct dir_node{
	char *dir_name;
	struct dir_node *next;
};

//Struct for file queue
struct dir_queue{
	struct dir_node *front, *rear;
	int size;
	int threads_awake;
	pthread_mutex_t lock;
	pthread_cond_t read_ready;
};

//Dequeue for file queue that stores the file name in the given parameter
char *file_dequeue(struct file_queue *file_q, struct dir_queue *dir_q){
	
	char *file;

	pthread_mutex_lock(&file_q->lock);

	if(file_q->size == 0){
		
		if(file_q->size == 0){
			pthread_mutex_unlock(&file_q->lock);
			return NULL;
		}

	}

	file = file_q->front->file_name;

	struct file_node *temp = file_q->front;

	file_q->front = file_q->front->next;

	if(file_q->front == NULL){
		file_q->rear = NULL;
	}

	--file_q->size;
	free(temp);

	pthread_mutex_unlock(&file_q->lock);

	return file;
	
}

//Initialize for file queue
int dir_init(struct dir_queue *Q, int num_of_dir_threads){
	Q->front = NULL;
	Q->rear = NULL;
	Q->size = 0;
	Q->threads_awake = num_of_dir_threads;
	pthread_mutex_init(&Q->lock, NULL);
	pthread_cond_init(&Q->read_ready, NULL);
	
	return 0;
}

//Enqueue for file queue
int dir_enqueue(struct dir_queue *Q, char *dir){

	pthread_mutex_lock(&Q->lock);
	
	struct dir_node *newNode = malloc(sizeof(struct dir_node));
	newNode->dir_name = malloc(sizeof(char) * (strlen(dir) + 1));
	strcpy(newNode->dir_name, dir);
	newNode->next = NULL;
	
	
	//If the queue is empty
	if(Q->rear == NULL){
		Q->front = newNode;
		Q->rear = newNode;
	}
	
	//Else adding the node to the queue
	else{
		Q->rear->next = newNode;
		Q->rear = newNode;
	}	
	
	++Q->size;
	
	pthread_cond_signal(&Q->read_ready);
	
	pthread_mutex_unlock(&Q->lock);

	if (DEBUG) printf("The directory is enqueued\n");
	
	return 0;
}

//Old Dequeue for file queue that stores the file name in the given parameter
char *dir_dequeue(struct dir_queue *Q){
	pthread_mutex_lock(&Q->lock);
	
	char *dir;

	if(Q->size == 0){
		--Q->threads_awake;

		if(Q->threads_awake == 0){
			pthread_mutex_unlock(&Q->lock);
			pthread_cond_broadcast(&Q->read_ready);
			return NULL;		
		}
		
		while(Q->size == 0 && Q->threads_awake != 0){
			pthread_cond_wait(&Q->read_ready, &Q->lock);
		}
		
		if(Q->size == 0){
			pthread_mutex_unlock(&Q->lock);
			return NULL;		
		}
		++Q->threads_awake;
	}
	
	dir = Q->front->dir_name;
	
	struct dir_node *temp = Q->front;
	
	Q->front = Q->front->next;
	
	if(Q->front == NULL){
		Q->rear = NULL;
	}

	--Q->size;
	free(temp);
	
	pthread_mutex_unlock(&Q->lock);
	
	return dir;
}

//A function to check if a file path is a directory
int is_directory(const char *path){
	struct stat statbuf;
	if (stat(path, &statbuf) != 0)
		return 0;
	return S_ISDIR(statbuf.st_mode);
}

struct wfd_per_file *calculateWFD(struct wfd_per_file *word_count_table){

	if(DEBUG) { printf("In the calculateWFD function\n"); }
	
	int number_of_words = word_count_table->number_of_words;
	
	struct word_count *ptr = word_count_table->head;
	
	//Convert the word count to the frequencies
	while(ptr != NULL){
		ptr->count = ptr->count / number_of_words;
		ptr = ptr->next;
	}
	
	return word_count_table;
}

//A function to read the words from a file and add the WFD calculation to the WFD repository
struct wfd_repository *read_file(struct wfd_repository *storage, char *file_name){
	int read_fd = open(file_name, O_RDONLY);
	
	struct wfd_per_file *word_count_table = (struct wfd_per_file *)malloc(sizeof(struct wfd_per_file));

	wfd_per_file_init(word_count_table); 
	char buff[256];
	char *temp;
	
	word_count_table->file_name = file_name;
	
	int number_of_words = 0;
	int bytes_read = read(read_fd, buff, 256);
	int word_counter = 0;
	int word_starting_position = 0;
	int has_previous = 0;
	
	while(bytes_read != 0){
		for(int i = 0; i < bytes_read; i++){
			
			//If the byte is a white-space then we add the word to the list
			if(isspace(buff[i])){
				char *tempWord = (char *)malloc(sizeof(char) * (word_counter + 1));
				
				int counter = 0;
				for(int j = word_starting_position; j < i; j++){
					tempWord[counter] = buff[j];
					if(DEBUG) printf("%c\n", tempWord[counter]);
					counter++;
				}
				tempWord[counter] = '\0';
				
				if(DEBUG){ printf("The word is %s\n", tempWord); }
				
				word_count_table->head = word_enqueue(word_count_table->head, tempWord, 1);

				number_of_words++;
				
				word_counter = 0;
				
				//Check for consecutive white-spaces
				word_starting_position = i + 1;
				
				for(int k = i; k < bytes_read; k++){
					
					if(!isspace(buff[k])){
						i = k - 1;
						word_starting_position = k;
						break;
					}
				}
				
			}
			
			//We increment the word count
			else{
				word_counter++;
				
				//Cheack if we have previous temp array
				if(has_previous == 1){
					
					//Reallocating the temp array
					char *p = realloc(temp, sizeof(char) * (word_counter + 1));
					
					temp = p;
					
					temp[word_counter - 1] = buff[i];
					
				}
				
				//Check if it is the last byte
				else if(i == 255 && has_previous == 0){
					
					has_previous = 1;
					
					temp = malloc(word_counter);
					
					int tempCounter = 0;
					
					for(int m = word_starting_position; m < bytes_read; m++){
						temp[tempCounter] = buff[m];
						tempCounter++;
					}
				}
			}
		}
		
		bytes_read = read(read_fd, buff, 256);
		
		if(bytes_read != 0){
			word_starting_position = 0;
		}
	}
	//If we have reached at the end of the file
	if(word_counter != 0){
		//If we have previous word left then temp will have the last word
		if(has_previous == 1){
			word_count_table->head = word_enqueue(word_count_table->head, temp, 1);
		}
		
		//The file has the last word that ended without a white-space
		else{
			char *tempWord = malloc(sizeof(char) * (word_counter + 1));
			int counter = 0;
			// added +1 to below for loop 
			for(int j = word_starting_position; j < word_starting_position + word_counter; j++){
				tempWord[counter] = buff[j];
				if(DEBUG) printf("%c\n", tempWord[counter]);
				counter++;
			}

			tempWord[counter] = '\0';

			if (DEBUG) printf("tempWord = %s\n", tempWord);
			word_count_table->head = word_enqueue(word_count_table->head, tempWord, 1);

			if (DEBUG) printf("GOT HERE %d\n", __LINE__);
			number_of_words++;
		}
		
	}
	
	close(read_fd);
	
	if(DEBUG) { printf("The number of words are: %d\n", number_of_words); }
	
	word_count_table->number_of_words = number_of_words;
	word_count_table->next = NULL;
	
	word_count_table = calculateWFD(word_count_table);
	
	if(DEBUG) { printf("The WFD has been calculated\n"); }

	if(DEBUG) { printf("The file name is: %s\n", word_count_table->file_name); }
	
	storage->head = wfd_enqueue(storage->head, word_count_table);
	storage->number_of_files = storage->number_of_files + 1;
	
	if(DEBUG) { printf("The first word in the head of the WFD repository is: %s\n", storage->head->head->word); }

	return storage;
	
}

//A function to calculate the JSD of two files
double calculateJSD(struct wfd_per_file *file1, struct wfd_per_file *file2){
	
	struct word_count* words_file1 = file1->head;
	struct word_count* words_file2 = file2->head;
	
	double temp_mean_frequency = 0;
	double kld_file1 = 0;
	double kld_file2 = 0;
	
	//A loop to traverse the words in each file
	while(words_file1 != NULL || words_file2 != NULL){

		temp_mean_frequency = 0;

		//If there are no more words in the first file
		if(words_file2 == NULL){
			if(DEBUG) { printf("The frequency of the word: %s in file1 is %f\n", words_file1->word, words_file1->count); } 
			temp_mean_frequency = 0.5 * (0 + words_file1->count);
			kld_file2 = kld_file2 + (words_file1->count * log2(words_file1->count / temp_mean_frequency));
			
			words_file1 = words_file1->next;
		}
		
		//If there are no more words in the second file
		else if(words_file1 == 	NULL){
			if(DEBUG) { printf("The frequency of the word: %s in file2 is %f\n", words_file2->word, words_file2->count); }
			temp_mean_frequency = 0.5 * (0 + words_file2->count);
			kld_file2 = kld_file2 + (words_file2->count * log2(words_file2->count / temp_mean_frequency));
			
			words_file2 = words_file2->next;		
		}
		
		
		//If the words in both the files are equal
		else if(strcmp(words_file1->word, words_file2->word) == 0){

			if(DEBUG) { printf("The frequency of the word: %s in file1 is %f\n", words_file1->word, words_file1->count); }
			if(DEBUG) { printf("The frequency of the word: %s in file2 is %f\n", words_file2->word, words_file2->count); }
			temp_mean_frequency = 0.5 * (words_file1->count + words_file2->count);
			kld_file1 = kld_file1 + (words_file1->count * log2(words_file1->count / temp_mean_frequency));
			kld_file2 = kld_file2 + (words_file2->count * log2(words_file2->count / temp_mean_frequency));
			
			words_file1 = words_file1->next;
			words_file2 = words_file2->next;
		}
		
		//If the word in the file2 is ordered before file1 then file1 does not have that word
		else if(strcmp(words_file1->word, words_file2->word) > 0){

			if(DEBUG) { printf("The frequency of the word: %s in file2 is %f\n", words_file2->word, words_file2->count); }
			temp_mean_frequency = 0.5 * (0 + words_file2->count);
			kld_file2 = kld_file2 + (words_file2->count * log2(words_file2->count / temp_mean_frequency));
			
			words_file2 = words_file2->next;
		}
		
		//If the word in the file1 is ordered before file2 then file2 does not have that word
		else{
			
			if(DEBUG) { printf("The frequency of the word: %s in file1 is %f\n", words_file1->word, words_file1->count); }
			temp_mean_frequency = 0.5 * (words_file1->count + 0);
			kld_file1 = kld_file1 + (words_file1->count * log2(words_file1->count / temp_mean_frequency));
			
			words_file1 = words_file1->next;
		}
		
	}

	if(DEBUG) { printf("The KLD for file1 is: %f\n", kld_file1); }
	if(DEBUG) { printf("The KLD for file2 is: %f\n", kld_file2); }
	
	double jsd = sqrt((0.5 * kld_file1) + (0.5 * kld_file2));
	
	return jsd;
}

//A function to compare two jsd_results struct
int compare_jsd_results(const void *ptr_1, const void *ptr_2){
	
	struct jsd_results *ptr1 = (struct jsd_results *)ptr_1;
	struct jsd_results *ptr2 = (struct jsd_results *)ptr_2;

	if(ptr1->combined_word_count < ptr2->combined_word_count){
		return -1;
	}
	else if(ptr1->combined_word_count > ptr2->combined_word_count){
		return 1;
	}
	else{
		return 0;
	}
}

//A struct for the arguments of the file thread function
struct file_args {
	struct file_queue *file_q;
	struct dir_queue *dir_q;
	struct wfd_repository *storage;
};

//The file thread function that processes the file on one thread
void *file_thread(void *args){
	
	//The file queue
	struct file_queue *file_q = ((struct file_args*) args)->file_q;
	
	//The directory queue
	struct dir_queue *dir_q = ((struct file_args*) args)->dir_q;
	
	//The wfd_repository
	struct wfd_repository *storage = ((struct file_args*) args)->storage;

	if(DEBUG) { printf("The file thread function.\n"); }
	//sleep(2);

	
	//Loop until the file queue is empty and the directory threads are stopped
	
	while(1){

		if(DEBUG) { printf("The loop of the file thread function.\n"); }
		if (DEBUG) printf("f_q size: %d, dir_thread_count: %d ThreadID: %ld\n", file_q->size, dir_thread_count, pthread_self());

		if (DEBUG) printf("WAITING...\n");
		while (file_q->size == 0 && dir_q->threads_awake != 0){
			
			//pthread_cond_wait(&file_q->read_ready, &file_q->lock);
			pthread_mutex_unlock(&file_q->lock);
		}

		if (DEBUG) printf("NOT WAITING...\n");
		if (DEBUG) printf("f_q size: %d, dir_thread_count: %d ThreadID: %ld\n", file_q->size, dir_thread_count, pthread_self());
		
		if (file_q->size == 0 && dir_q->threads_awake == 0){
			if (DEBUG) printf("BROKEN !!!!!!!!!!!!!...\n");
			pthread_cond_broadcast(&file_q->read_ready);
			break;
		}
		if (DEBUG) printf("NOT BROKEN !!!!!!!!!!!!!...\n");
		//The name of the file is returned in dequeue

		char *file_name = file_dequeue(file_q, dir_q);

		if (DEBUG) printf("file_name: %s ThreadID: %ld LINE: %d\n", file_name, pthread_self(), __LINE__); 
		if (file_name == NULL) {
			continue;
		}
		//Reading the file and calculating the WFD
		storage = read_file(storage, file_name);
	}
	return NULL;
}

struct analysis_args {
	struct wfd_per_file *ptr1;
	struct wfd_per_file *ptr2;
	struct jsd_results *result;
};

//The analysis thread function that process the WFD of the files
void *analysis_thread(void *args){
	
	struct wfd_per_file *ptr1 = ((struct analysis_args *)args)->ptr1;
	struct wfd_per_file *ptr2 = ((struct analysis_args *)args)->ptr2;
	struct jsd_results *result = ((struct analysis_args *)args)->result;

	pthread_mutex_lock(&result->lock);

	result->file1 = ptr1->file_name;
	result->file2 = ptr2->file_name;
	result->combined_word_count = ptr1->number_of_words + ptr2->number_of_words;
	result->jsd = calculateJSD(ptr1, ptr2);

	pthread_mutex_unlock(&result->lock);	

	return NULL;
}

//A struct for the arguments of the directory function
struct dir_args {
	struct dir_queue *dir_q;
	struct file_queue *file_q;
	char *suffix;
};

//The directory thread function that processes the directory on one thread
void* dirThreadFun(void* arg) {
	struct dir_queue *dir_q = ((struct dir_args*) arg)->dir_q;
  	struct file_queue *file_q = ((struct dir_args*) arg)->file_q;
	char *suffix = ((struct dir_args*) arg)->suffix;

	while (dir_q->size != 0 && dir_q->threads_awake != 0){
		
		char *dirpath = dir_dequeue(dir_q); 
		DIR *dirp;
		struct dirent *de;
		if ((dirp = opendir(dirpath)) == NULL) {
			perror("cannot open directory");
			exit(1);
		}

		if (DEBUG) printf("Path: %s\n", dirpath);
		// reading directory entries 
		while((de = readdir(dirp))){
			char *new_path = (char *)malloc(strlen(dirpath) + strlen(de->d_name) + 3);
			strcpy(new_path, dirpath);
			strcat(new_path, "/");
			strcat(new_path, de->d_name);

			if (DEBUG) printf("Directory scan: %d %s %d\n",is_directory(new_path), de->d_name, __LINE__);
			if (DEBUG) printf("Path info: %s %d\n\n", new_path, __LINE__);

			if (strcmp(".", de->d_name) == 0 || strcmp("..", de->d_name) == 0 || strcmp(".DS_Store", de->d_name) == 0){
				free(new_path);
				continue;
			}

			else if (is_directory(new_path)){
				dir_enqueue(dir_q, new_path);
				if (DEBUG) printf("dir enqueue info: %d %s\n", dir_q->size, dir_q->front->dir_name);
			}
			else if (strcmp(suffix, &new_path[strlen(new_path) - strlen(suffix)]) == 0){
				file_enqueue(file_q, new_path);
				pthread_cond_signal(&file_q->read_ready);
			}
			free(new_path);
		}
		free(dirpath);
		closedir(dirp);
	}
	dir_thread_count--; 
	dir_q->threads_awake--;
	return NULL;
}

int main(int argc, char **argv){

	// CHECK FOR INPUT ARGS.....
	// there are 4 possible input args that all do specific things 
	// program running will be ./compare (args) (files)
	if (argc == 1) {
		fprintf(stderr, "Error on line %d : Too few args\n", __LINE__);
	}

	// FOR parsing the input args. NOT for initiating any thread operations.
	int dir_threads = 1;
	int file_threads = 1;
	int analysis_threads = 1;
	char *file_name_suffix;
	int suffix_set = 0;
	int i;

	struct dir_queue dir_q;
	struct file_queue file_q;
	dir_init(&dir_q, dir_threads);
	file_init(&file_q);


	for(i = 1; i < argc; i++) 
	{
		if(suffix_set == 0){
			file_name_suffix = ".txt";
		}
		
		if (argv[i][0] == '-'){
			if (argv[i][1] == 'd'){

				int temp = strlen(argv[i]);
				char *s = malloc(sizeof(char) * temp);
				memcpy(s, &argv[i][2], strlen(argv[i])-1);
				if (strlen(s) < 2) {
					perror("invalid argument\n");
					dir_threads = 1;
				}
				dir_threads = atoi(s);
				if (dir_threads < 1){
					perror("invalid argument\n");
					dir_threads = 1;
				}
				dir_init(&dir_q, dir_threads);
				free(s);

			} 
			else if (argv[i][1] == 'f'){

				int temp = strlen(argv[i]);
				char *s = malloc(sizeof(char) * temp);
				memcpy(s, &argv[i][2], strlen(argv[i])-1);
				if (strlen(s) < 2) {
					perror("invalid argument\n");
					file_threads = 1;
				}
				file_threads = atoi(s);
				if (file_threads < 1){
					perror("invalid argument\n");
					file_threads = 1;
				}
				file_init(&file_q);
				free(s);

			}
			else if (argv[i][1] == 'a'){

				int temp = strlen(argv[i]);
				char *s = malloc(sizeof(char) * temp);
				memcpy(s, &argv[i][2], strlen(argv[i])-1);
				if (strlen(s) < 2) {
					perror("invalid argument\n");
					analysis_threads = 1;
				}
				analysis_threads = atoi(s);
				if (analysis_threads < 1){
					perror("invalid argument\n");
					analysis_threads = 1;
				}
				free(s);

			} 
			else if (argv[i][1] == 's'){
				suffix_set = 1;
				int temp = strlen(argv[i]);
				char *s = malloc(sizeof(char) * temp);
				memcpy(s, &argv[i][2], temp-1);
				file_name_suffix = &argv[i][2];
				free(s);
			}
			else{
				perror("invalid parameter: Exiting program");
				exit(0);
			}
		}
	
		else {

			// for placing input args in appropriate queue
			if (is_directory(argv[i]))
			{
				// place dir in Dqueue so that threads may process later
				dir_enqueue(&dir_q, argv[i]);
			} 
			else 
			{
				if (strcmp(file_name_suffix, &argv[i][strlen(argv[i]) - strlen(file_name_suffix)]) == 0){
					// place dir in Dqueue so that threads may process later
					file_enqueue(&file_q, argv[i]);
				}
			}
		}
	}
	

	pthread_t *dir_tids = malloc(dir_threads * sizeof(pthread_t));
	pthread_t *file_tids = malloc(file_threads * sizeof(pthread_t));
	struct dir_args dirArgs;
	struct file_args fileArgs;
	struct wfd_repository *storage = (struct wfd_repository*)malloc(sizeof(struct wfd_repository));

	// initialize args
	wfd_repository_init(storage);
	dirArgs.dir_q = &dir_q;
	dirArgs.file_q = &file_q;
	dirArgs.suffix = file_name_suffix;

	fileArgs.dir_q = &dir_q;
	fileArgs.file_q = &file_q;
	fileArgs.storage = storage;
 

 	for(int i = 0; i < dir_threads; i++) {
		pthread_create(&dir_tids[i], NULL, dirThreadFun, &dirArgs);
		dir_thread_count++;
	}
	for(int i = 0; i < file_threads; i++) {
		pthread_create(&file_tids[i], NULL, file_thread, &fileArgs);	
	}

    
    for (int i = 0; i < dir_threads; i++) {
        pthread_join(dir_tids[i], NULL);
    }

	for(int i = 0; i < file_threads; i++) {
		pthread_join(file_tids[i], NULL);
	}

	free(dir_tids);
	free(file_tids);

	//Phase: 2
	
	pthread_t *analysis_tids = malloc(analysis_threads * sizeof(pthread_t));

	int size = (storage->number_of_files * (storage->number_of_files - 1)) / 2;

	struct analysis_args *analysisArgs = malloc(size * sizeof(struct analysis_args));

	int counter = 0;

	struct jsd_results *array = malloc(size * sizeof(struct jsd_results));

	struct wfd_per_file *ptr1;
	struct wfd_per_file *ptr2;

	if(storage->head != NULL && storage->head->next != NULL){
		ptr1 = storage->head;
		ptr2 = storage->head->next;
	}
	else{
		fprintf(stderr, "There are not enough valid files\n");
		abort();	
	}

	for(int i = 0; i < storage->number_of_files - 1; i++){
		
		for(int j = i + 1; j < storage->number_of_files; j++){
			
			analysisArgs[counter].ptr1 = ptr1;
			analysisArgs[counter].ptr2 = ptr2;
			jsd_results_init(&array[counter]);
			analysisArgs[counter].result = &array[counter];
			
			if(DEBUG) { printf("Counter is: %d\n", counter); }

			pthread_create(&analysis_tids[counter % analysis_threads], NULL, analysis_thread, &analysisArgs[counter]);

			ptr2 = ptr2->next;
			counter++;
		}

		ptr1 = ptr1->next;
		ptr2 = ptr1->next;
	}

	for(int i = 0; i < counter && i < analysis_threads; i++){
		pthread_join(analysis_tids[i], NULL);	
	}

	qsort(array, size, sizeof(struct jsd_results), compare_jsd_results);

	for(int i = 0; i < size; i++){
		
		printf("%f %s %s\n", array[i].jsd, array[i].file1, array[i].file2);
		
	}

	free(array);
	free(analysisArgs);
	free(analysis_tids);
	wfd_repository_destroy(storage);
	free(storage);
	
	return EXIT_SUCCESS;
}
