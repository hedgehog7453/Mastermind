/* COMP30023 Project 2
 * Jiayu Li <jiayul3> 26/5/2016
 * header-client.h
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <ctype.h>


#define CODE_LEN 4
#define FEEDBACK_LEN 90

#define TRUE 1
#define FALSE 0


void gamePlay(int sockfd);
void signal_handler();
int validate(char* guessInput);