#
#  Makefile for 469 Assigment 3 (Fall 2013)

#  Attention: 
#     Please uncomment and modify this file properly so that you 
#     can use this file to make your chatclient.
#


###########################################################################

CC = gcc
CFLAGS = -Wall -g #-DUSE_LOCN_SERVER
SERVER_BIN = chatserver 
SERVER_OBJS = server_util.o server_main.o


CLIENT_BIN = chatclient receiver
CLIENT_OBJS = client_main.o client_util.o 
RECVR_OBJS = client_recv.o client_util.o

all: $(SERVER_BIN) $(CLIENT_BIN)

chatserver: $(SERVER_OBJS) 
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o chatserver

server_util.o: server_util.c server.h defs.h
server_main.o: server_main.c defs.h server.h

chatclient: $(CLIENT_OBJS) 
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o chatclient 

client_util.o: client_util.c client.h defs.h
client_main.o: client_main.c client.h defs.h 

receiver: $(RECVR_OBJS)
	$(CC) $(CFLAGS) $(RECVR_OBJS) -o receiver

clean:
	rm -f *.o $(SERVER_BIN) $(CLIENT_BIN) core *~


###########################################################################
