#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX 1024
#define UDPNUM 20
#define UDPTTL 255
#define UDPSIZE 1000


struct udp_payload{
    short packet_id;
    char payload[998];
};

void generate_random_file(){
    FILE *fp;
    char *filename = "random_file.txt";
    unsigned char buffer[2000];

    fp = fopen(filename, "w");

    int randomData = open("/dev/urandom", O_RDONLY);
    ssize_t result = read(randomData, buffer, sizeof(buffer));
    fprintf(fp, "%s", buffer);
    bzero(buffer, sizeof(buffer));
}

int main(){
    char *ip = "127.0.0.1";
    int port = 8090;
    char optval[4] = {0x40, 0, 0, 0};

    generate_random_file();
    

    // int sockfd;
    // struct sockaddr_in servaddr;
    // char buffer[MAX];
    // socklen_t addr_size;

    // sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    // if (sockfd < 0) {
    //     perror("UDP Client socket error");
    //     exit(1);
    // }
    // else {
    //     printf("UDP Client socket created\n");
    // }

    // // set DF flag
    // int df_val = 2;
    // if (setsockopt(sockfd, IPPROTO_IP, IP_DONTFRAG, &df_val, sizeof(df_val)) < 0){
    //     perror("setsockopt failed");
    //     exit(1);
    // }
    // else {
    //     printf("setsockopt successful!\n");
    // }

    // memset(&servaddr, 0, sizeof(servaddr));
    // servaddr.sin_family = AF_INET;
    // servaddr.sin_port = htons(port);
    // servaddr.sin_addr.s_addr = inet_addr(ip);

    
    // //set packet to 0
    // bzero(buffer, 1000);
    // memset(&buffer, 0, sizeof(buffer));
    // // set packet ID
    // struct udp_payload packet;
    
    // for (int i = 5000; i <5020; i++){
    //     packet.packet_id = i;
    //     memset(packet.payload, 0, 998);

    //     printf("Packet ID: %d\n", packet.packet_id);
    //     // printf("ID len: %lu\n", sizeof(packet.packet_id));
    //     // printf("Payload len: %lu\n", sizeof(packet.payload));
    //     printf("UDP Struct Packet length: %lu\n", sizeof(packet));

    //     // strcpy(buffer, "Hello from client");
    //     sendto(sockfd, &packet, 1000, 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
    //     // free(packet.packet_id);
    //     // free(packet.payload);
    //     // free(packet);

    // }
    
    
    
    printf("Data sent to client\n");

    return 0;
}