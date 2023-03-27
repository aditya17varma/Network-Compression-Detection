#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>          
#include <string.h>           

#include <netdb.h>            
#include <sys/types.h>        
#include <sys/socket.h>       
#include <netinet/in.h>       
#include <netinet/ip.h>       
#define __FAVOR_BSD           
#include <netinet/tcp.h>      
#include <arpa/inet.h>        
#include <sys/ioctl.h>        
#include <bits/ioctls.h>      
#include <net/if.h>           
#include <errno.h>    
#include <threads.h>  
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include "cJSON.h"

// Define some constants.
#define IP4_HDRLEN 20         // IPv4 header length
#define TCP_HDRLEN 20         // TCP header length, excludes options data

// Function prototypes
uint16_t checksum (uint16_t *, int);
uint16_t tcp4_checksum (struct ip, struct tcphdr);
char *allocate_strmem (int);
uint8_t *allocate_ustrmem (int);
int *allocate_intmem (int);

int setup_raw_tcp_socket(int);
int setup_udp_socket(int);
int send_syn(int, struct sockaddr_in, char*, char*, int, int );
int send_udp_trains(int, struct sockaddr_in, int, int, bool);
long double receive_rst(int, struct sockaddr_in, int, int, int);
void* thread_function (void *);

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

struct thread_data {
    int sockfd;
    struct sockaddr_in servaddr;
    int source_port;
    int dest_port;
    int dest_port2;
    long double rst_delta;
};

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


