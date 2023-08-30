#ifndef COMMON_H
#define COMMON_H

	#include <sys/types.h>
	#include <sys/socket.h>
	#include <stdio.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <stdlib.h>
	#include <string.h>
	#include <poll.h>
	#include <assert.h>

	/* Requests */
	#define OP_FIND_MATCH          0 /* Looking for a match */
	#define OP_YOUR_TURN           1 /* Player's turn */
	#define OP_SCORE               2 /* What's my score? */
	#define OP_FINISHED_MATCH      3 /* Server, I won the game */
	#define OP_REGISTER            4 /* Register my name */
	#define OP_DISCONNECT          5 
	#define OP_AMICONNECTED        6
	/* Replies */
	#define OP_CONNECT_TO      7 /* Found match. Connect to the other player */
	#define OP_WAIT_FOR_RIVAL  8
	#define OP_SUCCESS         9
	#define OP_FAIL            10

	/* error codes */
	#define NAME_IN_USE        1
	#define ALREADY_REGISTERED 2
	#define NOT_REGISTERED     3
	#define ALREADY_IN_GAME    4
	#define NOT_IN_GAME        5

	#define MAX_NAME_LENGHT 30
	#define MAX_PLAYERS     50

	#define PORT_NR 8080
	#define POLL_LENGHT (MAX_PLAYERS + 1)
	#define POINTS_NULL (-1)

	#if ((MAX_PLAYERS % 2) != 0)
	#error "bad max players"
	#endif


	struct player
	{
		char name[MAX_NAME_LENGHT];
		int points;
		char ip[INET6_ADDRSTRLEN];
		int port;
	};

	struct score_board
	{
		struct player scores[MAX_PLAYERS];
		int nplayers;
	};

	struct message
	{
		int opcode;

		union
		{
			struct
			{
				int points_to_add;
			} finish;	

			struct
			{
				char name[MAX_NAME_LENGHT];
			} inscribe;

			struct
			{
				int col;
				int row;
				int value;
				int won;
			} nextround;

			struct
			{
				struct score_board score_board;
				char ip[INET6_ADDRSTRLEN];
				int port_nr;
				int error_code;
			} reply;
		} body;
	};

#endif /* COMMON_H */