/*
 * Adapted by demke for CSC469 A3 from:
 *
 *   15-213 Spring 2000 L5 (designed based on L5 of Fall 1999) 
 *  
 *      File:      defs.h 
 *      Author:    Jun Gao
 *      Version:   1.0.0
 *      Date:      4/12/2000
 *   
 * Please report bugs/comments to demke@cs.toronto.edu
 *
 * This file must be included in your client implementation!
 * This file must not be modified by students!
 */

#ifndef _DEFS_H
#define _DEFS_H

#define REGISTER_REQUEST 	1
#define REGISTER_SUCC	 	2
#define REGISTER_FAIL	 	3

#define ROOM_LIST_REQUEST	4 
#define ROOM_LIST_SUCC 	        5 
#define ROOM_LIST_FAIL	        6

#define MEMBER_LIST_REQUEST	7
#define MEMBER_LIST_SUCC	8
#define MEMBER_LIST_FAIL        9

#define SWITCH_ROOM_REQUEST	10
#define SWITCH_ROOM_SUCC	11
#define SWITCH_ROOM_FAIL 	12	

#define CREATE_ROOM_REQUEST	13
#define CREATE_ROOM_SUCC	14
#define CREATE_ROOM_FAIL	15

#define MEMBER_KEEP_ALIVE	16

#define QUIT_REQUEST		17	

/* maximum length of a member name, including null terminator */
#define MAX_MEMBER_NAME_LEN     30	

/* maximum length of a host name */
#define MAX_HOST_NAME_LEN	80

/* maximum length of a file name */
#define MAX_FILE_NAME_LEN	80

/* maximum length of a room name, including null terminator */
#define MAX_ROOM_NAME_LEN       30 

/* maximum number of rooms in one session */
#define MAX_NUM_OF_ROOMS	40 

/* maximum number of members in one room */
#define MAX_NUM_OF_MEMBERS_PER_ROOM  30

#define MAX_NUM_OF_MEMBERS    (MAX_NUM_OF_ROOMS) * (MAX_NUM_OF_MEMBERS_PER_ROOM)

/* maximum length of a message */
#define MAX_MSG_LEN		2048	


/* data structures */

/* common control message header */

struct control_msghdr {
    u_int16_t msg_type;
    u_int16_t member_id;
    u_int16_t msg_len;
    u_int16_t reserved;
    caddr_t   msgdata[0];
};

/* chat message header */

struct chat_msghdr {
    union {
    	char member_name[MAX_MEMBER_NAME_LEN];
	u_int16_t member_id;
    }sender;
    u_int16_t msg_len;
    caddr_t msgdata[0];
};

/* REGISTER_REQUEST message data definition */

struct register_msgdata {
    u_int16_t udp_port;
    caddr_t member_name[0];
};

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif
