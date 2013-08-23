/* 
   Author: Dharit Tantiviramanond
   NetID: dtantivi
   Description: Simple network client that sends messages to
   a server.
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

/* BUF_MAX is max input line length */
enum {BUF_MAX = 256};

/*---------------------------------------------------------------------------------------------------*/

int createSocket(char *host, char* port) {
   int socketfd, ret;
   struct addrinfo hints, *servinfo, *p;

   memset(&hints, 0, sizeof(hints)); /* first set all memory to 0 */
   hints.ai_family = AF_UNSPEC; /* don't care if IPv4 or IPv6 */
   hints.ai_socktype = SOCK_STREAM; /* we're using TCP here */

   /* getaddrinfo call returns linked list of addrinfo structs */
   ret = getaddrinfo(host, port, &hints, &servinfo);
   if (ret != 0) { freeaddrinfo(servinfo); fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret)); return -1; }

   /* iterate through all results and connect to first one possible */
   for (p = servinfo; p != NULL; p = p->ai_next) {
      /* create socket file descriptor from addrinfo struct */
      socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if (socketfd == -1) { freeaddrinfo(servinfo); perror("socket"); continue; }

      /* connect to socket if it was created successfully */
      ret = connect(socketfd, p->ai_addr, p->ai_addrlen);
      if (ret == -1) { freeaddrinfo(servinfo); close(socketfd); perror("connect"); continue; }

      break;
   }

   /* p will only be NULL if we iterated through the whole list */
   if (p == NULL) { freeaddrinfo(servinfo); fprintf(stderr, "failed to connect\n"); return -1; }
   
   /* we don't need this anymore */
   freeaddrinfo(servinfo);
   return socketfd;
}

/*---------------------------------------------------------------------------------------------------*/

int main(int argc, char *argv[]) {
   char buf[BUF_MAX];
   int ret, socketfd, i, running;

   int fdmax;
   fd_set master, read_fds;

   if (argc != 3) {
      fprintf(stderr, "usage: ");
      fprintf(stderr, "./client server-IP-address port-number\n");
      exit(EXIT_FAILURE);
   }

   socketfd = createSocket(argv[1], argv[2]);
   if (socketfd == -1) return -1;
   memset(buf, 0, sizeof(buf)); /* avoid valgrind error of unitialized variables */

   // initialize fd set shit
   FD_ZERO(&master);
   FD_ZERO(&read_fds);
   FD_SET(socketfd, &master);
   FD_SET(fileno(stdin), &master);
   fdmax = socketfd;

   printf("connected to %s on port %s\n", argv[1], argv[2]);
   running = 1;

   while (running) {
      read_fds = master;
      if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) { perror("select"); return -1; }

      for (i = 0; i <= fdmax; i++) {
         if (FD_ISSET(i, &read_fds)) {
            if (i == socketfd) {
               ret = recv(i, buf, sizeof(buf), 0);
               if (ret <= 0) {
                  if (ret == 0) printf("server closed the connection\n");
                  else if (ret == -1) perror("recv");
                  close(i);
                  FD_CLR(i, &master);
               }
               else {
                  printf("%s", buf);
                  memset(buf, 0, sizeof(buf));
               }
            }
            else if (i == fileno(stdin)) {
               ret = read(fileno(stdin), buf, BUF_MAX);
               if (ret == -1) perror("read");
               if (ret == 0) running = 0;
               ret = send(socketfd, buf, ret, 0);
               if (ret == -1) perror("send");
               memset(buf, 0, sizeof(buf)); // reset buf
            }
         }
      }
   }
   
   close(socketfd);
   return 0;
}