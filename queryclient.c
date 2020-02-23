#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "includes/queryprotocol.h"
#define MAXQUERY 100
#define MAXDATASIZE 1500  // max number of bytes we can get at once
#define VALIDPORT 1024
void RunQuery(char *query, char* ip, char* port) {
  // Find the address

  // Create the socket

  // Connect to the server

  // Do the query-protocol

  // Close the connection
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(ip, port, &hints, &p)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        freeaddrinfo(p);
        exit(1);
    }

    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
        perror("error client: socket");
        exit(1);
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
        close(sockfd);
        perror("error client: connect");
        exit(1);
    }

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("error client: receive ACK");
        exit(1);
    }

    buf[numbytes] = '\0';
    if (CheckAck(buf) == -1) {
      perror("error client: check ACK");
      exit(1);
    }

    if (send(sockfd, query, strlen(query), 0) == -1) {
      perror("send query");
      exit(1);
    }


    if ((numbytes = recv(sockfd, &buf, sizeof(buf), 0)) == -1) {
      perror("error client: receive number of responses");
      exit(1);
    }
    buf[numbytes] = '\0';
    int index = atoi(buf);

    if (SendAck(sockfd) == -1) {
      perror("send ack");
      exit(1);
    }

    char rec1[MAXDATASIZE] = {'\0'};
    int tempbytes;
    for (int i = 0; i < index; i++) {
      tempbytes = recv(sockfd, rec1, sizeof(rec1), 0);
      if (tempbytes == -1) {
        perror("error client: get a movie result");
        exit(-1);
      } else {
          rec1[tempbytes] = '\0';
          printf("%s\n", rec1);
        }

      if (SendAck(sockfd) == -1) {
        perror("error client: send ack");
        exit(1);
      }
    }

    if (recv(sockfd, buf, sizeof(buf), 0) == -1) {
      perror("error client: receive good bye");
      exit(-1);
    }

    if (CheckGoodbye(buf) == -1) {
      perror("error client: check good bye");
      exit(1);
    }
  freeaddrinfo(p);
}

void KillServer(char* ip, char* port) {
  // Find the address

  // Create the socket

  // Connect to the server

  // Do the query-protocol

  // Close the connection
  int sockfd;
  struct addrinfo hints, *p;
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(ip, port, &hints, &p)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    freeaddrinfo(p);
    exit(1);
  }


  if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
    perror("client: socket");
    exit(1);
  }

  if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
    close(sockfd);
    perror("client: connect");
    exit(1);
  }
  freeaddrinfo(p);
  if (SendKill(sockfd) == -1) {
    perror("send kill");
    exit(1);
  }
}

void RunPrompt(char* ip, char* port) {
  char input[1000];

  while (1) {
    printf("Enter a term to search for, q to quit or k to kill: ");
    scanf("%s", input);
    if (strlen(input) > MAXQUERY) {
      perror("only 100 characters are allowed");
      exit(1);
    }
    if (strlen(input) == 1) {
      if (input[0] == 'q') {
       // Quit the program
        exit(0);

      } else {
          if ((input[0] == 'k') || input[0] == 'K') {
        // Kill the server
            KillServer(ip, port);
            exit(0);
          }
        }
    }

  // Get the query
  RunQuery(input, ip, port);
  }
}

int isValidIpAddress(char *ipAddress) {
  struct sockaddr_in sa;
  int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
  return result != 1;
}

int main(int argc, char **argv) {
  // Check/get arguments
  if (argc != 3) {
    printf("invalid number of arguments\n");
    exit(1);
  }
  if (isValidIpAddress(argv[1])) {
    printf("Plead enter a valid ip address in this foramt: x.x.x.x\n");
    exit(1);
  }
  if (atoi(argv[2]) < 1024) {
    printf("port number needs to be >= 1024\n");
    exit(-1);
  }

  // Get info from user
  RunPrompt(argv[1], argv[2]);
  // Run Query
  return 0;
}
