/*
 *   CSC469 Fall 2013 A3
 *  
 *      File:      client_recv.c 
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
#include <netdb.h>

#include "client.h"

static char *option_string = "f:";

/* For communication with chat client control process */
int ctrl2rcvr_qid;
char ctrl2rcvr_fname[MAX_FILE_NAME_LEN];
/* Global variables for remote UDP server address and our UDP port number */
struct sockaddr_in server_addr;
int socket_fd;
/* For control of socket timeout/ main receiver loop speed */
int TIMEOUT = 1;

void usage(char **argv) {
	printf("usage:\n");
	printf("%s -f <msg queue file name>\n",argv[0]);
	exit(1);
}


void open_client_channel(int *qid) {

	/* Get messsage channel */
	key_t key = ftok(ctrl2rcvr_fname, 42);

	if ((*qid = msgget(key, 0400)) < 0) {
		perror("open_channel - msgget failed");
		fprintf(stderr,"for message channel ./msg_channel\n");

		/* No way to tell parent about our troubles, unless/until it 
		 * wait's for us.  Quit now.
		 */
		exit(1);
	}

	return;
}

void send_error(int qid, u_int16_t code)
{
	/* Send an error result over the message channel to client control process */
	msg_t msg;

	msg.mtype = CTRL_TYPE;
	msg.body.status = RECV_NOTREADY;
	msg.body.value = code;

	if (msgsnd(qid, &msg, sizeof(struct body_s), 0) < 0) {
		perror("send_error msgsnd");
	}
							 
}

void send_ok(int qid, u_int16_t port)
{
	/* Send "success" result over the message channel to client control process */
	msg_t msg;

	msg.mtype = CTRL_TYPE;
	msg.body.status = RECV_READY;
	msg.body.value = port;

	if (msgsnd(qid, &msg, sizeof(struct body_s), 0) < 0) {
		perror("send_ok msgsnd");
	} 

}
/*!!!!!!!! port and host are hardcoded for now*/

void init_receiver()
{

	/* 1. Make sure we can talk to parent (client control process) */
	printf("Trying to open client channel\n");

	open_client_channel(&ctrl2rcvr_qid);

	/**** YOUR CODE TO IMPLEMENT STEPS 2 AND 3 ****/

	/* 2. Initialize UDP socket for receiving chat messages. */

	socklen_t server_addr_len;

	server_addr_len = sizeof(server_addr);

	if( (socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		send_error(ctrl2rcvr_qid, SOCKET_FAILED);
		exit(1);
	}

    int optval = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int));

          struct timeval tv;
  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
      perror("Error");
  }

	memset(&server_addr, 0, server_addr_len);
	server_addr.sin_family = AF_INET;

	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);


		server_addr.sin_port = 0;

		if( bind(socket_fd, (struct sockaddr *)&server_addr,
			 server_addr_len) < 0 ) {
			send_error(ctrl2rcvr_qid, BIND_FAILED);
			exit(1);
		}	

		if( getsockname(socket_fd, (struct sockaddr *)&server_addr,
				&server_addr_len) < 0 ) {
			send_error(ctrl2rcvr_qid, NAME_FAILED);
			exit(1);
		}

    

	/* server is created successfully */

	send_ok(ctrl2rcvr_qid, ntohs(server_addr.sin_port));
	printf("Client channel open\n");

	/* 3. Tell parent the port number if successful, or failure code if not. 
	 *    Use the send_error and send_ok functions
	 */

}




/* Function to deal with a single message from the chat server */

void handle_received_msg(char *buf)
{

	/**** YOUR CODE HERE ****/

	struct chat_msghdr *cmh;

	cmh = (struct chat_msghdr *)buf;

	printf("%s: ", cmh->sender.member_name);

	printf("%s", (char *)cmh->msgdata);


}



/* Main function to receive and deal with messages from chat server
 * and client control process.  
 *
 * You may wish to refer to server_main.c for an example of the main 
 * server loop that receives messages, but remember that the client 
 * receiver will be receiving (1) connection-less UDP messages from the 
 * chat server and (2) IPC messages on the from the client control process
 * which cannot be handled with the same select()/FD_ISSET strategy used 
 * for file or socket fd's.
 */
void receive_msgs()
{
	char *buf = (char *)malloc(MAX_MSG_LEN);
  
	if (buf == 0) {
		printf("Could not malloc memory for message buffer\n");
		exit(1);
	}


	/**** YOUR CODE HERE ****/
	
	int n;
	socklen_t server_addr_len = sizeof(server_addr);

	while(TRUE) {
		/**** YOUR CODE HERE ****/

		memset(buf, 0, MAX_MSG_LEN);

		int result;
		msg_t msg;
		
		result = msgrcv(ctrl2rcvr_qid, &msg, sizeof(struct body_s), RECV_TYPE, IPC_NOWAIT);
		if (result > 0) {
			if (msg.body.status == CHAT_QUIT) {
				exit(1);

			} 
		}
		
		n = recvfrom(socket_fd, buf, MAX_MSG_LEN, 0, (struct sockaddr *)&server_addr, &server_addr_len);

		if (n > 0){
			handle_received_msg(buf);
		}
	}

	/* Cleanup */
	free(buf);
	return;
}


int main(int argc, char **argv) {
	char option;

	printf("RECEIVER alive: parsing options! (argc = %d\n",argc);

	while((option = getopt(argc, argv, option_string)) != -1) {
		switch(option) {
		case 'f':
			strncpy(ctrl2rcvr_fname, optarg, MAX_FILE_NAME_LEN);
			break;
		default:
			printf("invalid option %c\n",option);
			usage(argv);
			break;
		}
	}

	if(strlen(ctrl2rcvr_fname) == 0) {
		usage(argv);
	}

	printf("Receiver options ok... initializing\n");

	init_receiver();

	receive_msgs();

	return 0;
}
