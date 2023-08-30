Compilation:
gcc -o server server.c
gcc -o client client.c

For execution, the server and each of the clients must be run in separate terminals:
./server
./client

Inside the server, there are no functions to be written by the user, only within the client.
These are:

register
scores
match
exit

All functions should be entered alone and then the Enter key should be pressed. If they require additional input, it should be added on the next line, as is the case with 'register'.

Writing example:
register
Samuel

NOTE: In line 3 of 'client.c', the macro definition SERVER_IPV6 must be completed to correspond to the IPv6 address of the machine where the server is being executed.