int main (int argc, char **argv) {
  int status, sockfd, udp_sockfd;
  char *src_ip, *dst_ip;
  // struct addrinfo hints, *res;
  struct sockaddr_in *ipv4, servaddr, udp_servaddr;
  struct ifreq ifr;
  void *tmp;

  struct config_struct *config = parse_JSON();
  in_addr_t server_ip = config->Server_IP;
  int tcp_port = config->TCP_Port;
  int udp_dest_port = config->Dest_UDP_Port;
  int payload_size = config->UDP_Payload_Size;
  int num_packets = config->Number_UDP_Packets;
  int udp_ttl = config->UDP_TTL;
  int inter_mes = config->Inter_Measurement_Time;
  int tcp_head = config->Dest_Port_TCP_Head_SYN;
  int tcp_tail = config->Dest_Port_TCP_Tail_SYN;
  printf("Config num_packets: %d\n", num_packets);

  // Allocate memory for various arrays.
  src_ip = allocate_strmem (INET_ADDRSTRLEN);
  

  // Source IPv4 address: you need to fill this out
  strcpy (src_ip, "192.168.64.3");
  printf("src_ip: %s\n", src_ip);

  int source_port = tcp_port;
  int dest_port = tcp_head;
  int dest_port2 = tcp_tail;
  int recvfrom_timeout = inter_mes * 0.75;
  printf("Inter Mes: %d Timeout: %d\n", inter_mes, recvfrom_timeout);

  pthread_t recv_thread;

  struct in_addr dest_addr;
  dest_addr.s_addr = server_ip;
  dst_ip = inet_ntoa(dest_addr);
  // struct in_addr src_addr;
  // src_addr.s_addr = INADDR_ANY;
  // printf("src_ip: %s, ntoa_ip: %s\n", src_ip, inet_ntoa(src_addr));

  //RAW SERVADDR
  memset (&servaddr, 0, sizeof (struct sockaddr_in));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(dst_ip);

  // UDP SERVADDR
  // assign UDP IP, PORT
  memset(&udp_servaddr, 0, sizeof(udp_servaddr));
  udp_servaddr.sin_family = AF_INET;
  udp_servaddr.sin_addr.s_addr = inet_addr(dst_ip);
  udp_servaddr.sin_port = htons(9999);
  printf("Client UDP IP: %d\n", server_ip);

  sockfd = setup_raw_tcp_socket(recvfrom_timeout);
  udp_sockfd = setup_udp_socket(udp_ttl);

  //START RECVFROM THREAD
  //create struct to pass info to thread
  struct thread_data thrd_data = {
    .sockfd = sockfd,
    .servaddr = servaddr,
    .source_port = source_port,
    .dest_port = dest_port,
    .dest_port2 = dest_port2
  };

  // TRAIN 1
  printf("Strating recvfrom thread...\n");
  pthread_create(&recv_thread, NULL, thread_function , (void *)&thrd_data);
  printf("Called pthread_create\n");

  bool high_entropy = false;

  //HEAD SYN
  int s = send_syn(sockfd, servaddr, src_ip, dst_ip, source_port, dest_port);
  if (s == 1){
    printf("Send_syn worked %d\n", dest_port);
  }
  else {
    perror("Send_syn");
  }

  // UDP TRAINS choo choo
  int u1 = send_udp_trains(udp_sockfd, udp_servaddr, num_packets, payload_size, high_entropy);
  if (u1 < 0){
    perror("Send UDP Train error...");
    exit(1);
  }

  close(udp_sockfd);

  //TAIL SYN
  int s2 = send_syn(sockfd, servaddr, src_ip, dst_ip, source_port, dest_port2);
  if (s2 == 1){
    printf("Send_syn worked %d\n", dest_port2);
  }
  else {
    perror("Send_syn");
  }

  pthread_join(recv_thread, NULL);
  printf("Thread exited and returned: %Lf\n", thrd_data.rst_delta);
  
  // Close socket descriptor.
  close (sockfd);

  sleep(inter_mes);

  //TRAIN 2
  sockfd = setup_raw_tcp_socket(recvfrom_timeout);
  udp_sockfd = setup_udp_socket(udp_ttl);

  //START RECVFROM THREAD
  //create struct to pass info to thread
  struct thread_data thrd_data2 = {
    .sockfd = sockfd,
    .servaddr = servaddr,
    .source_port = source_port,
    .dest_port = dest_port,
    .dest_port2 = dest_port2
  };

  printf("Strating recvfrom thread...\n");
  pthread_create(&recv_thread, NULL, thread_function , (void *)&thrd_data2);
  printf("Called pthread_create\n");

  high_entropy = true;

  //HEAD SYN
  s = send_syn(sockfd, servaddr, src_ip, dst_ip, source_port, dest_port);
  if (s == 1){
    printf("Send_syn worked %d\n", dest_port);
  }
  else {
    perror("Send_syn");
  }

  // UDP TRAINS choo choo
  int u2 = send_udp_trains(udp_sockfd, udp_servaddr, num_packets, payload_size, high_entropy);
  if (u2 < 0){
    perror("Send UDP Train error...");
    exit(1);
  }

  close(udp_sockfd);

  //TAIL SYN
  s2 = send_syn(sockfd, servaddr, src_ip, dst_ip, source_port, dest_port2);
  if (s2 == 1){
    printf("Send_syn worked %d\n", dest_port2);
  }
  else {
    perror("Send_syn");
  }

  pthread_join(recv_thread, NULL);
  printf("Thread exited and returned: %Lf\n", thrd_data2.rst_delta);
  
  // Close socket descriptor.
  close (sockfd);

  printf("Delta1: %Lf\n", thrd_data.rst_delta);
  printf("Delta2: %Lf\n", thrd_data2.rst_delta);
  printf("Total Delta: %d\n", abs(thrd_data2.rst_delta - thrd_data.rst_delta));


  free (src_ip);

  return (EXIT_SUCCESS);
}

int setup_raw_tcp_socket(int recv_timeout) {
  
  int sockfd;
  if ((sockfd = socket (AF_INET, SOCK_RAW, IPPROTO_TCP)) < 0) {
    perror ("socket() failed ");
    printf("%s\n",strerror(errno));
    exit (EXIT_FAILURE);
  }
  else{
    printf("Socket created\n");
  }

  // Set flag so socket expects us to provide IPv4 header.
  const int on = 1;
  if (setsockopt (sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof (on)) < 0) {
    perror ("setsockopt() failed to set IP_HDRINCL ");
    exit (EXIT_FAILURE);
  }

  struct timeval timeout;
  timeout.tv_sec = recv_timeout;
  timeout.tv_usec = 0;
  if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    perror("Couldn't set RCV Timeout");
    exit(1);
  }
  else {
    printf("Recvfrom timeout set to: %ld\n", timeout.tv_sec);
  }

  return sockfd;

}

