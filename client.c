/**
Program to implement the client to initiate TCP connections and send UDP trains to the server.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include "cJSON.h"
#include <errno.h>
#include <stdbool.h>

#define MAX 1500
#define SA struct sockaddr

int global_payload_size = 1500;

/*
Function Declarations
*/
int send_udp_trains(int, struct sockaddr_in, int, int, bool);

/*
Struct to store data from the config file, used to parse the JSON file. Server_IP is type in_addr_t.
All other variables as type int.
*/
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

/*
Uses the cJSON library to parse the config.json file and load the values into a struct config_struct.
*/
struct config_struct* parse_JSON(char* filename) {

    struct config_struct *config = malloc(sizeof(struct config_struct));

    FILE *fp = fopen(filename, "r");


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
        perror("Config File Key error.");
        exit(1);
    }

    cJSON *Source_UDP_Port = cJSON_GetObjectItemCaseSensitive(root, "Source_UDP_Port");
    if (cJSON_IsNumber(Source_UDP_Port)) {
        config->Source_UDP_Port = Source_UDP_Port->valueint;
        // printf("Source_UDP_Port: %d\n", config.Source_UDP_Port);
    }
    else {
        perror("Config File Key error.");
        exit(1);
    }

    cJSON *Dest_UDP_Port = cJSON_GetObjectItemCaseSensitive(root, "Dest_UDP_Port");
    if (cJSON_IsNumber(Dest_UDP_Port)) {
        config->Dest_UDP_Port = Dest_UDP_Port->valueint;
        // printf("Dest_UDP_Port: %d\n", config.Dest_UDP_Port);
    }
    else {
        perror("Config File Key error.");
        exit(1);
    }

    cJSON *Dest_Port_TCP_Head_SYN = cJSON_GetObjectItemCaseSensitive(root, "Dest_Port_TCP_Head_SYN");
    if (cJSON_IsNumber(Dest_Port_TCP_Head_SYN)) {
        config->Dest_Port_TCP_Head_SYN = Dest_Port_TCP_Head_SYN->valueint;
        // printf("Dest_Port_TCP_Head_SYN: %d\n", config.Dest_Port_TCP_Head_SYN);
    }
    else {
        perror("Config File Key error.");
        exit(1);
    }

    cJSON *Dest_Port_TCP_Tail_SYN = cJSON_GetObjectItemCaseSensitive(root, "Dest_Port_TCP_Tail_SYN");
    if (cJSON_IsNumber(Dest_Port_TCP_Tail_SYN)) {
        config->Dest_Port_TCP_Tail_SYN = Dest_Port_TCP_Tail_SYN->valueint;
        // printf("Dest_Port_TCP_Tail_SYN: %d\n", config.Dest_Port_TCP_Tail_SYN);
    }
    else {
        perror("Config File Key error.");
        exit(1);
    }

    cJSON *TCP_Port = cJSON_GetObjectItemCaseSensitive(root, "TCP_Port");
    if (cJSON_IsNumber(TCP_Port)) {
        config->TCP_Port = TCP_Port->valueint;
        // printf("TCP_Port: %d\n", config.TCP_Port);
    }
    else {
        perror("Config File Key error.");
        exit(1);
    }

    cJSON *UDP_Payload_Size = cJSON_GetObjectItemCaseSensitive(root, "UDP_Payload_Size");
    if (cJSON_IsNumber(UDP_Payload_Size)) {
        config->UDP_Payload_Size = UDP_Payload_Size->valueint;
        // printf("UDP_Payload_Size: %d\n", config.UDP_Payload_Size);
    }
    else {
        perror("Config File Key error.");
        exit(1);
    }

    cJSON *Inter_Measurement_Time = cJSON_GetObjectItemCaseSensitive(root, "Inter_Measurement_Time");
    if (cJSON_IsNumber(Inter_Measurement_Time)) {
        config->Inter_Measurement_Time = Inter_Measurement_Time->valueint;
        // printf("Inter_Measuerment_Time: %d\n", config.Inter_Measurement_Time);
    }  
    else {
        perror("Config File Key error.");
        exit(1);
    }

    cJSON *Number_UDP_Packets = cJSON_GetObjectItemCaseSensitive(root, "Number_UDP_Packets");
    if (cJSON_IsNumber(Number_UDP_Packets)) {
        config->Number_UDP_Packets = Number_UDP_Packets->valueint;
        // printf("Number_UDP_Packets: %d\n", config.Number_UDP_Packets);
    }  
    else {
        perror("Config File Key error.");
        exit(1);
    }

    cJSON *UDP_TTL = cJSON_GetObjectItemCaseSensitive(root, "UDP_TTL");
    if (cJSON_IsNumber(UDP_TTL)) {
        config->UDP_TTL = UDP_TTL->valueint;
        // printf("UDP_TTL: %d\n", config.UDP_TTL);
    }
    else {
        perror("Config File Key error.");
        exit(1);
    }

    cJSON_Delete(root);
    return config;
}

