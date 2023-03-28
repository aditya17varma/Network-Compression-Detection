#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include "cJSON.h"
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#define MAX 1500

void pre_probing(int sockfd) {
    int num_bytes;
    FILE *fp;
    char *filename = "server_config.json";
    char buffer[MAX];

    // printf("Starting pre-probing\n");

    fp = fopen(filename, "w");
    while ((num_bytes = recv(sockfd, buffer, MAX, 0)) > 0){
        // printf("Num bytes: %d\n", num_bytes);
        // printf("buffer: %s\n", buffer);
        fprintf(fp, "%s\n", buffer);
    }

    fclose(fp);
    return;
}

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

struct config_struct* parse_JSON() {

    struct config_struct *config = malloc(sizeof(struct config_struct));

    FILE *fp = fopen("server_config.json", "r");

    char buffer[1024];
    fread(buffer, 1, 1024, fp);
    fclose(fp);
    
    cJSON *root = cJSON_Parse(buffer);
    char *server_ip_string;

    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "JSON Parsing error before: %s\n", error_ptr);
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
    }

    cJSON *Dest_UDP_Port = cJSON_GetObjectItemCaseSensitive(root, "Dest_UDP_Port");
    if (cJSON_IsNumber(Dest_UDP_Port)) {
        config->Dest_UDP_Port = Dest_UDP_Port->valueint;
    }

    cJSON *Dest_Port_TCP_Head_SYN = cJSON_GetObjectItemCaseSensitive(root, "Dest_Port_TCP_Head_SYN");
    if (cJSON_IsNumber(Dest_Port_TCP_Head_SYN)) {
        config->Dest_Port_TCP_Head_SYN = Dest_Port_TCP_Head_SYN->valueint;
    }

    cJSON *Dest_Port_TCP_Tail_SYN = cJSON_GetObjectItemCaseSensitive(root, "Dest_Port_TCP_Tail_SYN");
    if (cJSON_IsNumber(Dest_Port_TCP_Tail_SYN)) {
        config->Dest_Port_TCP_Tail_SYN = Dest_Port_TCP_Tail_SYN->valueint;
    }

    cJSON *TCP_Port = cJSON_GetObjectItemCaseSensitive(root, "TCP_Port");
    if (cJSON_IsNumber(TCP_Port)) {
        config->TCP_Port = TCP_Port->valueint;
    }

    cJSON *UDP_Payload_Size = cJSON_GetObjectItemCaseSensitive(root, "UDP_Payload_Size");
    if (cJSON_IsNumber(UDP_Payload_Size)) {
        config->UDP_Payload_Size = UDP_Payload_Size->valueint;
    }

    cJSON *Inter_Measurement_Time = cJSON_GetObjectItemCaseSensitive(root, "Inter_Measurement_Time");
    if (cJSON_IsNumber(Inter_Measurement_Time)) {
        config->Inter_Measurement_Time = Inter_Measurement_Time->valueint;
    }  

    cJSON *Number_UDP_Packets = cJSON_GetObjectItemCaseSensitive(root, "Number_UDP_Packets");
    if (cJSON_IsNumber(Number_UDP_Packets)) {
        config->Number_UDP_Packets = Number_UDP_Packets->valueint;
    }  

    cJSON *UDP_TTL = cJSON_GetObjectItemCaseSensitive(root, "UDP_TTL");
    if (cJSON_IsNumber(UDP_TTL)) {
        config->UDP_TTL = UDP_TTL->valueint;
    }

    cJSON_Delete(root);
    return config;
}

void post_probing (int sockfd, bool comp) {
    char *post_string;
    int n;
    char buffer[MAX];
        
    if (comp) {
        post_string = "Compression Detected..\n";
        strcpy(buffer, post_string);

    }
    else {
        post_string = "No Compression Detected!!\n";
        strcpy(buffer, post_string);
    }

    if (send(sockfd, buffer, sizeof(buffer), 0) == -1){
        perror("Server TCP post-probing error, sending result string..");
        exit(1);
    }
}


