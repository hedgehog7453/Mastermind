/* COMP30023 Project 2
 * Jiayu Li <jiayul3> 26/5/2016
 * client.c
 */

#include <stdio.h>

#include "header-client.h"

int sockfd;

int main(int argc, char** argv) {

	int portno;
	struct sockaddr_in serv_addr;
	struct hostent* server;

	// Catch and handle ctrl-C
	signal(SIGINT, signal_handler);

	if (argc<3) {
		fprintf(stderr, "IP and port number not specified\n");
		exit(0);
	}

	server = gethostbyname(argv[1]);
	if (server==NULL) {
		fprintf(stderr, "ERROR no such host\n");
		exit(0);
	}

	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, 
		server->h_length);
	portno = atoi(argv[2]);
	serv_addr.sin_port = htons(portno);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0) {
		perror("ERROR opening socket");
		exit(0);
	}
	
	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting");
		exit(0);
	}

	gamePlay(sockfd);
	
	return 0;
}


void gamePlay(int sockfd) {
	char* guessInput = malloc((CODE_LEN+2)*sizeof(char));
	char* guess = malloc((CODE_LEN+1)*sizeof(char));
	char* feedback = malloc((FEEDBACK_LEN+1)*sizeof(char));
	int game_finished, n;

	bzero(feedback, FEEDBACK_LEN+1);
	if ((n=recv(sockfd, feedback, FEEDBACK_LEN+1, 0))<0) {
		perror("ERROR reading from socket");
		exit(0);
	}
	printf("%s\n", feedback);
	bzero(guess, CODE_LEN+1);

	// Repeat until server send a Game-finished flag to client
	while (fgets(guessInput, CODE_LEN+2, stdin)) {

		if (!validate(guessInput)) {
			printf("Please Enter 4 alphabets. Try again: \n");
			continue;
		}

		int i;
		for (i=0; i<CODE_LEN; i++) {
			guess[i] = guessInput[i];
		}
		guess[CODE_LEN] = '\0';

		if ((n = send(sockfd, guess, CODE_LEN+1, 0))<0) {
			perror("ERROR writing to socket");
			exit(0);
		}
	
		bzero(feedback, FEEDBACK_LEN+1);
		if ((n = recv(sockfd, feedback, FEEDBACK_LEN+1, 0))<0) {
			perror("ERROR reading from socket");
			exit(0);
		}

		printf("%s\n", feedback);

		bzero(&game_finished, sizeof(game_finished));
		if ((n=recv(sockfd, &game_finished, sizeof(game_finished), 0))<0) {
			perror("ERROR reading from socket");
			exit(0);
		}

		if (game_finished) {
			break;
		}
		bzero(guess, CODE_LEN+1);
	}

	free(guess);
	free(guessInput);
	free(feedback);
}

/* 
 * Send a 3-alphabet to server to notify the termination of client. This method
 * bases two assumptions: 
 * 1. Ctrl-C can only be pressed while waiting for user input
 * 2. The player will only enter a 4-alphabet combination
 */
void signal_handler() {
	char msg[4]; int n;
	msg[0]='O';
	msg[1]='O';
	msg[2]='O';
	msg[3]='\0';
	if ((n = send(sockfd, msg, CODE_LEN+1, 0))<0) {
		perror("ERROR writing to socket");
		exit(0);
	}
	exit(1);
}

/* 
 * Return TRUE if the input guess is a combination of 4 alphabets, 
 * otherwise FALSE
 */
int validate(char* guessInput) {
	int i;
	for (i=0; i<CODE_LEN; i++) {
		if (!isalpha(guessInput[i])) return FALSE;
	}
	if (guessInput[4]!='\n') return FALSE;
	return TRUE;
}
