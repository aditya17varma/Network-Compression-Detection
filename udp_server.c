#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#define MAX 1024


struct udp_payload{
    short packet_id;
    char payload[998];
};

int main( int argc, char **argv) {
    char *ip = "127.0.0.1";
    int port = 8090;

    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1000];
    socklen_t addr_size;
    int n;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("UDP Server socket error");
        exit(1);
    }
    else {
        printf("UDP Server socket created\n");
    }

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    n = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (n < 0){
        perror("UDP Server bind error");
        exit(1);
    }
    else {
        printf("UDP Server bind successful\n");
    }
    printf("UDP Server awaiting data...\n");

    time_t endwait;
    time_t start = time(NULL);
    time_t looptime = 10;
    endwait = time(NULL) + looptime;

    while(start < endwait) {
        bzero(buffer, 1000);
        addr_size = sizeof(client_addr);
        int rv = recvfrom(sockfd, buffer, 1000, 0, (struct sockaddr*)&client_addr, &addr_size);

        if (rv == -1){
            perror("UDP Server recvfrom");
            break;
        }

        struct udp_payload *recv_packet = (struct udp_payload *) buffer;
        printf("packet payload: %s\n", recv_packet->payload);
        printf("packet_id: %d\n", recv_packet->packet_id);
        printf("Data received from Client\n");

        start = time(NULL);
    }
    
    

    return 0;



}



