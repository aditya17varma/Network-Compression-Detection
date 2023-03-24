#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>

#define MAX 1024
#define SA struct sockaddr

void pre_probing (int sockfd, FILE *fp) {
    char data[MAX];
    int n;

    while (fgets(data, MAX, fp) != NULL) {
        if (send(sockfd, data, sizeof(data), 0) == -1){
            perror("Client TCP pre-probing error, sending file..");
            exit(1);
        }
        bzero(data, MAX);
    }
}

int main() {
    char *ip = "127.0.0.1";
    int port = 8080;

    int sockfd, connfd;
    struct sockaddr_in servaddr;
    FILE *fp;
    char *filename = "config.json";

    //socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        perror("Client socket creation failed...");
        exit(1);
    }
    else {
        printf("Client socket created...\n");
    }
    // bzero(&servaddr, sizeof(servaddr));
    printf("Assigning IP and ports.\n");
    // assign IP, PORT
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip);
    servaddr.sin_port = htons(port);
    char port_buff[MAX];
    inet_ntop(AF_INET, &servaddr, port_buff, MAX);
    printf("Assigned IP: %s\n", port_buff);
    printf("Assigned IP and ports!\n");
    
    // connect the client socket to server socket
    printf("Tryng to connect...\n");
    connfd = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (connfd == -1) {
        printf("Client connection to server socket failed...\n");
        exit(1);
    }
    else {
        printf("Client connected successfully to Server socket\n");
    }

    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Client error in reading file");
        exit(1);
    }
    
    pre_probing(sockfd, fp);
    printf("Client sent file successfully!\n");
    printf("Client closing connection..\n");

    close(sockfd);

}

