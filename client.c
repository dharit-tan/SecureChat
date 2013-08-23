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

int main(int argc, char *argv[])
{
   char buf[BUF_MAX];
   struct addrinfo hints;
   struct addrinfo *servinfo, *p;
   int ret, socketfd;
   if (argc != 3)
   {
      fprintf(stderr, "usage: ");
      fprintf(stderr, "./client server-IP-address port-number\n");
      exit(EXIT_FAILURE);
   }

   memset(&hints, 0, sizeof(hints)); /* first set all memory to 0 */
   hints.ai_family = AF_UNSPEC; /* don't care if IPv4 or IPv6 */
   hints.ai_socktype = SOCK_STREAM; /* we're using TCP here */

   /* getaddrinfo call returns linked list of addrinfo structs */
   ret = getaddrinfo(argv[1], argv[2],
                     &hints, &servinfo);
   if (ret != 0)
   {
      freeaddrinfo(servinfo);
      fprintf(stderr, "client: getaddrinfo: %s\n", gai_strerror(ret));
      return 1;
   }

   /* iterate through all results and connect to first one possible */
   for (p = servinfo; p != NULL; p = p->ai_next)
   {
      /* create socket file descriptor from addrinfo struct */
      socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if (socketfd == -1)
      {
         freeaddrinfo(servinfo);
         perror("client: socket");
         continue;
      }
      printf("after socket call\n");

      /* connect to socket if it was created successfully */
      ret = connect(socketfd, p->ai_addr, p->ai_addrlen);
      if (ret == -1)
      {
         freeaddrinfo(servinfo);
         close(socketfd);
         perror("client: connect");
         continue;
      }
      printf("after connect\n");

      break;
   }

   /* p will only be NULL if we iterated through the whole list */
   if (p == NULL)
   {
      freeaddrinfo(servinfo);
      fprintf(stderr, "client: failed to connect\n");
      return 2;
   }
   
   /* we don't need this anymore */
   freeaddrinfo(servinfo);
   memset(buf, 0, sizeof(buf)); /* avoid valgrind error of unitialized variables */

   /* main loop to send message to server */
   while ((ret = read(0, buf, BUF_MAX)) > 0)
   {
      ret = send(socketfd, buf, ret, 0);
      if (ret == -1) perror("client: send");
      memset(buf, 0, sizeof(buf)); // reset buf
   }
   // for (;;) {
   //    ret = recv(socketfd, buf, BUF_MAX, 0);
   //    if (ret < 0) perror("recv");
   //    printf("recv'd from server: %s\n", buf);
   // }
   
   close(socketfd);
   return 0;
}
