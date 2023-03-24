#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#define MAX 1024

void write_file(int sockfd) {
    int n;
    FILE *fp;
    char *filename = "Server_config.json";
    char buffer[MAX];

    fp = fopen(filename, "w");
    while(1) {
        n = recv(sockfd, buffer, MAX, 0);
        if (n <= 0){
            break;
            return;
        }
        printf("%s\n", buffer);
        bzero(buffer, MAX);
    }

    fclose(fp);

}

void post_probing (int sockfd) {
    char post_string[] = "No Compression!!";
    int n;

    if (send(sockfd, post_string, sizeof(post_string), 0) == -1){
        perror("Server TCP post-probing error, sending result string..");
        exit(1);
    }
}


int main() {
    char *ip = "127.0.0.1";
    int port = 8080;
    // int n;

    // FILE *fp;
    // char *filename = "test_results.txt";
    
    int sockfd, bindfd, new_sock;
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;
    char buffer[MAX];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        perror("Server error in creating socket...");
        exit(1);
    }
    else {
        printf("Server socket creation successful\n");
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    char port_buff[MAX];
    inet_ntop(AF_INET, &server_addr, port_buff, MAX);
    printf("Assigned IP: %s\n", port_buff);

    bindfd = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (bindfd == -1){
        perror("Server bind failed");
    }
    printf("Server socket bind successful!\n");

    if (listen(sockfd, 10) == 0) {
        printf("Server listening...\n");
    }
    else {
        perror("Server error listening...");
        exit(1);
    }

    // char buffer[MAX];
    // while(1) {
    //     n = recv(sockfd, buffer, MAX, 0);
    //     if (n <= 0) {
    //         break;
    //         return;
    //     }
    //     printf("%s\n", buffer);
    //     bzero(buffer, MAX);
    // }

    // fp = fopen(filename, "r");
    // if (fp == NULL) {
    //     perror("Client error in reading file");
    //     exit(1);
    // }

    addr_size = sizeof(new_addr);
    new_sock = accept(sockfd, (struct sockaddr*)&new_addr, &addr_size);
    printf("Server accepted!\n");
    write_file(new_sock);

    // post_probing(sockfd);
    printf("Server sent file successfully!\n");
    printf("Server closing connection..\n");

    close(sockfd);

}
