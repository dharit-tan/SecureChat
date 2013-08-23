#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

enum {BACKLOG = 128, MAX_BUF_LEN = 256};

/*---------------------------------------------------------------------------------------------------*/

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*---------------------------------------------------------------------------------------------------*/

int createSocket(char *host, char *port) {
	int ret, yes, socketfd;
	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;             // don't care if IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;         // TCP
	if (!host) hints.ai_flags = AI_PASSIVE;  // fill in my IP for me if host is not supplied

	ret = getaddrinfo(host, port, &hints, &servinfo);
	if (ret != 0) { free(servinfo); return -1; }

	/* iterate through returned addrinfos and attempt to create socket, then bind or connect */
	for (p = servinfo; p != NULL; p = p->ai_next) {
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
    socklen_t addrlen;                  // length of connecting client's address

	// select() shit
	int fdmax;
	fd_set master, read_fds;

	// misc
	int i, j, bytes;
	char buf[MAX_BUF_LEN];
    char remoteIP[INET6_ADDRSTRLEN];

	// must specify port
	if (!argv[1]) { printf("usage: reed port\n"); return -1; }

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

	listenerfd = createSocket(NULL, argv[1]); // create socket
	// listen
	if (listen(listenerfd, 10) == -1) { perror("listen"); return -1; }

    FD_SET(listenerfd, &master); // add the listener to the master set
    fdmax = listenerfd; // keep track of the max fd, right now it's listenerfd

	// main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) { perror("select"); return -1; } //int select(int numfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
        
        for (i = 0; i <= fdmax; i++) {
        	if (FD_ISSET(i, &read_fds)) {
        		if (i == listenerfd) {
        			addrlen = sizeof(remoteaddr);

        			// accept new connection
        			newfd = accept(listenerfd, (struct sockaddr *)&remoteaddr, &addrlen);
        			if (newfd == -1) perror("accept");
        			else {
        				FD_SET(newfd, &master);
        				if (newfd > fdmax) fdmax = newfd;

        				 printf("server: new connection from %s on socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                            get_in_addr((struct sockaddr*)&remoteaddr),
                            remoteIP, INET6_ADDRSTRLEN),
                            newfd);
        			}
        		}
        		else {
        			bytes = recv(i, buf, sizeof(buf), 0);
        			if (bytes <= 0) {
	        			if (bytes == 0)      // client closed the connection
	        				printf("selectserver: socket %d hung up\n", i);
	        			else
	        				perror("recv");
	        			close(i);
	        			FD_CLR(i, &master);  // remove this fd from the master set
	        		}
	        		else {
	        			for (j = 0; j <= fdmax; j++) {
	        				if (FD_ISSET(j, &master) && j != i) {
	        					printf("%s", buf);
        					    memset(&buf, 0, sizeof(buf)); /* avoid valgrind error of unitialized variables */
		        				if (j != listenerfd && j != i) {
		        					// bytes = send(j, buf, bytes, 0);
		        					// if (bytes == -1) perror("send");
		        				}
		        			}
	        			} // END sending for loop
	        		} // END recv else
        		} // END listenerfd else
        	} // END read_fds if statement
        } // END going through all the fd's
	} // END main for loop

	return 0;
}