#include "common.h"

#define MAX_GAMES (MAX_PLAYERS/2)

static struct score_board score_board;

static struct {
	int index; /* player index in the array */
	int fd;	   /* file */
} looking_for_match;

static struct {
	int p1; /* player 1 index */
	int p2; /* player 2 index */
} games[MAX_GAMES];

int add_game(int i1, int i2) {
	for (int i = 0; i < MAX_GAMES; i++) {
		if (games[i].p1 == -1 && games[i].p2 == -1) {
			games[i].p1 = i1;
			games[i].p2 = i2;
			
			return (0);
		}
	}
	return (-1);
}

int init_game_board(void) {
	for (int i = 0; i < MAX_GAMES; i++) {
		games[i].p1 = -1;
		games[i].p2 = -1;
	}
	return (0);
}

int game_board_lookup(int index) {
	for (int i = 0; i < MAX_GAMES; i++) {
		if (games[i].p1 == index || games[i].p2 == index) {
			return (i);
		}
	}
	return (-1);
}

void game_board_remove(int index) {
	games[index].p1 = -1;
	games[index].p2 = -1;
}

int init_score_board(void) {
	for (int i = 0; i < MAX_PLAYERS; i++) {
		score_board.scores[i].points = POINTS_NULL;
	}
	score_board.nplayers = 0;
	
	return (0);
}

int score_board_lookup_name(char name[MAX_NAME_LENGHT]) {
	int index = -1;
	
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (score_board.scores[i].points == POINTS_NULL) {
			continue;
		} else if (strcmp(score_board.scores[i].name, name) == 0){
			index = i;
			break;
		}
	}
	return index;
}

int score_board_lookup_ip(const char ip[INET6_ADDRSTRLEN], int port) {
	int index = -1;
	
	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (score_board.scores[i].points == POINTS_NULL) {
			continue;
		} else if (!strcmp(score_board.scores[i].ip, ip) && score_board.scores[i].port == port) {
			index = i;
			break;
		}
	}
	return index;
}

void add_player(
	int fd,
	struct message *request,
	const char ip[INET6_ADDRSTRLEN],
	int port
) {
	struct message response;
	struct player player;
	char name[MAX_NAME_LENGHT];
	int ret;

	strcpy(name, request->body.inscribe.name);
	response.opcode = OP_FAIL;

	if (score_board_lookup_ip(ip, port) >= 0) {
		response.body.reply.error_code = (-ALREADY_REGISTERED);
	} else if (score_board_lookup_name(name) >= 0) {
		response.body.reply.error_code = (-NAME_IN_USE);
	} else {
		for (int i = 0; i < MAX_PLAYERS; i++) {
			if (score_board.scores[i].points == POINTS_NULL) {
				strcpy(score_board.scores[i].name, name);
				score_board.scores[i].points = 0;
				strcpy(score_board.scores[i].ip, ip);
				score_board.scores[i].port = port;
				
				score_board.nplayers++;
				response.opcode = OP_SUCCESS;
				
				printf("New player registered: %s\n", name);
				break;
			}
		}
	}
	send(fd, &response, sizeof(struct message), 0);
}

void match_make(
	int fd,
	const char ip[INET6_ADDRSTRLEN],
	int port
) {
	int index;
	struct message response1, response2;
	struct player player1;
	struct player player2;

	/* player not registered yet */
	if ((index = score_board_lookup_ip(ip, port)) < 0) {
		response1.opcode = OP_FAIL;
		response1.body.reply.error_code = (-NOT_REGISTERED);
		send(fd, &response1, sizeof(struct message), 0);
		return;
	}

	/* player already in game */
	if ((game_board_lookup(index)) >= 0) {
		response1.opcode = OP_FAIL;
		response1.body.reply.error_code = (-ALREADY_IN_GAME);
		send(fd, &response1, sizeof(struct message), 0);
		return;
	}

	/* add player to the "queue" and wait another olayer */
	if (looking_for_match.index == -1) {
		looking_for_match.index = index;
		looking_for_match.fd = fd;
		return;
	} 

	/* save game */
	assert((add_game(looking_for_match.index, index)) == 0);

	player1 = score_board.scores[looking_for_match.index];
	player2 = score_board.scores[index];
	/* build message for player 1 */
	response1.opcode = OP_CONNECT_TO;
	strcpy(response1.body.reply.ip, player2.ip);
	response1.body.reply.port_nr = player2.port;
	send(looking_for_match.fd, &response1, sizeof(struct message), 0);
	/* build message for player 2 */
	response2.opcode = OP_WAIT_FOR_RIVAL;
	send(fd, &response2, sizeof(struct message), 0);

	printf("new match found\n");
	looking_for_match.index = -1;
}

void finish_match(
	int fd,
	struct message *request,
	const char ip[INET6_ADDRSTRLEN],
	int port
) {
	struct message response;
	int points;
	int index_score, index_game;
	int p1, p2;

	response.opcode = OP_FAIL;
	points = request->body.finish.points_to_add;
	if ((index_score = score_board_lookup_ip(ip, port)) < 0) {
		response.body.reply.error_code = (-NOT_REGISTERED);
	} else if ((index_game = game_board_lookup(index_score)) < 0) {
		response.body.reply.error_code = (-NOT_IN_GAME);
	} else {

		if (
			strcmp(score_board.scores[games[index_game].p1].ip, ip) == 0 && 
			score_board.scores[games[index_game].p1].port == port
		) {
			p1 = games[index_game].p1;
			p2 = games[index_game].p2;
		} else {
			p1 = games[index_game].p2;
			p2 = games[index_game].p1;
		}

		switch (points) {
			case 0:
				score_board.scores[p2].points += 3;
				break;
			case 1:
				score_board.scores[p1].points += 1;
				score_board.scores[p2].points += 1;
				break;
			case 3:
				score_board.scores[p1].points += 3;
				break;
			default:
				printf("received invalid points to add\n");
				return;
		}

		game_board_remove(index_game);
		return;
	}
	//send(fd, &response, sizeof(struct message), 0);
}

