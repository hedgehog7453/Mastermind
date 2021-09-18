/* COMP30023 Project 2
 * Jiayu Li <jiayul3> 26/5/2016
 * server.c
 */

#include <stdio.h>

#include "header-server.h"


int newsockfd[NUM_SOCKETS];
int num_clients=0;
int num_success_clients=0;
pthread_mutex_t lock;

int main(int argc, char** argv) {

	// Catch and handle Ctrl-C
	signal(SIGINT, signal_handler);

	// Create log file if it does not exist, or clear the file if already exists
	FILE *f = fopen("log.txt", "w+");
	if (f == NULL) {
		printf("Error opening file\n");
		exit(1);
	}
	fclose(f);

	// Check the number of arguments
	if (argc < 2) {
		printf("Not enough arguments!\n");
		exit(1);
	}

	// Record the secret code if specified
	char* code = NULL;
	if (argc==3) {
		code = malloc((CODE_LEN+1)*sizeof(char));
		strcat(code, argv[2]);
	}

	int sockfd, portno;
	unsigned int clilen;
	struct sockaddr_in serv_addr, cli_addr;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	portno = atoi(argv[1]);
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR on binding");
		exit(1);
	}

	listen(sockfd, 5);
	clilen = sizeof(cli_addr);

	int index;
	pthread_t thread_id[NUM_SOCKETS]; 
	int n, i;

	// Initialize sockfd_status
	// (used for tracking status of 20 spots for sockfds)
	for (i = 0; i < NUM_SOCKETS; i++) {
		newsockfd[i] = 0;
	}
	
	while ((n=accept(sockfd, (struct sockaddr*)&cli_addr, &clilen))) {
		if (n<0) {
			perror("ERROR on accept");
			exit(1);
		} else {
			num_clients++;
			index = getAvailSockIndex(newsockfd);
			newsockfd[index] = n;

			// Initialize args
			args_t* args = malloc(sizeof(args_t));
			args->sockfd = n;
			args->inputcode = code;
			args->cli_ip = getIP(cli_addr);

			if (pthread_create(&thread_id[index], NULL, gamePlay, (void*)args)<0) {
				perror("ERROR on creating thread");
				exit(1);
			}
		}
	}
	free(code);
	return 0;
}