int setup_udp_socket(int udp_ttl){
  int udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (udp_sockfd < 0) {
      perror("UDP Client socket error");
      exit(1);
  }
  else {
      printf("UDP Client socket created\n");
  }

  int df_val = IP_PMTUDISC_DO;
  if (setsockopt(udp_sockfd, IPPROTO_IP, IP_MTU_DISCOVER, &df_val, sizeof(df_val)) < 0){
      perror("UDP setsockopt DF failed");
      exit(1);
  }

  if (setsockopt(udp_sockfd, IPPROTO_IP, IP_TTL, &udp_ttl, sizeof(udp_ttl)) < 0){
    perror("UDP setsockopt TTL failed");
  }

  return udp_sockfd;

}

int send_syn(int sockfd, struct sockaddr_in servaddr, char* source_ip, char* dest_ip, int source_port, int dest_port) {
  struct ip iphdr;
  struct tcphdr tcphdr;
  uint8_t *packet;
  int i, status, *ip_flags, *tcp_flags;

  ip_flags = allocate_intmem (4);
  tcp_flags = allocate_intmem (8);
  packet = allocate_ustrmem (IP_MAXPACKET);
  
  // IPv4 header
  iphdr.ip_hl = IP4_HDRLEN / sizeof (uint32_t); // IPv4 header length (4 bits): Number of 32-bit words in header = 5
  iphdr.ip_v = 4; // Internet Protocol version (4 bits): IPv4
  iphdr.ip_tos = 0; // Type of service (8 bits)
  iphdr.ip_len = htons (IP4_HDRLEN + TCP_HDRLEN); // Total length of datagram (16 bits): IP header + TCP header  
  iphdr.ip_id = htons (0); // ID sequence number (16 bits): unused, since single datagram
  // Flags, and Fragmentation offset (3, 13 bits): 0 since single datagram
  ip_flags[0] = 0; // Zero (1 bit)
  ip_flags[1] = 0; // Do not fragment flag (1 bit)
  ip_flags[2] = 0; // More fragments following flag (1 bit)
  ip_flags[3] = 0; // Fragmentation offset (13 bits)
  iphdr.ip_off = htons ((ip_flags[0] << 15)
                      + (ip_flags[1] << 14)
                      + (ip_flags[2] << 13)
                      +  ip_flags[3]);
  iphdr.ip_ttl = 255; // Time-to-Live (8 bits): default to maximum value
  iphdr.ip_p = IPPROTO_TCP; // Transport layer protocol (8 bits): 6 for TCP
  
  // Source IPv4 address (32 bits)
  if ((status = inet_pton (AF_INET, source_ip, &(iphdr.ip_src))) != 1) {
    fprintf (stderr, "inet_pton() failed for source address.\nError message: %s", strerror (status));
    exit (EXIT_FAILURE);
  }
  printf("iphdr src_ip: %d\n", iphdr.ip_src.s_addr);

  // Destination IPv4 address (32 bits)
  if ((status = inet_pton (AF_INET, dest_ip, &(iphdr.ip_dst))) != 1) {
    fprintf (stderr, "inet_pton() failed for destination address.\nError message: %s", strerror (status));
    exit (EXIT_FAILURE);
  }
  // IPv4 header checksum (16 bits): set to 0 when calculating checksum
  iphdr.ip_sum = 0;
  iphdr.ip_sum = checksum ((uint16_t *) &iphdr, IP4_HDRLEN);

  // TCP header
  tcphdr.th_sport = htons (source_port); // Source port number (16 bits)
  tcphdr.th_dport = htons (dest_port); // Destination port number (16 bits)
  tcphdr.th_seq = htonl (0); // Sequence number (32 bits)
  tcphdr.th_ack = htonl (0); // Acknowledgement number (32 bits): 0 in first packet of SYN/ACK process
  tcphdr.th_x2 = 0; // Reserved (4 bits): should be 0
  tcphdr.th_off = TCP_HDRLEN / 4; // Data offset (4 bits): size of TCP header in 32-bit words
  // Flags (8 bits)
  tcp_flags[0] = 0; // FIN flag (1 bit)
  tcp_flags[1] = 1; // SYN flag (1 bit): set to 1
  tcp_flags[2] = 0; // RST flag (1 bit)
  tcp_flags[3] = 0; // PSH flag (1 bit)
  tcp_flags[4] = 0; // ACK flag (1 bit)
  tcp_flags[5] = 0; // URG flag (1 bit)
  tcp_flags[6] = 0; // ECE flag (1 bit)
  tcp_flags[7] = 0; // CWR flag (1 bit)
  tcphdr.th_flags = 0;
  for (i=0; i<8; i++) {
    tcphdr.th_flags += (tcp_flags[i] << i);
  }
  tcphdr.th_win = htons (1460); // Window size (16 bits)
  tcphdr.th_urp = htons (0); // Urgent pointer (16 bits): 0 (only valid if URG flag is set)
  tcphdr.th_sum = tcp4_checksum (iphdr, tcphdr); // TCP checksum (16 bits)

  // Prepare packet.
  // First part is an IPv4 header.
  memcpy (packet, &iphdr, IP4_HDRLEN * sizeof (uint8_t));

  // Next part of packet is upper layer protocol header.
  memcpy ((packet + IP4_HDRLEN), &tcphdr, TCP_HDRLEN * sizeof (uint8_t));

  // SEND PACKET
  if (sendto (sockfd, packet, IP4_HDRLEN + TCP_HDRLEN, 0, (struct sockaddr *) &servaddr, sizeof (struct sockaddr)) < 0)  {
    perror ("sendto() failed ");
    exit (EXIT_FAILURE);
  }
  else {
    printf("Packet sent\n");
  }

  free (packet);
  free (ip_flags);
  free (tcp_flags);

  return 1;
}

