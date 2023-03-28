# Network Programming - Project1

Project to build a network application to detect if network compression is present on a network path, and if found, to locate the compression link.


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
### Build Server - Part1
`./server <TCP port>`<br>
Make sure it's the same TCP Port as in the config.json file!<br>
Here's an example:
```
./server 2000
```

### Build Client - Part1
`./client <config filename>`<br>
Make sure the filename has the proper path!
```
./client config.json
```

### Build RawClient - Part2
`sudo ./raw_client <config filename>` <br>
Make sure the filename has the proper path! Sudo required for raw socket permissions.
```
sudo ./raw_client config.json
```

## Requirements
This project requires the cJSON library to parse the config.json file.
cJSON.c and cJSON.h need to included in the project directory.

## Recommended Modules

## Installation

## Configuration

## Troubleshooting

