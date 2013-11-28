/*
 *   CSC469 Fall 2010 A3
 *  
 *      File:      client.h 
 *      Author:    Angela Demke Brown
 *      Version:   1.0.0
 *      Date:      17/11/2010
 *   
 * Please report bugs/comments to demke@cs.toronto.edu
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <netdb.h>

#include "client.h"

 #include <errno.h>
 #include <ctype.h>


static void build_req(char *buf)
{
	/* Write an HTTP GET request for the chatserver.txt file into buf */

	int nextpos;

	sprintf(buf,"GET /~csc469h/winter/chatserver.txt HTTP/1.0\r\n");

	nextpos = strlen(buf);
	sprintf(&buf[nextpos],"\r\n");
}

static char *skip_http_headers(char *buf)
{
	/* Given a pointer to a buffer which contains an HTTP reply,
	 * skip lines until we find a blank, and then return a pointer
	 * to the start of the next line, which is the reply body.
	 * 
	 * DO NOT call this function if buf does not contain an HTTP
	 * reply message.  The termination condition on the while loop 
	 * is ill-defined for arbitrary character arrays, and may lead 
	 * to bad things(TM). 
	 *
	 * Feel free to improve on this.
	 */

	char *curpos;
	int n;
	char line[256];

	curpos = buf;

	while ( sscanf(curpos,"%256[^\n]%n",line,&n) > 0) {
		if (strlen(line) == 1) { /* Just the \r was consumed */
			/* Found our blank */
			curpos += n+1; /* skip line and \n at end */
			break;
		}
		curpos += n+1;
	}

	return curpos;
}

int connection(int type, char *name, int port){

	int server_socket_fd;
	struct hostent *hp;
	struct sockaddr_in server_addr; 

	hp = gethostbyname(name); 

    if ( hp == NULL ) 
    {  
		fprintf(stderr, "host error\n");
		exit(1);
    }

    /* create socket */
    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    int optval = 1;
    setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int));
    
    memset((char *)&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;

	memcpy((char *)&server_addr.sin_addr.s_addr, (char *)hp->h_addr,  hp->h_length);

	/*Assign the port*/
	server_addr.sin_port = htons((unsigned short)port);

    /* request connection to server */
    if (connect(server_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {  
		printf("connection to server refused\n");
		return -1; 
    }    	


	return server_socket_fd;


}


int retrieve_chatserver_info(char *chatserver_name, u_int16_t *tcp_port, u_int16_t *udp_port)
{
	int locn_socket_fd;
	char *buf;
	int buflen;
	int code;
	int  n;

	/* Initialize locnserver_addr. 
	 * We use a text file at a web server for location info
	 * so this is just contacting the CDF web server 
	 */

	/* 
	 * 1. Set up TCP connection to web server "www.cdf.toronto.edu", 
	 *    port 80 
	 */

	/**** YOUR CODE HERE ***/

	 locn_socket_fd = connection(SOCK_STREAM, "www.cdf.toronto.edu", 80);

	/* The code you write should initialize locn_socket_fd so that
	 * it is valid for the write() in the next step.
	 */

	/* 2. write HTTP GET request to socket */

	buf = (char *)malloc(MAX_MSG_LEN);
	bzero(buf, MAX_MSG_LEN);
	build_req(buf);
	buflen = strlen(buf);

	write(locn_socket_fd, buf, buflen);

	/* 3. Read reply from web server */

	read(locn_socket_fd, buf, MAX_MSG_LEN);

	/* 
	 * 4. Check if request succeeded.  If so, skip headers and initialize
	 *    server parameters with body of message.  If not, print the 
	 *    STATUS-CODE and STATUS-TEXT and return -1.
	 */

	/* Ignore version, read STATUS-CODE into variable 'code' , and record
	 * the number of characters scanned from buf into variable 'n'
	 */
	sscanf(buf, "%*s %d%n", &code, &n);

	/**** YOUR CODE HERE ****/
	if (code == 200){

		char host_name[MAX_HOST_NAME_LEN];
		char tcp[10];
		char udp[10];

		char *str =  skip_http_headers(buf);
  		char *p;
  		int i;
  		int j = 0;
  		p = str;

  		for(i = 0; !isblank(p[ i ]); i++){
    		host_name[j] =  p[ i ];
    		j++;
    	}

    	host_name[j] = '\0';
    	j=0;

    	i++;
    	for(;!isblank(p[ i ]); i++){
    		tcp[j] =  p[ i ];
    		j++;
    	}
    	tcp[j] = '\0';
    	j=0;

    	i++;
    	/*needs to be enabled for additional lines in location server file otherwise will crash*/
    	for(;p[ i ] != NULL; i++){
    		udp[j] =  p[ i ];
    		j++;
    	}
    	udp[j] = '\0';

    	strcpy(chatserver_name, host_name);
    	*tcp_port = atoi(tcp);
    	*udp_port = atoi(udp);



    	free(buf);
		return 0;
	}

	/* 5. Clean up after ourselves and return. */

	free(buf);
	return code;

}