void get_scores(int fd) {
	struct message response;

	response.opcode = OP_SUCCESS;
	response.body.reply.score_board = score_board;

	send(fd, &response, sizeof(struct message), 0);
}

void are_player_connected(int fd, const char ip[INET6_ADDRSTRLEN], int port) {
	int ret;
	struct message response;

	response.opcode = (score_board_lookup_ip(ip,port)<0) ? OP_FAIL : OP_SUCCESS;

	send(fd, &response, sizeof(struct message), 0);
}

int startserver(void) {
	int sd = -1, on = 1;
	struct sockaddr_in6 serveraddr;
	
	sd = socket(AF_INET6, SOCK_STREAM, 0);
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on,sizeof(on));

	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin6_family = AF_INET6;
	serveraddr.sin6_port   = htons(PORT_NR);
	serveraddr.sin6_addr   = in6addr_any;

	bind(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));

	listen(sd, POLL_LENGHT);

	return sd;
}

void add_connection(struct pollfd *fds, int newsock, int *nfds) {
	for (int i = 0; i < POLL_LENGHT; i++) {
		if (fds[i].fd != 0) {
			continue;
		}
		fds[i].fd = newsock;
		fds[i].events = POLLIN;
		(*nfds)++;
		break;
	}
}

void remove_connection(struct pollfd *fds, int i, int *nfds) {
	fds[i].fd = 0;
	(*nfds)--;
}

void handle_pollin(struct pollfd *fds, int i, int server_fd, int *nfds) {
	struct sockaddr_in6 addr;
	struct player player;
	struct message request;
	struct message response;
	int socklen, newsock, bytes, ret;
	long double op1, op2, result;
	char op, str[INET6_ADDRSTRLEN];

	/* new connection */
	if (fds[i].fd == server_fd) {
		socklen = sizeof(addr);

		newsock = accept(server_fd, NULL, NULL);

		getpeername(newsock, (struct sockaddr *)&addr, &socklen);
		inet_ntop(AF_INET6, &addr.sin6_addr, str, sizeof(str));
		
		if (*nfds == MAX_PLAYERS) {
			printf("new connection rejected %s %d\n", str, ntohs(addr.sin6_port));
			close(newsock);
		} else {
			printf("new connection! %s %d\n", str, ntohs(addr.sin6_port));
			add_connection(fds, newsock, nfds);
		}	
		
		fflush(stdout);
		
		return;
	}

	/* operation request */
	int p = recv(fds[i].fd, &request, sizeof(struct message), 0);
	if (p > 0) {
		getpeername(fds[i].fd, (struct sockaddr *)&addr, &socklen);
		switch (request.opcode) {
			case OP_FIND_MATCH:
				match_make(
					fds[i].fd,
					inet_ntop(AF_INET6, &addr.sin6_addr, str, sizeof(str)),
					ntohs(addr.sin6_port)
				);
				break;
			case OP_SCORE:
				get_scores(fds[i].fd);
				break;
			case OP_FINISHED_MATCH:
				finish_match(
					fds[i].fd,
					&request,
					inet_ntop(AF_INET6, &addr.sin6_addr, str, sizeof(str)),
					ntohs(addr.sin6_port)
				);
				break;
			case OP_REGISTER:
				add_player(
					fds[i].fd,
					&request,
					inet_ntop(AF_INET6, &addr.sin6_addr, str, sizeof(str)),
					ntohs(addr.sin6_port)
				);
				break;
			case OP_AMICONNECTED:
				are_player_connected(
					fds[i].fd,
					inet_ntop(AF_INET6, &addr.sin6_addr, str, sizeof(str)),
					ntohs(addr.sin6_port)
				);
				break;
			default:
				printf("Unespected operation!\n");
		}
		
	} else {
		close(fds[i].fd);
		remove_connection(fds, i, nfds);
		printf("client disconnected\n");
	}
}

void init(void) {
	init_game_board();
	init_score_board();
	looking_for_match.index = -1;
}

int main() {
	int server_fd;
	int nfds;
	struct pollfd fds[POLL_LENGHT] = {{.fd = 0, .events = POLLIN}};
	
	init();
	server_fd = startserver();
	fds[0].fd = server_fd;
	nfds = 1;
	printf("listening\n");

	while (1) {
		poll(fds, nfds, -1);
		
		for (int i = 0; i < nfds; i++) {
			switch (fds[i].revents) {
                case 0:
                    break;
                case POLLIN:
                    handle_pollin(fds, i, server_fd, &nfds); 
                    break;
                case POLLNVAL:
                case POLLPRI:
                case POLLOUT:
                case POLLERR:
                case POLLHUP:
                case POLLRDBAND:
                case POLLWRBAND:
                default:
                    printf("Unespected revents\n");
                    close(fds[i].fd);
					remove_connection(fds, i, &nfds);
            }
		}
	}

	exit(0);
}