int send_udp_trains(int udp_sockfd, struct sockaddr_in udp_servaddr, int num_packets, int payload_size, bool high_entropy) {
  // get high-entropy data ready
  struct udp_payload{
      unsigned short packet_id;
      char payload[payload_size - 2];
  };
  struct udp_payload udp_packet;
  FILE *random_fp = fopen("../random_file.txt", "r");
  char rand_buffer[payload_size];
  printf("Trying to fgets random file\n");
  fgets(rand_buffer, sizeof(rand_buffer), random_fp);
  printf("Rand array filled\n");

  for (int i = 0; i < num_packets; i++){
      // set packet id
      udp_packet.packet_id = (unsigned short)i;
      // clear packet payload
      memset(udp_packet.payload, 0, payload_size -2);
      if (high_entropy){
        strncpy(udp_packet.payload, rand_buffer, payload_size - 2);
      }
      if (sendto(udp_sockfd, &udp_packet, sizeof(udp_packet), 0, (struct sockaddr*)&udp_servaddr, sizeof(udp_servaddr)) == -1) {
          perror("UDP Sendto error");
          printf("%s\n", strerror(errno));
      }
  }
  printf("Sent train!\n");
  return 1;
}

long double receive_rst(int sockfd, struct sockaddr_in servaddr, int source_port, int dest_port, int dest_port2) {
  socklen_t addrlen = sizeof(servaddr);
  char recv_buffer[1500];
  struct timeval rst_start, rst_end;
  bool first_rst_captured = false;
  
  while(1){
    ssize_t bytes_received = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&servaddr, &addrlen);
    if (bytes_received < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK){
        printf("RECV timeout\n");
        break;
      }
      else {
        perror("recvfrom erro");
        exit(1);
      }
    }
    unsigned char *recv_packet = &recv_buffer;
    struct iphdr *ip_recv = (struct iphdr*)recv_packet;
    struct tcphdr *tcp_recv = (struct tcphdr*)(recv_packet + ip_recv->ihl*4);
    int tcp_recv_destport = tcp_recv->th_dport;
    if (tcp_recv->th_flags & TH_RST){
      printf("Bytes Captured: %ld\n", bytes_received);
      printf("RST found\n");
      printf("Sent to: %d\n", ntohs(tcp_recv->th_dport));
      printf("Sent from: %d\n", ntohs(tcp_recv->th_sport));
      if (!first_rst_captured){
        printf("Found first RST\n");
        gettimeofday(&rst_start, NULL);
        first_rst_captured = true;
      }
      else {
        printf("Found second RST\n");
        gettimeofday(&rst_end, NULL);
        break;
      }
    }
    
    bzero(recv_buffer, sizeof(recv_buffer));
  }

  printf("RST Start: sec: %ld usec:%ld\n", rst_start.tv_sec, rst_start.tv_usec);
  printf("RST End: sec: %ld usec:%ld\n", rst_end.tv_sec, rst_end.tv_usec);
  long double sec_diff = rst_end.tv_sec - rst_start.tv_sec;
  printf("Sec_diff: %Lf\n", sec_diff);
  long double usec_diff = rst_end.tv_usec - rst_start.tv_usec;
  printf("uSec_diff: %Lf\n", usec_diff);
  long double rst_delta = ((rst_end.tv_sec - rst_start.tv_sec) * 1000 + (rst_end.tv_usec - rst_start.tv_usec) / 1000) ;
  return rst_delta;

}

