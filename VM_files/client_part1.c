#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include "cJSON.h"
#include <errno.h>

#define MAX 1500
#define SA struct sockaddr

int global_payload_size = 1500;

struct config_struct {
    in_addr_t Server_IP;
    int Source_UDP_Port;
    int Dest_UDP_Port;
    int Dest_Port_TCP_Head_SYN;
    int Dest_Port_TCP_Tail_SYN;
    int TCP_Port;
    int UDP_Payload_Size;
    int Inter_Measurement_Time;
    int Number_UDP_Packets;
    int UDP_TTL;
};

// struct udp_payload{
//     short packet_id;
//     char payload[global_payload_size - 2];
// };

struct config_struct* parse_JSON() {

    struct config_struct *config = malloc(sizeof(struct config_struct));

    FILE *fp = fopen("../config.json", "r");

    char buffer[1024];
    fread(buffer, 1, 1024, fp);
    fclose(fp);
    
    cJSON *root = cJSON_Parse(buffer);
    char *server_ip_string;

    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        exit(EXIT_FAILURE);
    }

    cJSON *Server_IP = cJSON_GetObjectItem(root, "Server_IP");
    if (cJSON_IsString(Server_IP)) {
        config->Server_IP = inet_addr(Server_IP->valuestring);
        // printf("Server IP struct: %d\n", config->Server_IP);
    }
    else {
        printf("Shouldn't get here\n");
    }

    cJSON *Source_UDP_Port = cJSON_GetObjectItemCaseSensitive(root, "Source_UDP_Port");
    if (cJSON_IsNumber(Source_UDP_Port)) {
        config->Source_UDP_Port = Source_UDP_Port->valueint;
        // printf("Source_UDP_Port: %d\n", config.Source_UDP_Port);
    }

    cJSON *Dest_UDP_Port = cJSON_GetObjectItemCaseSensitive(root, "Dest_UDP_Port");
    if (cJSON_IsNumber(Dest_UDP_Port)) {
        config->Dest_UDP_Port = Dest_UDP_Port->valueint;
        // printf("Dest_UDP_Port: %d\n", config.Dest_UDP_Port);
    }

    cJSON *Dest_Port_TCP_Head_SYN = cJSON_GetObjectItemCaseSensitive(root, "Dest_Port_TCP_Head_SYN");
    if (cJSON_IsNumber(Dest_Port_TCP_Head_SYN)) {
        config->Dest_Port_TCP_Head_SYN = Dest_Port_TCP_Head_SYN->valueint;
        // printf("Dest_Port_TCP_Head_SYN: %d\n", config.Dest_Port_TCP_Head_SYN);
    }

    cJSON *Dest_Port_TCP_Tail_SYN = cJSON_GetObjectItemCaseSensitive(root, "Dest_Port_TCP_Tail_SYN");
    if (cJSON_IsNumber(Dest_Port_TCP_Tail_SYN)) {
        config->Dest_Port_TCP_Tail_SYN = Dest_Port_TCP_Tail_SYN->valueint;
        // printf("Dest_Port_TCP_Tail_SYN: %d\n", config.Dest_Port_TCP_Tail_SYN);
    }

    cJSON *TCP_Port = cJSON_GetObjectItemCaseSensitive(root, "TCP_Port");
    if (cJSON_IsNumber(TCP_Port)) {
        config->TCP_Port = TCP_Port->valueint;
        // printf("TCP_Port: %d\n", config.TCP_Port);
    }

    cJSON *UDP_Payload_Size = cJSON_GetObjectItemCaseSensitive(root, "UDP_Payload_Size");
    if (cJSON_IsNumber(UDP_Payload_Size)) {
        config->UDP_Payload_Size = UDP_Payload_Size->valueint;
        // printf("UDP_Payload_Size: %d\n", config.UDP_Payload_Size);
    }

    cJSON *Inter_Measurement_Time = cJSON_GetObjectItemCaseSensitive(root, "Inter_Measurement_Time");
    if (cJSON_IsNumber(Inter_Measurement_Time)) {
        config->Inter_Measurement_Time = Inter_Measurement_Time->valueint;
        // printf("Inter_Measuerment_Time: %d\n", config.Inter_Measurement_Time);
    }  

    cJSON *Number_UDP_Packets = cJSON_GetObjectItemCaseSensitive(root, "Number_UDP_Packets");
    if (cJSON_IsNumber(Number_UDP_Packets)) {
        config->Number_UDP_Packets = Number_UDP_Packets->valueint;
        // printf("Number_UDP_Packets: %d\n", config.Number_UDP_Packets);
    }  

    cJSON *UDP_TTL = cJSON_GetObjectItemCaseSensitive(root, "UDP_TTL");
    if (cJSON_IsNumber(UDP_TTL)) {
        config->UDP_TTL = UDP_TTL->valueint;
        // printf("UDP_TTL: %d\n", config.UDP_TTL);
    }

    cJSON_Delete(root);
    return config;
}

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
    // char *ip = "192.168.64.2";
    struct config_struct *config = parse_JSON();
    in_addr_t server_ip = config->Server_IP;
    int tcp_port = config->TCP_Port;
    int udp_dest_port = config->Dest_UDP_Port;
    int payload_size = config->UDP_Payload_Size;
    int num_packets = config->Number_UDP_Packets;
    int udp_ttl = config->UDP_TTL;


    // printf("STRUCT SERVER IP: %d\n", serv);
    // printf("Packets: %d\n", config->Number_UDP_Packets);

    int sockfd, connfd, udp_sockfd;
    struct sockaddr_in servaddr, udp_servaddr;
    FILE *fp;
    char *filename = "../config.json";

    //TCP socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        perror("TCP Client socket creation failed...");
        exit(1);
    }
    else {
        printf("TCP Client socket created...\n");
    }

    //UDP socket
    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sockfd < 0) {
        perror("UDP Client socket error");
        exit(1);
    }
    else {
        printf("UDP Client socket created\n");
    }

    int df_val = IP_PMTUDISC_DO;
    if (setsockopt(udp_sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &df_val, sizeof(df_val)) < 0){
        perror("UDP setsockopt failed");
        exit(1);
    }
    else {
        printf("UDP setsockopt successful!\n");
    }

    
    // bzero(&servaddr, sizeof(servaddr));
    printf("Assigning IP and ports.\n");

    // assign TCP IP, PORT
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = server_ip;
    servaddr.sin_port = htons(tcp_port);
    // char port_buff[MAX];
    // inet_ntop(AF_INET, &servaddr, port_buff, MAX);
    printf("TCP IP: %d TCP port: %d\n", server_ip, tcp_port);
    printf("Assigned IP and TCP port!\n");

    // assign UDP IP, PORT
    // memset(&udp_servaddr, 0, sizeof(udp_servaddr));
    // udp_servaddr.sin_family = AF_INET;
    // udp_servaddr.sin_addr.s_addr = server_ip;
    // printf("Client UDP IP: %d\n", server_ip);
    
    

    // get high-entropy data ready
    struct udp_payload{
        short packet_id;
        char payload[payload_size - 2];
    };
    struct udp_payload packet;

    FILE *random_fp = fopen("../random_file.txt", "r");
    char rand_buffer[payload_size];
    printf("Trying to fgets random file\n");
    fgets(rand_buffer, sizeof(rand_buffer), random_fp);
    printf("Rand array filled\n");
    // close(random_fp);

    
    // TCP connect the client socket to server socket
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

    // TCP PRE-PROBING
    pre_probing(sockfd, fp);
    printf("Client sent file successfully!\n");
    printf("Client closing connection..\n");

    close(sockfd);

    sleep(5);

    servaddr.sin_port = htons(udp_dest_port);
    printf("UDP port: %d\n", servaddr.sin_port);

    // UDP TRAINS choo choo
    // Train 1: Low Entropy
    // socklen_t server_len = sizeof(servaddr);
    for (int i = 0; i <num_packets; i++){
        // set packet id
        packet.packet_id = i;
        // clear packet payload
        memset(packet.payload, 0, payload_size -2);
        if (sendto(udp_sockfd, &packet, sizeof(packet), 0, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
            perror("UDP Sendto error");
            printf("%s\n", strerror(errno));
        }
        
    }
    printf("Sent first train!\n");

    sleep(10);

    for (int i = 0; i <num_packets; i++){
        // set packet id
        packet.packet_id = i;
        // clear packet payload
        memset(packet.payload, 0, payload_size - 2);

        //high entropy
        strncpy(packet.payload, rand_buffer, payload_size - 2);
        if (sendto(udp_sockfd, &packet, sizeof(packet), 0, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
            perror("UDP Sendto error");
            printf("%s\n", strerror(errno));
        }

    }
    
    printf("Data sent to client\n");
    close(udp_sockfd);

    sleep(10);

    // TCP Post-Probing
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        perror("TCP Client socket creation failed...");
        exit(1);
    }
    else {
        printf("TCP Client socket created...\n");
    }

    printf("Assigning IP and ports.\n");

    // assign TCP IP, PORT
    // memset(&servaddr, 0, sizeof(servaddr));
    // servaddr.sin_family = AF_INET;
    // servaddr.sin_addr.s_addr = server_ip;
    servaddr.sin_port = htons(tcp_port);
    printf("Assigned IP and TCP port!\n");

    printf("Tryng to connect...\n");
    connfd = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (connfd == -1) {
        printf("Client connection to server socket failed...\n");
        exit(1);
    }
    else {
        printf("Client connected successfully to Server socket\n");
    }

    // TCP POST-PROBING
    int n;
    char buffer[MAX];

    n = recv(sockfd, buffer, MAX, 0);
    if (n < 0){
        perror("Client post-probing recv failed...");
        exit(1);
    }
    else {
        printf("%s\n", buffer);
        bzero(buffer, MAX);
    }
    


    printf("TCP post-probing done!\n");
    printf("Client closing connection..\n");

    close(sockfd);

    return 0;

}

