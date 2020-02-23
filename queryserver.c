#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include<sys/stat.h>

#include "includes/queryprotocol.h"
#include "includes/docset.h"
#include "includes/movieIndex.h"
#include "includes/docidmap.h"
#include "includes/Hashtable.h"
#include "includes/queryprocessor.h"
#include "includes/fileparser.h"
#include "includes/filecrawler.h"
#define MAXDATASIZE 1500  // max number of bytes we can get at once
#define MAXQUERY 100
DocIdMap docs;
Index docIndex;
#define BACKLOG 10

// const int SEARCH_RESULT_LENGTH = 1500;
char movieSearchResult[1500];

int Cleanup();

void Setup(char *dir);

void sigint_handler(int sig) {
  write(0, "Exit signal sent. Cleaning up...\n", 34);
  Cleanup();
  exit(0);
}

void Setup(char *dir) {
  printf("Crawling directory tree starting at: %s\n", dir);
  // Create a DocIdMap
  docs = CreateDocIdMap();
  CrawlFilesToMap(dir, docs);

  printf("Crawled %d files.\n", NumElemsInHashtable(docs));

  // Create the index
  docIndex = CreateIndex();

  // Index the files
  printf("Parsing and indexing files...\n");
  ParseTheFiles(docs, docIndex);
  printf("%d entries in the index.\n", NumElemsInHashtable(docIndex));
}

int Cleanup() {
  DestroyIndex(docIndex);
  DestroyDocIdMap(docs);

  return 0;
}

int main(int argc, char **argv) {
  // Check args
  struct stat dir;
  int dir1;
  dir1 = stat(argv[1], &dir);
  if (argc != 3) {
    printf("invalid number of arguments: 3 is required\n");
    exit(1);
  }

  if (dir1 == -1) {
    perror("not a valid directory");
    exit(1);
  }

  if (atoi(argv[2]) < 1024) {
    printf("port number needs to be a number that is >= 1024\n");
    exit(1);
  }

  Setup(argv[1]);

  int sockfd, numbytes;
  char buf[MAXDATASIZE] = {'\0'};
  struct addrinfo hints, *p;
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  // Step 1: get address/port info to open
  if ((rv = getaddrinfo(NULL, argv[2], &hints, &p)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    freeaddrinfo(p);
    return 1;
  }

  // Step 2: Open socket
  if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
    perror("client: socket");
    freeaddrinfo(p);
    exit(1);
  }
  // Step 3: Bind socket
  if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
    close(sockfd);
    freeaddrinfo(p);
    perror("server: bind");
  }
  freeaddrinfo(p);
  // Step 4: Listen on the socket
  printf("Waiting for connection...\n");
  if (listen(sockfd, BACKLOG) == -1) {
      perror("listen");
      exit(1);
  }
  // Step 5: Handle clients that connect
  while (1) {
    int clientID = accept(sockfd, NULL, NULL);
    printf("Client connected\n");
    if (SendAck(clientID) == -1) {
      perror("error server: send ack");
      exit(1);
    }
    numbytes = recv(clientID, buf, MAXDATASIZE, 0);
    buf[numbytes] = '\0';

    if (CheckKill(buf) == 0) {
      Cleanup();
      exit(0);
    }
    SearchResultIter iter = FindMovies(docIndex, buf);
    if (iter == NULL) {
      char *msg = "0";
      int responseNumber = send(clientID, msg, strlen(msg), 0);
      if (responseNumber == -1) {
        perror("error server: send numbers of responses");
        exit(1);
      }
      int length = recv(clientID, &buf, sizeof(buf), 0);
      buf[length] = '\0';
      if (CheckAck(buf) == -1) {
        perror("error server: receive ack");
        exit(1);
      }
      if (SendGoodbye(clientID) == -1) {
        perror("error server: send good bye");
        exit(1);
      }
    } else {
        int responses = NumResultsInIter(iter);
        char msg[200];
        sprintf(msg, "%d", responses);
        int responseNumber = send(clientID, msg, strlen(msg), 0);
        if (responseNumber == -1) {
          perror("error server: send numbers of responses");
          exit(1);
        }
        int length1 = recv(clientID, &buf, sizeof(buf), 0);
        buf[length1] = '\0';
        if (CheckAck(buf) == -1) {
          perror("error ACK");
          exit(1);
        }
        SearchResult output = (SearchResult)malloc(sizeof(struct searchResult));
        char dest[MAXDATASIZE] = {'\0'};
        int result;
        int recAck;
        while (SearchResultIterHasMore(iter)) {
          GetNextSearchResult(iter, output);
          GetRowFromFile(output, docs, dest);
          result = send(clientID, &dest, sizeof(dest), 0);
          if (result == -1) {
            perror("error result");
            exit(1);
          }
          recAck = recv(clientID, &dest, sizeof(dest), 0);
          dest[recAck] = '\0';
          if (CheckAck(dest) == -1) {
            perror("error ACK");
            exit(1);
          }
        }
       if (SendGoodbye(clientID) == -1) {
          perror("error send goodbye");
          exit(1);
       }
        printf("all movies are sent\n");
        printf("Waiting for connection...\n");
        free(output);
        DestroySearchResultIter(iter);
      }
  }
  // Step 6: Close the socket
  close(sockfd);
  Cleanup();
  return 0;
}