void* gamePlay(void* args) {

	int counter = 0; 
	int gameFinished = FALSE;
	int n; 

	char* code = NULL;
	char* file_entry = malloc((ENTRY_LEN+1)*sizeof(char));

	// Initialize secret code
	if (((args_t*)args)->inputcode==NULL) {
		code = generateCode();
	} else {
		int i;
		code = malloc((CODE_LEN+1)*sizeof(char));
		for (i=0; i<CODE_LEN; i++) {
			code[i] = ((args_t*)args)->inputcode[i];
		}
		code[CODE_LEN] = '\0';
	}

	// Write entry to file
	bzero(file_entry, ENTRY_LEN+1);
	sprintf(file_entry, "(%s)(soc_id %d) client connected\n", ((args_t*)args)->cli_ip, ((args_t*)args)->sockfd);
	writetofile(file_entry);

	// Write entry to file
	bzero(file_entry, ENTRY_LEN+1);
	sprintf(file_entry, "(0.0.0.0)(soc_id %d) server's secret=%s\n", ((args_t*)args)->sockfd, code);
	writetofile(file_entry);

	// Send a welcome message to client
	char* welcomeMessage = "Welcome to game MasterMind.\nEnter a code (combination of 4 alphabets from {A,B,C,D,E,F}):";
	if ((n=send(((args_t*)args)->sockfd, welcomeMessage, FEEDBACK_LEN+1, 0)) < 0) {
		perror("ERROR writing to socket");
		exit(1);
	}

	char* guess = malloc((CODE_LEN+1)*sizeof(char));
	char* feedback = malloc((FEEDBACK_LEN+1)*sizeof(char));

	// Loop until game is over
	while (1) {
		bzero(guess, CODE_LEN+1);
		if ((n = recv(((args_t*)args)->sockfd, guess, CODE_LEN+1, 0))<0) {
			perror("ERROR reading from socket");
			exit(1);
		}
		if (guess[3] == '\0') {
			// Write entry to file
			bzero(file_entry, ENTRY_LEN+1);
			sprintf(file_entry, "(%s)(soc_id %d) client terminated\n", ((args_t*)args)->cli_ip, ((args_t*)args)->sockfd);
			writetofile(file_entry);

			free(guess);
			free(feedback);
			free(code);
			close(((args_t*)args)->sockfd);
			removeSock(((args_t*)args)->sockfd);

			pthread_exit(NULL);
			return NULL;
		}
		printf("%d\n", counter);
		counter++;

		// Write entry to file
		bzero(file_entry, ENTRY_LEN+1);
		sprintf(file_entry, "(%s)(soc_id %d) client's guess=%s\n", ((args_t*)args)->cli_ip, ((args_t*)args)->sockfd, guess);
		writetofile(file_entry);

		// Generate feedback
		int validity = validateGuess(code, guess);
		bzero(feedback, FEEDBACK_LEN+1);
		char* status = malloc((STATUS_LEN+1)*sizeof(char));
		bzero(status, STATUS_LEN+1);
		if (validity == INVALID) {
			sprintf(feedback, "INVALID. Enter a code: ");
			sprintf(status, "INVALID");
			counter--;
		} else if (validity==SUCCESS) {
			sprintf(feedback, "SUCCESS! Number of guesses: %d", counter);
			sprintf(status, "SUCCESS");
			gameFinished = TRUE;
		} else if (validity==FAILURE && counter<MAX_NUM_ATTEMPTS) {
			free(feedback);
			feedback = generateFeedback(code, guess);
			strcpy(status, feedback);
			status[5]='\0';
		} else {
			sprintf(feedback, "FAILURE. Correct code is %s.\nThanks for playing", code);
			sprintf(status, "FAILURE");
			gameFinished = TRUE;
		}

		// Write entry to file
		bzero(file_entry, ENTRY_LEN+1);
		sprintf(file_entry, "(0.0.0.0)(soc_id %d) server's response=%s\n", ((args_t*)args)->sockfd, status);
		writetofile(file_entry);
		free(status);

		// Send the feedback to client
		if ((n=send(((args_t*)args)->sockfd, feedback, FEEDBACK_LEN+1, 0))<0) {
			perror("ERROR writing to socket");
			exit(1);
		}

		

		// Generate a flag indicating whether the game will finish at this round
		int msg = FALSE;
		if (gameFinished) {
			msg = TRUE;
			if ((n=send(((args_t*)args)->sockfd, &msg, sizeof(msg), 0))<0) {
				perror("ERROR writing to socket");
				exit(1);
			}

			// Write entry to file
			bzero(file_entry, ENTRY_LEN+1);
			if (validity==SUCCESS) {
				sprintf(file_entry, "(%s)(soc_id %d) SUCCESS game over\n", ((args_t*)args)->cli_ip, ((args_t*)args)->sockfd);
				num_success_clients++;
			} else {
				sprintf(file_entry, "(%s)(soc_id %d) FAILURE game over\n", ((args_t*)args)->cli_ip, ((args_t*)args)->sockfd);
			}
			writetofile(file_entry);

			free(guess);
			free(feedback);
			free(code);

			close(((args_t*)args)->sockfd);
			removeSock(((args_t*)args)->sockfd);

			pthread_exit(NULL);
			return NULL;
		}


		// free(guess);
		// free(feedback);
		// free(code);
		// Send the flag to client
		if ((n=send(((args_t*)args)->sockfd, &msg, sizeof(msg), 0))<0) {
			perror("ERROR writing to socket");
			exit(1);
		}
	}
}

/*
 * Generates a random secret code
 */
char* generateCode() {
	char* code = malloc((CODE_LEN+1)*sizeof(char));
	char candidates[] = {'A', 'B', 'C', 'D', 'E', 'F'};

	int i;
	for (i=0; i<CODE_LEN; i++) {
		srand(time(NULL)*(i+1));
		int index = (rand()%5);
		code[i] = candidates[index];
	}
	code[CODE_LEN] = '\0';
	return code;
}

/*
 * Returns INVALID if the guess is not consisted of {A, B, C, D, E, F}
 * Returns SUCCESS if the guess is identical to the secret code
 * Returns FAILURE if the guess is wrong
 */
