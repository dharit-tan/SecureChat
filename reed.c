#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

enum {BACKLOG = 128};

/*---------------------------------------------------------------------------------------------------*/

int createSocket(char *host, char *port) {
	int ret, yes, socketfd;
	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;             // don't care if IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;         // TCP
	if (!host) hints.ai_flags = AI_PASSIVE;  // fill in my IP for me if host is not supplied

	ret = getaddrinfo(host, port, &hints, &servinfo);
	if (ret != 0) free(servinfo); return -1;

	/* iterate through returned addrinfos and attempt to create socket, then bind or connect */
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		// create socket
		socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (socketfd == -1) continue;

		// set socket to be reusable
		yes = 1;
		ret = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		if (ret < 0) { close(socketfd); freeaddrinfo(servinfo); continue; }

		ret = bind(socketfd, p->ai_addr, p->ai_addrlen);
		if (ret < 0) continue;

		break;
	}

	// if p is NULL then we didn't get bound
	if (!p) exit(EXIT_FAILURE);
	freeaddrinfo(servinfo);
	return socketfd;
}

/*---------------------------------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
	// socket fd shit
	int listenerfd, newfd;
	struct sockaddr_storage remoteaddr; // connecting client's address
    socklen_t addrlen;

	// select() shit
	int fdmax;
	fd_set master, read_fds;

	// misc
	int i;

	// must specify port
	if (!argv[1]) { printf("usage: reed port\n"); return 1; }

	listenerfd = createSocket(NULL, argv[1]); // create socket

	// listen
	if (listen(listenerfd, 10) == -1) { perror("listen"); exit(3); }

    FD_SET(listenerfd, &master); // add the listener to the master set
    fdmax = listenerfd; // keep track of the max fd, right now it's listenerfd

	// main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) { perror("select"); exit(4); } //int select(int numfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

        for (i = 0; i < fdmax; i++) {
        	if FD_ISSET(i, &read_fds) {
        		if (i == listenerfd) {
        			remoteaddr = sizeof()
        		}
        	}
        }









}