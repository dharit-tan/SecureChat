#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

int createSocket(char *host, char *port) {
	int ret, yes, socketfd;
	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;     // don't care if IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

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

int main(argc, char *argv[]) {
	
}