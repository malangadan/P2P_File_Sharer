/*///////////////////////////////////////////////////////////
*
* FILE:		client.c
* AUTHOR:	Your Name Here
* PROJECT:	CNT 4007 Project 1 - Professor Traynor
* DESCRIPTION:	Network Client Code
*
*////////////////////////////////////////////////////////////

/* Included libraries */

#include <stdio.h>		    /* for printf() and fprintf() */
#include <sys/socket.h>		    /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>		    /* for sockaddr_in and inet_addr() */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <openssl/evp.h>	    /* for OpenSSL EVP digest libraries/SHA256 */

/* Constants */
#define RCVBUFSIZE 512		    /* The receive buffer size */
#define SNDBUFSIZE 512		    /* The send buffer size */
#define MDLEN 32

void fatal_error(const char *message) {
    perror(message);
    exit(1);
}

/* The main function */
int main(int argc, char *argv[])
{

    int clientSock;		    /* socket descriptor */
    struct sockaddr_in serv_addr;   /* The server address */

    char *studentName;		    /* Your Name */

    char sndBuf[SNDBUFSIZE];	    /* Send Buffer */
    char rcvBuf[RCVBUFSIZE];	    /* Receive Buffer */
    
    int i;			    /* Counter Value */

    /* Get the Student Name from the command line */
    if (argc != 2) 
    {
	printf("Incorrect input format. The correct format is:\n\tnameChanger your_name\n");
	exit(1);
    }
    studentName = argv[1];
    memset(&sndBuf, 0, SNDBUFSIZE);
    memset(&rcvBuf, 0, RCVBUFSIZE);

    /* Create a new TCP socket*/
    /*	    FILL IN	*/
    if ((clientSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        fatal_error("Socket Failed");
    }
    /* Construct the server address structure */
    /*	    FILL IN	 */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8080);

    /* Establish connecction to the server */
    /*	    FILL IN	 */
    if (connect(clientSock,(struct sockaddr *)& serv_addr, sizeof(serv_addr)) < 0) {
        fatal_error("connect() failed");
    }
    

    /* Send the string to the server */
    /*	    FILL IN	 */
    int msglen = strlen(studentName);
    if (send(clientSock,studentName,msglen, 0) != msglen){
        fatal_error("send() sent unexpected number of bytes");
    }
    /* Receive and print response from the server */
    /*	    FILL IN	 */
    int returnBytes = recv(clientSock,rcvBuf,RCVBUFSIZE - 1,0);
    
    if (returnBytes < 0){
        fatal_error("less than 0 bytes from recv()");
    }

    printf("%s\n", studentName);
    printf("Transformed input is: ");
    for(i = 0; i < MDLEN; i++) printf("%02x", rcvBuf[i]);
    printf("\n");

    close(clientSock);
    return 0;
}
