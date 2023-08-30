#include "common.h"

#define SERVER_IPV6 "YOUR_IP"

static const char VALUES[2] = {'X', 'O'};

int print_tictac(char matriz[3][3]) {

    printf("     |     |     \n");
    printf("  %c  |  %c  |  %c \n", matriz[0][0], matriz[0][1], matriz[0][2]);

    printf("_____|_____|_____\n");
    printf("     |     |     \n");

    printf("  %c  |  %c  |  %c \n", matriz[1][0], matriz[1][1], matriz[1][2]);

    printf("_____|_____|_____\n");
    printf("     |     |     \n");

    printf("  %c  |  %c  |  %c \n", matriz[2][0], matriz[2][1], matriz[2][2]);

    printf("     |     |     \n\n");

    return 1;
}

void set_matriz(char matriz[3][3], int line, int col, char value) {
    matriz[line][col] = value;
}

int reset_matriz(char matriz[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            matriz[i][j] = ' ';
        }
    }
}

int verify_diagonals(char matriz[3][3]) {
    for (int i = 0; i < 2; i++) {
        if ((matriz[0][0] == VALUES[i] && matriz[1][1] == VALUES[i] &&
        matriz[2][2] == VALUES[i])
        || (matriz[0][2] == VALUES[i] &&
        matriz[1][1] == VALUES[i] && matriz[2][0] == VALUES[i])) {
            return 1;
        }
    }

    return 0;
}

int verify_column(char matriz[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            if (matriz[i][0] == VALUES[j] &&
            matriz[i][1] == VALUES[j] && matriz[i][2] == VALUES[j]) {
                return 1;
            }
        }
    }

    return 0;
}

int verify_line(char matriz[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            if (matriz[i][0] == VALUES[j] &&
            matriz[i][1] == VALUES[j] && matriz[i][2] == VALUES[j]) {
                return 1;
            }
        }
    }

    return 0;
}

int ifFull(char matriz[3][3]) {
    for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < 3; j++) {
            if (matriz[i][j] == ' ') {
                return 0;
            }
        }
    }
    
    return 1;
}

int verify_tictac(char matriz[3][3], int line, int col, char value) {
    if (matriz[line][col] == ' ') {
        matriz[line][col] = value;

        int l = verify_line(matriz);
        int c = verify_column(matriz);
        int d = verify_diagonals(matriz);

        if (l || c || d) {
            return 1; //WIN
        }

        if (ifFull(matriz)) {
            return 2;
        }

        return 0; //VALID VALUE
    }

    return -1; // INVALID VALUE
}

void send_next_round(int fd, char matriz[3][3], int player_value, int server_fd) {
    int line, col, won;
    struct message request, final_request;

    while (1) {
		line = -1;
		col = -1;
        printf("choose a line: (1-3)\n");
        scanf("%d", &line);
        if (line < 1 || line > 3) {
			printf("invalid line.\n");
            continue;
        }
        printf("choose one column: (1-3)\n");
		scanf("%d", &col);
		if (col < 1 || col > 3) {
			printf("invalid column\n");
            continue;
        }
        break;
    }
    --line, --col;
    
    won = verify_tictac(matriz, line, col, VALUES[player_value]);

    print_tictac(matriz);

    if (won == -1) {
        printf("line and column already in use.\n");
        send_next_round(fd, matriz, player_value, server_fd);
    } else {
        request.body.nextround.col = col;
        request.body.nextround.row = line;
        request.body.nextround.value = VALUES[player_value];
        request.body.nextround.won = won;

        send(fd, &request, sizeof(struct message), 0);

        if (won == 2 || won == 1) {            
            final_request.opcode = OP_FINISHED_MATCH;
            final_request.body.finish.points_to_add = 1;
            if (won == 1) {
                final_request.body.finish.points_to_add = 3;
                printf("Win");
            } else {
                printf("Draw");
            }

            send(server_fd, &final_request, sizeof(struct message), 0);
        }
    }
}

void match(int fd, int init, int server_fd) {
    char matriz[3][3];
	char (*prt)[3];
	prt = matriz;

    struct message request;

    reset_matriz(prt);

    printf("welcome!\n");
    printf("good luck!\n");
    printf("\n");
    print_tictac(matriz);

    if (init) {
        send_next_round(fd, matriz, init, server_fd);
    } 
    while (1) {
        recv(fd, &request, sizeof(struct message), 0);

        if (request.body.nextround.won == 2 ||
        request.body.nextround.won == 1) {
            send(fd, &request, sizeof(struct message), 0);
            close(fd);

            break;
        }

        int line = request.body.nextround.row;
        int col = request.body.nextround.col;
        char value = request.body.nextround.value;

        set_matriz(prt, line, col, value);

        print_tictac(prt);

        send_next_round(fd, matriz, init, server_fd);
    }
}