int validateGuess(char* code, char* guess) {
	int i;
	int success = TRUE;
	for (i=0; i<CODE_LEN; i++) {
		if (guess[i]!=code[i]) {
			success = FALSE;
		}
		if (guess[i]!='A' && guess[i]!='B' && guess[i]!='C' 
			&& guess[i]!='D' && guess[i]!='E' && guess[i]!='F') {
			return INVALID;
		}
	}
	if (success) {
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

/*
 * Calculates and returns b and m
 */
char* generateFeedback(char* code, char* guess) {
	char* feedback = malloc((FEEDBACK_LEN+1)*sizeof(char));
	int b=0; int m=0; int i; int j;
	int visit[4] = {0,0,0,0};

	for (i=0; i<CODE_LEN; i++) {
		if (code[i]==guess[i]) {
			visit[i] = 2;
			b++;
		}
	}

	for (i=0; i<CODE_LEN; i++) {
		if (visit[i]==2) continue;
		for (j=0; j<CODE_LEN; j++) {
			if (guess[i]==code[j] && !visit[j]) {
				m++;
				visit[j]=1;
				break;
			}
		}
	}
	sprintf(feedback, "[%d:%d] Enter a code: ", b, m);
	return feedback;
}

char* getTime() {
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	char* time_string = malloc(11*sizeof(char));
	int day = timeinfo->tm_mday;
	int mon = (timeinfo->tm_mon)+1;
	int year = (timeinfo->tm_year)+1900;
	int hour = timeinfo->tm_hour;
	int min = timeinfo->tm_min;
	int sec = timeinfo->tm_sec;
	sprintf(time_string, "[%d %d %d %02d:%02d:%02d]", 
		day, mon, year, hour, min, sec);
	return time_string;
}

char* getIP(struct sockaddr_in cli_addr) {
	struct in_addr ipAddr = cli_addr.sin_addr;
	char* clientip = malloc(INET_ADDRSTRLEN*sizeof(char));
	inet_ntop(AF_INET, &ipAddr, clientip, INET_ADDRSTRLEN);
	return clientip;
}

int getAvailSockIndex(int* sockfds) {
	int i;
	for (i=0; i<NUM_SOCKETS; i++) {
		if (sockfds[i]==0) return i;
	}
	return INVALID;
}

int getSockIndex(int* sockfds, int sockfd) {
	int i;
	for (i=0; i<NUM_SOCKETS; i++) {
		if (sockfds[i]==sockfd) return i;
	}
	return INVALID;
}

void removeSock(int sockfd) {
	int i;
	for (i=0; i<NUM_SOCKETS; i++) {
		if (newsockfd[i]==sockfd) newsockfd[i] = 0;
	}
}

void writetofile(char* content) {
	pthread_mutex_lock(&lock);
	FILE *f = fopen("log.txt", "a");
	if (f == NULL) {
		printf("Error opening file\n");
		exit(1);
	}

	char* time_string = getTime();
	fprintf(f, "%s%s", time_string, content);
	free(time_string);

	fclose(f);
	pthread_mutex_unlock(&lock);
}

void signal_handler() {
	int i;
	for (i=0; i<NUM_SOCKETS; i++) {
		if (newsockfd[i]!=0) close(newsockfd[i]);
	}

	FILE *f = fopen("log.txt", "a");
	if (f == NULL) {
		printf("Error opening file\n");
		exit(1);
	}

	fprintf(f, "Number of clients connected = %d\n", num_clients);
	fprintf(f, "Number of clients that successfully guessed the code = %d\n", 
		num_success_clients);

	struct rusage usage;
	getrusage(RUSAGE_SELF, &usage);
	struct timeval utimeval, ktimeval;
	utimeval = usage.ru_utime;
	ktimeval = usage.ru_stime;
	fprintf(f, "Memory usage = %ld kb\n", usage.ru_maxrss);
	fprintf(f, "User timeval = %d ms, Kernel timeval = %d ms\n", 
		utimeval.tv_usec, ktimeval.tv_usec);

	char* time_string = getTime();
	fprintf(f, "%s(0.0.0.0) server terminated\n", time_string);
	free(time_string);

	fclose(f);
	exit(0);
}