/*
Sends the config file to the server using the send() function.
Arguments: socket file descriptor (for the TCP socket), and the FILE pointer (to the config file)
*/
void pre_probing (int sockfd, FILE *fp) {
    char *read_buff;

    long lSize;
    fseek(fp, 0L, SEEK_END);
    lSize = ftell(fp);
    rewind(fp);

    read_buff = calloc(1, lSize + 1);
    if (!read_buff){
        fclose(fp);
        perror("read_buff failed...");
        exit(1);
    }
    if(1 != fread(read_buff, lSize, 1, fp)){
        fclose(fp);
        perror("fread failed...");
        free(read_buff);
        exit(1);
    }

    // printf("read_buff: %s\n", read_buff);
    // printf("Read_buff size: %ld\n", sizeof(read_buff));

    if (send(sockfd, read_buff, lSize + 1, 0) == -1){
        perror("Client TCP pre-probing error, sending file..");
        // printf("%s\n", read_buff);
        exit(1);
    }

    free(read_buff);
}

/*
Main function
Parses the config file name from the commmand line argument.
Sets up the TCP and UDP sockets and sockaddr_in structs.
First TCP connection is used to send the config file to the server.
Then, two UDP packet trains are sent.
A final TCP connection is established with the server to get back the results of the compression check.
*/
int main(int argc, char** argv) {
    if (argc < 2){
        perror("Please include the name of the config file");
        printf("Usage: ./client <config.json>\n");
    }

    char *filename = argv[1];
    // printf("filename: %s\n", argv[1]);

    struct config_struct *config = parse_JSON(argv[1]);
    in_addr_t server_ip = config->Server_IP;
    int tcp_port = config->TCP_Port;
    int udp_dest_port = config->Dest_UDP_Port;
    int payload_size = config->UDP_Payload_Size;
    int num_packets = config->Number_UDP_Packets;
    int udp_ttl = config->UDP_TTL;
    int inter_mes = config->Inter_Measurement_Time;
    int udp_source_port = config->Source_UDP_Port;



    // printf("STRUCT SERVER IP: %d\n", serv);
    // printf("Packets: %d\n", config->Number_UDP_Packets);

    int sockfd, connfd, udp_sockfd;
    struct sockaddr_in servaddr, udp_servaddr, udp_cliaddr;
    FILE *fp;

    /*
    TCP socket create and verification
    */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        perror("TCP Client socket creation failed...");
        exit(1);
    }
    else {
        printf("TCP Client socket created...\n");
    }

    
    /*
    UDP socket
    */
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

    memset(&udp_cliaddr, 0, sizeof(udp_cliaddr));
    udp_cliaddr.sin_family = AF_INET;
    udp_cliaddr.sin_addr.s_addr = INADDR_ANY;
    udp_cliaddr.sin_port = htons(udp_source_port);

    if (bind(udp_sockfd, (struct sockaddr *)&udp_cliaddr, sizeof(udp_cliaddr)) < 0) {
        perror("UDP bind failed");
        exit(EXIT_FAILURE);
    }
    else{
        printf("UDP Bind successful\n");
    }

    
    // bzero(&servaddr, sizeof(servaddr));
    // printf("Assigning IP and ports.\n");

    /*
    Assign TCP IP, PORT
    */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = server_ip;
    servaddr.sin_port = htons(tcp_port);
    
    /*
    TCP connect the client socket to server socket
    */
    // printf("Tryng to connect...\n");
    connfd = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (connfd == -1) {
        printf("Client connection to server socket failed...\n");
        printf("%s\n", strerror(errno));
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

    /*
    TCP PRE-PROBING
    */
    pre_probing(sockfd, fp);
    // printf("Client sent file successfully!\n");
    // printf("Client closing connection..\n");
    fclose(fp);

    close(sockfd);

    sleep(inter_mes * 0.75);

    servaddr.sin_port = htons(udp_dest_port);
    // printf("UDP port: %d\n", servaddr.sin_port);

    bool high_entropy = false;

    
    /*
    UDP TRAINS choo choo
    Train 1: Low Entropy
    */
    int u1 = send_udp_trains(udp_sockfd, servaddr, num_packets, payload_size, high_entropy);
    if (u1 < 0){
        perror("Send UDP Train error...");
        exit(1);
    }
    
    // printf("Sent first train!\n");

    sleep(inter_mes);

    high_entropy = true;

    // }
    int u2 = send_udp_trains(udp_sockfd, servaddr, num_packets, payload_size, high_entropy);
    if (u2 < 0){
        perror("Send UDP Train error...");
        exit(1);
    }

    // printf("Data sent to client\n");
    close(udp_sockfd);

    sleep(inter_mes * 0.75 + 1);

    
    /*
    TCP Post-Probing
    */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        perror("TCP Client socket creation failed...");
        exit(1);
    }
    // else {
    //     printf("TCP Client socket created...\n");
    // }

    // printf("Assigning IP and ports.\n");

    
    /*
    Assign TCP PORT
    */
    servaddr.sin_port = htons(tcp_port);
    // printf("Assigned IP and TCP port!\n");

    // printf("Tryng to connect...\n");
    connfd = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (connfd == -1) {
        printf("Client connection to server socket failed...\n");
        exit(1);
    }
    // else {
    //     printf("Client connected successfully to Server socket\n");
    // }

    
    /*
    TCP POST-PROBING
    */
    int n;
    char buffer[MAX];

    n = recv(sockfd, buffer, MAX, 0);
    if (n < 0){
        perror("Client post-probing recv failed...");
        exit(1);
    }
    else {
        printf("\n%s\n", buffer);
        bzero(buffer, MAX);
    }
    
    printf("TCP post-probing done!\n");
    // printf("Client closing connection..\n");

    close(sockfd);

    return 0;

}

