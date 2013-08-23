/* beej's chat server code */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char** argv) {
	char *port;
	fd_set master; // master fd list
	fd_set read_fds; // temp fd list for select
	int fdmax; // max fd number

	int listener;
	int newfd;
	struct sockaddr_storage remoteaddr; // client addr
	socklen_t addrlen;

	char buf[256]; // buffer for client data
	int nbytes;

	char remoteIP[INET6_ADDRSTRLEN];

	int yes=1;
	int i, j, rv;

	struct addrinfo hints, *ai, *p;
	
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <port-number>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	port = argv[1];

	FD_ZERO(&master); // clear the master and temp sets
	FD_ZERO(&read_fds);

	// get and bind socket
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}

	for (p = ai; p != NULL; p = p->ai_next) {
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) {
			continue;
		}
		// lose "addr in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}
		
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}

	freeaddrinfo(ai); // all done with this

	// listen
	if (listen(listener, 10) == -1) {
		perror("listen");
		exit(3);
	}

	// add the listener to the master set
	FD_SET(listener, &master);

	fdmax = listener; // biggest fd so far

	// main loop
	while(1) {
		read_fds = master;
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}

		// run through existing connections looking for data to read
		for(i=0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == listener) {
					// handle new connections
					addrlen = sizeof(remoteaddr);
					newfd = accept(listener, 
							(struct sockaddr *)&remoteaddr, 
							&addrlen);
					if (newfd == -1) {
						perror("accept");
					}

					else {
						FD_SET(newfd, &master); // add to master set
						if (newfd > fdmax) {
							fdmax = newfd;
						}
						printf("server: new connection from %s on "
								"socket %d\n",
								inet_ntop(remoteaddr.ss_family,
									get_in_addr((struct sockaddr*)&remoteaddr),
									remoteIP, INET6_ADDRSTRLEN),
								newfd);
					}
				}
				else { // handle client data
					// could add extra fd for stdin here->
					// if reading from stdin, 
					if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
						// got error/connection closed
						if (nbytes == 0) {
							// connection closed
							printf("selectserver: socket %d hung up\n", i);
						}

						else {
							perror("recv");
						}
						close(i);
						FD_CLR(i, &master);
					}
					else {
						printf("data to send\n");
						for(j = 0; j <= fdmax; j++) {
							// send to everyone
							if (FD_ISSET(j, &master)) {
								if (j != listener && j != i) {
									if (send(j, buf, nbytes, 0) == -1) {
										perror("send");
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return 0;
}