void* thread_function (void *arg) {
  struct thread_data *t_data = (struct thread_data *) arg;
  printf("Check Thread source_port: %d\n", t_data->source_port);
  int sockfd = t_data->sockfd;
  struct sockaddr_in servaddr = t_data->servaddr;
  printf("Checking servaddr ip: %d\n",servaddr.sin_addr.s_addr);
  int source_port = t_data->source_port;
  int dest_port = t_data->dest_port;
  int dest_port2 = t_data->dest_port2;

  long double result = receive_rst(sockfd, servaddr, source_port, dest_port, dest_port2);

  t_data->rst_delta = result;
  pthread_exit(NULL);
  // return (void *)rst_delta;
}

// Computing the internet checksum (RFC 1071).
// Note that the internet checksum is not guaranteed to preclude collisions.
uint16_t
checksum (uint16_t *addr, int len) {

  int count = len;
  register uint32_t sum = 0;
  uint16_t answer = 0;

  // Sum up 2-byte values until none or only one byte left.
  while (count > 1) {
    sum += *(addr++);
    count -= 2;
  }

  // Add left-over byte, if any.
  if (count > 0) {
    sum += *(uint8_t *) addr;
  }

  // Fold 32-bit sum into 16 bits; we lose information by doing this,
  // increasing the chances of a collision.
  // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  // Checksum is one's compliment of sum.
  answer = ~sum;

  return (answer);
}

