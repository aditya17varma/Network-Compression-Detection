# Network Programming - Project1

Project to build a network application to detect if network compression is present on a network path. Part 1 is implemented using the TCP and UDP sockets. Part 2 is implemented using RAW_SOCKETS for TCP.

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

## External Resources Used
P.D. Buchanan's [Sending a IPv4 TCP packet via raw sockets](https://www.pdbuchan.com/rawsock/tcp4.c)<br>
Used for implementing the checksum and headers in part2.

