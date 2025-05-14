#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SIZE_MSG 100

#define PORT 7
#define ADDR "::1" // localhost -> changer à "lampe" ou "nivose"

int main(int argc, char *argv[])
{
  // struct sockaddr_in6 address_sock;
  int socket_fd, ret_val;
  char* addr;
  char* port;
  if(argc > 1){
    addr=argv[1];
    port=argv[2];
  }else{
    fprintf(stderr,"usage : ./a.out <domain> <port>\n",strlen("usage : ./a.out <domain> <port>\n"));
    exit(EXIT_FAILURE);
  }
  char buf[SIZE_MSG], msg[SIZE_MSG];
  int lus, recus;

  // memset(&address_sock, 0, sizeof(address_sock));
  // address_sock.sin6_family = AF_INET6;
  // address_sock.sin6_port = htons(PORT);
  // inet_pton(AF_INET6, ADDR, &address_sock.sin6_addr);
  struct addrinfo hints, *r, *p;
  hints.ai_flags = 0;
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;

  if (getaddrinfo(addr, port, &hints, &r) > 0)
  {
    perror("getaddrinfo");
    exit(EXIT_FAILURE);
  }

  p = r;
  while (p != NULL)
  {
    if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) > 0)
    {
      if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == 0)
      {
        break;
      }
      close(socket_fd);
    }
    p = p->ai_next;
  }
  if (p == NULL)
    exit(EXIT_FAILURE);

  for (int i = 0; i < 10; i++)
  {
    snprintf(msg, SIZE_MSG, "Hello%d", i);
    printf("SENDING: %s\n", msg);
    lus = 0;
    while (lus < strlen(msg))
    {
      int n;
      if ((n = send(socket_fd, msg + lus, strlen(msg) - lus, 0)) <= 0)
      {
        perror("send");
        exit(EXIT_FAILURE);
      }
      lus += n;
    }

    recus = 0;
    while (recus < strlen(msg))
    {
      int n;
      if ((n = recv(socket_fd, buf + recus, strlen(msg) - recus, 0)) <= 0)
      {
        perror("send");
        exit(EXIT_FAILURE);
      }
      recus += n;
    }
    buf[recus] = '\0';
    printf("RECEIVED: %s\n", buf);
  }

  close(socket_fd);
}