// Build IPv4 TCP pseudo-header and call checksum function.
uint16_t
tcp4_checksum (struct ip iphdr, struct tcphdr tcphdr) {

  uint16_t svalue;
  char buf[IP_MAXPACKET], cvalue;
  char *ptr;
  int chksumlen = 0;

  // ptr points to beginning of buffer buf
  ptr = &buf[0];

  // Copy source IP address into buf (32 bits)
  memcpy (ptr, &iphdr.ip_src.s_addr, sizeof (iphdr.ip_src.s_addr));
  ptr += sizeof (iphdr.ip_src.s_addr);
  chksumlen += sizeof (iphdr.ip_src.s_addr);

  // Copy destination IP address into buf (32 bits)
  memcpy (ptr, &iphdr.ip_dst.s_addr, sizeof (iphdr.ip_dst.s_addr));
  ptr += sizeof (iphdr.ip_dst.s_addr);
  chksumlen += sizeof (iphdr.ip_dst.s_addr);

  // Copy zero field to buf (8 bits)
  *ptr = 0; ptr++;
  chksumlen += 1;

  // Copy transport layer protocol to buf (8 bits)
  memcpy (ptr, &iphdr.ip_p, sizeof (iphdr.ip_p));
  ptr += sizeof (iphdr.ip_p);
  chksumlen += sizeof (iphdr.ip_p);

  // Copy TCP length to buf (16 bits)
  svalue = htons (sizeof (tcphdr));
  memcpy (ptr, &svalue, sizeof (svalue));
  ptr += sizeof (svalue);
  chksumlen += sizeof (svalue);

  // Copy TCP source port to buf (16 bits)
  memcpy (ptr, &tcphdr.th_sport, sizeof (tcphdr.th_sport));
  ptr += sizeof (tcphdr.th_sport);
  chksumlen += sizeof (tcphdr.th_sport);

  // Copy TCP destination port to buf (16 bits)
  memcpy (ptr, &tcphdr.th_dport, sizeof (tcphdr.th_dport));
  ptr += sizeof (tcphdr.th_dport);
  chksumlen += sizeof (tcphdr.th_dport);

  // Copy sequence number to buf (32 bits)
  memcpy (ptr, &tcphdr.th_seq, sizeof (tcphdr.th_seq));
  ptr += sizeof (tcphdr.th_seq);
  chksumlen += sizeof (tcphdr.th_seq);

  // Copy acknowledgement number to buf (32 bits)
  memcpy (ptr, &tcphdr.th_ack, sizeof (tcphdr.th_ack));
  ptr += sizeof (tcphdr.th_ack);
  chksumlen += sizeof (tcphdr.th_ack);

  // Copy data offset to buf (4 bits) and
  // copy reserved bits to buf (4 bits)
  cvalue = (tcphdr.th_off << 4) + tcphdr.th_x2;
  memcpy (ptr, &cvalue, sizeof (cvalue));
  ptr += sizeof (cvalue);
  chksumlen += sizeof (cvalue);

  // Copy TCP flags to buf (8 bits)
  memcpy (ptr, &tcphdr.th_flags, sizeof (tcphdr.th_flags));
  ptr += sizeof (tcphdr.th_flags);
  chksumlen += sizeof (tcphdr.th_flags);

  // Copy TCP window size to buf (16 bits)
  memcpy (ptr, &tcphdr.th_win, sizeof (tcphdr.th_win));
  ptr += sizeof (tcphdr.th_win);
  chksumlen += sizeof (tcphdr.th_win);

  // Copy TCP checksum to buf (16 bits)
  // Zero, since we don't know it yet
  *ptr = 0; ptr++;
  *ptr = 0; ptr++;
  chksumlen += 2;

  // Copy urgent pointer to buf (16 bits)
  memcpy (ptr, &tcphdr.th_urp, sizeof (tcphdr.th_urp));
  ptr += sizeof (tcphdr.th_urp);
  chksumlen += sizeof (tcphdr.th_urp);

  return checksum ((uint16_t *) buf, chksumlen);
}

// Allocate memory for an array of chars.
char *
allocate_strmem (int len) {

  void *tmp;

  if (len <= 0) {
    fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_strmem().\n", len);
    exit (EXIT_FAILURE);
  }

  tmp = (char *) malloc (len * sizeof (char));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (char));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_strmem().\n");
    exit (EXIT_FAILURE);
  }
}

// Allocate memory for an array of unsigned chars.
uint8_t *
allocate_ustrmem (int len) {

  void *tmp;

  if (len <= 0) {
    fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_ustrmem().\n", len);
    exit (EXIT_FAILURE);
  }

  tmp = (uint8_t *) malloc (len * sizeof (uint8_t));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (uint8_t));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_ustrmem().\n");
    exit (EXIT_FAILURE);
  }
}

// Allocate memory for an array of ints.
int *
allocate_intmem (int len) {

  void *tmp;

  if (len <= 0) {
    fprintf (stderr, "ERROR: Cannot allocate memory because len = %i in allocate_intmem().\n", len);
    exit (EXIT_FAILURE);
  }

  tmp = (int *) malloc (len * sizeof (int));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (int));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_intmem().\n");
    exit (EXIT_FAILURE);
  }
}