void wait_for_rival(int server_fd) {
    int new_socket, sd = -1, on = 1;
    struct sockaddr_in6 serveraddr;
    
    sd = socket(AF_INET6, SOCK_STREAM, 0);
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

    memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin6_family = AF_INET6;
	serveraddr.sin6_port   = htons(4000);
	serveraddr.sin6_addr   = in6addr_any;

	bind(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));

	listen(sd, 1);

    printf("waiting rival connection\n");

    new_socket = accept(sd, NULL, NULL);
    printf("connection accepted. Wait for your turn\n");

    fflush(stdout);
    match(new_socket, 1, server_fd);
    close(sd);
}

void connect_to_rival(char ip[INET6_ADDRSTRLEN], int port, int server_fd) {
	int ret, rival_fd, ntrials;
	struct sockaddr_in6 addr;
    struct timeval timeout;

	ntrials = 10;
	
	rival_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(4000);
	inet_pton(AF_INET6, ip, &addr.sin6_addr);

    while((ret = connect(rival_fd, (struct sockaddr *) &addr, sizeof(addr))) < 0);

	match(rival_fd, 0, server_fd);
    close(rival_fd);
}

int starclient(void) {
    int ret, fd;
	struct sockaddr_in6 addr;
	
	fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(PORT_NR);
	inet_pton(AF_INET6, SERVER_IPV6, &addr.sin6_addr);


    //while((ret = connect(fd, (struct sockaddr *) &addr, sizeof(addr))) < 0);
	if ((ret = connect(fd, (struct sockaddr *) &addr, sizeof(addr))) < 0) {
		perror("unable to connect");
		exit(1);
	}

	return (fd);
}

void request_match(int fd) {
    int error, sucess, again, port;
    struct message request;
    char ip[INET6_ADDRSTRLEN];

    request.opcode = OP_FIND_MATCH;
    send(fd, &request, sizeof(struct message), 0);
    recv(fd, &request, sizeof(struct message), 0);

    if (request.opcode == OP_FAIL) {
        error = -request.body.reply.error_code;
        switch(error) {
            case ALREADY_IN_GAME:
                printf("you are alreary in a match\n");
				break;
            case NOT_REGISTERED:
                printf("You need to register your user\n");
				break;
            default:
                printf("unexpected error when registering name\n");
        }
    } else {
        sucess = request.opcode;
        printf("%d\n", sucess);
        switch(sucess) {
            case OP_WAIT_FOR_RIVAL:
                wait_for_rival(fd);
                break;
            case OP_CONNECT_TO:
				port = request.body.reply.port_nr;
				strcpy(ip, request.body.reply.ip);
				connect_to_rival(ip, port, fd);
                break;
            default:
                printf("unexpected opcode\n");
		}
    }
}

void inscribe(int fd) {
	int error, again;
	struct message request;
    char name[MAX_NAME_LENGHT];

	request.opcode = OP_AMICONNECTED;
	send(fd, &request, sizeof(request), 0);
	recv(fd, &request, sizeof(request), 0);
	
	if (request.opcode == OP_SUCCESS) {
		printf("you are already registered and can request a match\n");
		return;
	}

	again = 1;
	printf("you are not registered. Type your name\n");
	while (again) {
		scanf("%s", name);

		request.opcode = OP_REGISTER;
		strcpy(request.body.inscribe.name, name);

		send(fd, &request, sizeof(request), 0);
		recv(fd, &request, sizeof(request), 0);

		if (request.opcode == OP_FAIL) {
			error = -request.body.reply.error_code;
			switch (error) {
				case NAME_IN_USE:
					printf("name already in use. Try again\n");
					break;
				case ALREADY_REGISTERED:
					printf("you are already registered\n");
					break;
				default:
					printf("unexpected error when registering name\n");
			}
		} else {
            printf("succesfully registered\n");
			again = 0;
		}
	}
}

void display_scores(int fd) {
	struct message request;
	struct score_board board;

	request.opcode = OP_SCORE;
	send(fd, &request, sizeof(struct message), 0);
	recv(fd, &request, sizeof(struct message), 0);

	memcpy(&board, &request.body.reply.score_board, sizeof(struct score_board));
	for (int i = 0; i < MAX_PLAYERS; i++) {
        if (board.scores[i].points == POINTS_NULL)
            continue;
		printf("%s: %d\n", board.scores[i].name, board.scores[i].points);
	}
}


int mainloop(int fd) {
	char command[20];

	printf("Welcome!\n\ncommands:\nregister\nmatch\nscores\n");
    while (1) {	
		scanf("%s", command);

		if (!strcmp(command, "register")) {
			inscribe(fd);
		} else if (!strcmp(command, "match")) {
			request_match(fd);
		} else if (!strcmp(command, "scores")) {
			display_scores(fd);
		} else if (!strcmp(command, "exit")) {
            close(fd);
            break;
        } else {
			printf("invalid operation\n");
		}
    }    
}

int main() {
    int fd;

    if ((fd = starclient()) < 0) {
        printf("error connecting to server\n");
        exit(1);
    }

    mainloop(fd);
}