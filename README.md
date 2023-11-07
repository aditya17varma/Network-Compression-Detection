# End-to-End Detection of Network Compression

Project to build a network application to detect if network compression is present on a network path. Part 1 is implemented using the TCP and UDP sockets. Part 2 is a standalone application using RAW_SOCKETS for TCP.

## Project Details
### Part 1: Client/Server Application
A client/server application where the client sends two sets of _i_ UDP packets back-to-back (called packet train). The reciver records the arrival time between the first and last packet in the train.
The first packet train consists of all packets of size _l_ bytes in payload (low-entropy), filled with all 0's, while the second packet train contains a random sequence of bits (high-entropy).

If the difference in arrival time between the first and last packets of the two trains (∆tH − ∆tL) is more than a fixed threshold τ = 100 ms,  we have detected compression.

Three phases:
1. **Pre-Probing TCP Connection:**</br>   
   The client application initiates a TCP connection to the server. Upon a successful connection, the client then passes all the configuration file’s contents to the server. The server will use this information in later phases to detect whether network compression is present or not. The TCP connection is released upon the successful transmission of the contents of the configuration file.
   
2. **Probing Phase:**</br>
   In this phase, the sender sends the UDP packets. After sending n UDP packets with low entropy data, the client must wait Inter-Measurement Time (γ) seconds before sending the packet train consists of n UDP packets with high entropy data. This is to ensure that the two probing phases will not interfere with one another.
   
3. **Post-Probing TCP Connection:**</br>
   After all packets are arrived to the server, the server performs the necessary calculations required to detect the presence of network compression on the path between the server and the client. After the probing phase, the client application initiates another TCP connection. Once the connection is established, the server sends its findings to the client.

### Part 2: Standalone Raw Sockets
A standalone application that detects network compression, without requiring cooperation from the server, other than responding to standard network events. This application also locates the link where the compression is applied.

Using a raw socket implementation, the program sends a single head SYN packet. The SYN packet is immediately followed by a train of n UDP packets, and a single tail SYN packet. The SYN packets are sent to two different ports (x and y) that are not among well-known ports, so they are expected to be inactive or closed ports that no application is listening to. Sending SYN packets to closed ports should trigger RST packets from the server. The application records the two RST packets’ arrival time. The difference between the arrival time of the RST packets is then used to determine whether network compression was detected or not. If either of the RST packets is lost or the server is not responding with an RST packet, the application will output Failed to detect due to insufficient information and terminates.

## Build
### Build using makefile:
Make all files (client, server, and raw_client):<br>
`make`

Make client:<br>
`make client`

Make server:<br>
`make server`

Make raw_client:<br>
`make raw_client`

### Build using GCC:
Client:<br>
`gcc client.c cJSON.c -I. -o client`

Server:<br>
`gcc server.c cJSON.c -I. -o server`

Raw_Client:<br>
`sudo gcc raw_client.c cJSON.c -I. -o raw_client`


## Run
For Part 1: run the server, and then the client.<br>
For Part 2: running the standalone application (raw_client) is enough.
### Run Server - Part1
`./server <TCP port>`<br>
Make sure it's the same TCP Port as in the config.json file!<br>
Here's an example:
```
./server 2000
```

### Run Client - Part1
`./client <config filename>`<br>
Make sure the filename has the proper path!
```
./client config.json
```

### Run RawClient - Part2
`sudo ./raw_client <config filename>` <br>
Make sure the filename has the proper path! Sudo required for raw socket permissions.
```
sudo ./raw_client config.json
```

## Requirements
This project requires the cJSON library to parse the config.json file.
cJSON.c and cJSON.h need to included in the project directory.

The config.json must be properly constructed. client.c and server.c require all fields but the 'Client_IP', raw_client.c requires all fields.
```
{
    "Server_IP": "192.168.64.2",
    "Client_IP": "192.168.64.3",
    "Source_UDP_Port": 5000,
    "Dest_UDP_Port": 8765,
    "Dest_Port_TCP_Head_SYN": 8080,
    "Dest_Port_TCP_Tail_SYN": 8081,
    "TCP_Port": 2000,
    "UDP_Payload_Size": 1000,
    "Inter_Measurement_Time": 15,
    "Number_UDP_Packets": 6000,
    "UDP_TTL": 255
}
```

## Implementation Notes
Currently, the server calls the gettimeofday() function to update the end timestamp for packet but the first. The delta could be potentially reduced if the end time is only updated for the last 10% of packets.

The recvfrom() in thread2 sometimes doesn't timestamp properly, even though it captures the RST.

## External Resources Used
P.D. Buchanan's [Sending a IPv4 TCP packet via raw sockets](https://www.pdbuchan.com/rawsock/tcp4.c)<br>
Used for implementing the checksum and headers in part2.

