/* COMP30023 Project 2
 * Jiayu Li <jiayul3> 26/5/2016
 * header-server.h
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <unistd.h>


#define CODE_LEN 4
#define MAX_NUM_ATTEMPTS 10
#define FEEDBACK_LEN 90
#define ENTRY_LEN 200
#define STATUS_LEN 7
#define NUM_SOCKETS 20

#define TRUE 1
#define FALSE 0

#define INVALID -1
#define SUCCESS 1
#define FAILURE 0


typedef struct arguments {
	int sockfd;
	char* inputcode;
	char* cli_ip;
} args_t;


void* gamePlay(void* args);
char* generateCode();
int validateGuess(char* code, char* guess);
char* generateFeedback(char* code, char* guess);
char* getTime();
char* getIP(struct sockaddr_in cli_addr);
int getAvailSockIndex(int* sockfds);
int getSockIndex(int* sockfds, int sockfd);
void removeSock(int sockfd);
void writetofile(char* content);
void signal_handler();
