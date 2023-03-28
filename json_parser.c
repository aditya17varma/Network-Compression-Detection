#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

int main() {
    const char *json_string = "{\"name\": \"John\", \"age\": 30}";
    FILE *fp = fopen("../config.json", "r");

    char buffer[1024];
    fread(buffer, 1, 1024, fp);
    fclose(fp);
    
    cJSON *root = cJSON_Parse(buffer);



    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        exit(EXIT_FAILURE);
    }

    cJSON *Server_IP = cJSON_GetObjectItemCaseSensitive(root, "Server_IP");
    if (cJSON_IsNumber(Server_IP)) {
        printf("Server IP: %d\n", Server_IP->valueint);
    }

    cJSON *Source_UDP_Port = cJSON_GetObjectItemCaseSensitive(root, "Source_UDP_Port");
    if (cJSON_IsNumber(Source_UDP_Port)) {
        printf("Source_UDP_Port: %d\n", Source_UDP_Port->valueint);
    }

    cJSON *Dest_UDP_Port = cJSON_GetObjectItemCaseSensitive(root, "Dest_UDP_Port");
    if (cJSON_IsNumber(Dest_UDP_Port)) {
        printf("Dest_UDP_Port: %d\n", Dest_UDP_Port->valueint);
    }

    cJSON *Dest_Port_TCP_Head_SYN = cJSON_GetObjectItemCaseSensitive(root, "Dest_Port_TCP_Head_SYN");
    if (cJSON_IsNumber(Dest_Port_TCP_Head_SYN)) {
        printf("Dest_Port_TCP_Head_SYN: %d\n", Dest_Port_TCP_Head_SYN->valueint);
    }

    cJSON *Dest_Port_TCP_Tail_SYN = cJSON_GetObjectItemCaseSensitive(root, "Dest_Port_TCP_Tail_SYN");
    if (cJSON_IsNumber(Dest_Port_TCP_Tail_SYN)) {
        printf("Dest_Port_TCP_Tail_SYN: %d\n", Dest_Port_TCP_Tail_SYN->valueint);
    }

    cJSON *TCP_Port = cJSON_GetObjectItemCaseSensitive(root, "TCP_Port");
    if (cJSON_IsNumber(TCP_Port)) {
        printf("TCP_Port: %d\n", TCP_Port->valueint);
    }

    cJSON *UDP_Payload_Size = cJSON_GetObjectItemCaseSensitive(root, "UDP_Payload_Size");
    if (cJSON_IsNumber(UDP_Payload_Size)) {
        printf("UDP_Payload_Size: %d\n", UDP_Payload_Size->valueint);
    }

    cJSON *Inter_Measurement_Time = cJSON_GetObjectItemCaseSensitive(root, "Inter_Measurement_Time");
    if (cJSON_IsNumber(Inter_Measurement_Time)) {
        printf("Inter_Measuerment_Time: %d\n", Inter_Measurement_Time->valueint);
    }  

    cJSON *Number_UDP_Packets = cJSON_GetObjectItemCaseSensitive(root, "Number_UDP_Packets");
    if (cJSON_IsNumber(Number_UDP_Packets)) {
        printf("Number_UDP_Packets: %d\n", Number_UDP_Packets->valueint);
    }  

    cJSON *UDP_TTL = cJSON_GetObjectItemCaseSensitive(root, "UDP_TTL");
    if (cJSON_IsNumber(UDP_TTL)) {
        printf("UDP_TTL: %d\n", UDP_TTL->valueint);
    }


    cJSON_Delete(root);
    return 0;
}