int main(int argc, char** argv) {
     if (argc < 2){
        perror("Please include the name of the config file");
        printf("Usage: ./server <TCP Port>\n");
    }

    char *filename = argv[1];
    int tcp_port = atoi(argv[1]);

    int sockfd, bindfd, new_sock, udp_sockfd;
    struct sockaddr_in server_addr, new_addr, udp_server_addr, udp_client_addr;
    socklen_t addr_size;
    char buffer[MAX];

    // TCP Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        perror("Server error in creating socket...");
        exit(1);
    }
    else {
        printf("Server TCP socket creation successful\n");
    }

    //TCP setsock opt reuse addr and reuse port
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0){
        perror("Client TCP REUSEADDR");
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)) < 0){
        perror("Client TCP REPORT");
    }


    // TCP serv_addr
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(tcp_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // char port_buff[MAX];
    // inet_ntop(AF_INET, &server_addr, port_buff, MAX);
    // printf("Assigned IP: %s\n", port_buff);

    bindfd = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (bindfd == -1){
        perror("Server bind failed");
    }
    else {
        printf("Server TCP socket bind successful!\n");
    }
    

    if (listen(sockfd, 10) < 0) {
        perror("Server error listening...");
        exit(1);
       
    }
    else {
         printf("Server listening...\n");
    }

    addr_size = sizeof(new_addr);
    new_sock = accept(sockfd, (struct sockaddr*)&new_addr, &addr_size);
    // printf("Server accepted!\n");
    
    //accept and write config file to server_config.
    pre_probing(new_sock);
    
    // Parse JSON for other info
    struct config_struct *config = parse_JSON();
    int inter_mes = config->Inter_Measurement_Time;
    // printf("Server sent file successfully!\n");
    printf("Server closing connection..\n");
    sleep(inter_mes / 2);
    close(sockfd);

    int udp_dest_port = config->Dest_UDP_Port;
    int payload_size = config->UDP_Payload_Size;
    int num_packets = config->Number_UDP_Packets;
    int udp_ttl = config->UDP_TTL;
    in_addr_t server_ip = config->Server_IP;
    


    // Setup UDP
    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sockfd < 0) {
        perror("UDP Server socket error");
        exit(1);
    }
    else {
        printf("UDP Server socket created\n");
    }
    memset(&udp_server_addr, '\0', sizeof(udp_server_addr));
    udp_server_addr.sin_family = AF_INET;
    udp_server_addr.sin_port = htons(udp_dest_port);
    udp_server_addr.sin_addr.s_addr = server_ip;
    // printf("Server UDP IP: %d\n", server_ip);

    int n;
    n = bind(udp_sockfd, (struct sockaddr*)&udp_server_addr, sizeof(udp_server_addr));
    if (n < 0){
        perror("UDP Server bind error");
        exit(1);
    }
    // else {
    //     printf("UDP Server bind successful\n");
    // }
    // printf("UDP Server awaiting data...\n");

    // set timeout, tv_sec is timeout in seconds
    struct timeval timeout;
    timeout.tv_sec = inter_mes * 0.75;
    timeout.tv_usec = 0;
    if (setsockopt(udp_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
        perror("UDP setsockopt timeout");
        exit(1);
    }

    // clock_t low_start, low_end, high_start, high_end;
    struct timeval low_start, low_end, high_start, high_end;

    int train = 0;
    bool first_received = false;
    char payload_buffer[payload_size];

    struct udp_payload{
        unsigned short packet_id;
        char payload[payload_size-2];
    };

    int train0_packets = 0;
    int train1_packets = 0;

    while(1){
        bzero(payload_buffer, payload_size);
        addr_size = sizeof(udp_client_addr);
        int rv = recvfrom(udp_sockfd, payload_buffer, payload_size, 0, (struct sockaddr*)&udp_client_addr, &addr_size);
        if (train == 0){
            if (!first_received){
                // low_start = clock();
                gettimeofday(&low_start, NULL);
                first_received = true;
            }
            else {
                // low_end = clock();
                gettimeofday(&low_end, NULL);
                train0_packets++;
            }
        }
        else {
            if (!first_received){
                // high_start = clock();
                gettimeofday(&high_start, NULL);
                first_received = true;
            }
            else {
                // high_end = clock();
                gettimeofday(&high_end, NULL);
                train1_packets++;
            }
        }

        if (rv <= 0){
            if (rv == 0){
                printf("NO PACKETS RECEIVED\n");
            }
            else {
                if (errno == EAGAIN){
                    // printf("UDP Timeout\n");
                    first_received = false;
                    if (train == 0){
                        train = 1;
                        // printf("Train set to: %d\n\n", train);
                        continue;
                    }
                    else {
                        break;
                    }
                }
                else{
                    perror("UDP recvfrom");
                    exit(1);
                }
            }
        }

        struct udp_payload *recv_packet = (struct udp_payload *) payload_buffer;
        // printf("packet payload: %s\n", recv_packet->payload);
        // printf("packet_id: %d\n", recv_packet->packet_id);
        // printf("Data received from Client\n");
    }

    // printf("Train 0: %d\n", train0_packets);
    // printf("Train 1: %d\n", train1_packets);

    printf("\nLow start: %ld\n", low_start.tv_sec);
    printf("Low ends: %ld\n", low_end.tv_sec);
    long double low_diff = (low_end.tv_sec*1000 + low_end.tv_usec/1000) - (low_start.tv_sec*1000 + low_start.tv_usec/1000);
    printf("Low Delta: %Lf\n", low_diff);
    printf("\nHigh start: %ld\n", high_start.tv_sec);
    printf("High end: %ld\n", high_end.tv_sec);
    long double high_diff =  (high_end.tv_sec*1000  + high_end.tv_usec/1000) - (high_start.tv_sec*1000 - high_start.tv_usec/1000);
    printf("High Delta: %Lf\n", high_diff);

    long double difference = high_diff - low_diff;
    
    printf("\nDifference between high and low Deltas: %Lfms\n", difference);
    bool compression = false;
    if (difference < 100){
        printf("No Compression!\n");
    }
    else {
        compression = true;
        printf("Compression Detected!\n");
    }

    // printf("Exited while loop!\n");

    
    // TCP Post-Probing
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        perror("Server error in creating socket...");
        exit(1);
    }
    // else {
    //     printf("Server socket creation successful\n");
    // }

    //TCP setsock opt reuse addr and reuse port
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0){
        perror("Client TCP REUSEADDR");
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)) < 0){
        perror("Client TCP REPORT");
    }

    // TCP serv_addr
    server_addr.sin_port = htons(tcp_port);

    bindfd = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (bindfd == -1){
        perror("Server bind failed");
    }
    // printf("Server socket bind successful!\n");

    if (listen(sockfd, 10) < 0) {
        perror("Server error listening...");
        exit(1);
       
    }
    // else {
    //      printf("Server listening...\n");
    // }

    addr_size = sizeof(new_addr);
    new_sock = accept(sockfd, (struct sockaddr*)&new_addr, &addr_size);
    // printf("Server accepted!\n");

    post_probing(new_sock, compression);
    close(sockfd);
    
    return 0;

}