/*
Sends the UDP packet trains to the destination IP and Port.
Arguments: socket file descriptor, sockaddr_in, number of UDP packets, UDP payload size, boolean to assign high entropy data.
Uses the udp_payload struct to simplify assignment of payload data and packet id.
High-entropy data loaded from the random-file.txt, which was compiled on a local machine using /dev/urandom.
If true passed in for the bool high_entropy, payload is high entropy, otherwise zero'd out.
Returns 1 on success.
*/
int send_udp_trains(int udp_sockfd, struct sockaddr_in udp_servaddr, int num_packets, int payload_size, bool high_entropy) {
    /*
    UDP payload struct
    */
    struct udp_payload{
        unsigned short packet_id;
        char payload[payload_size - 2];
    };
    struct udp_payload udp_packet;
    FILE *random_fp = fopen("random_file.txt", "r");
    char rand_buffer[payload_size];
    // printf("Trying to fgets random file\n");
    fgets(rand_buffer, sizeof(rand_buffer), random_fp);
    // printf("Rand array filled\n");

    memset(udp_packet.payload, 0, payload_size -2);
    if (high_entropy){
        strncpy(udp_packet.payload, rand_buffer, payload_size - 2);
    }

    for (int i = 0; i < num_packets; i++){
        
        /*
        Set packet id
        */
        udp_packet.packet_id = (unsigned short)i;
        
        /*
        Send the UDP packet
        */
        if (sendto(udp_sockfd, &udp_packet, sizeof(udp_packet), 0, (struct sockaddr*)&udp_servaddr, sizeof(udp_servaddr)) == -1) {
            perror("UDP Sendto error");
            printf("%s\n", strerror(errno));
            exit(1);
        }
    }
    // printf("Sent train!\n");
    return 1;
}

