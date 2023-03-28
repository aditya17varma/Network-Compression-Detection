CC=gcc
CFLAGS=-I.
DEPS = cJSON.h
inter = obj
target = bin

OBJC: $(inter)/client.o $(inter)/cJSON.c 
OBJS: $(inter)/server.o $(inter)/cJSON.c 
OBJR: $(inter)/raw_client.o $(inter)/cJSON.c 

all: client server raw_client

client: $(OBJC) | $(target)
	$(CC) $(CFLAGS) $(OBJC) -o$(target)/client
server: $(OBJS) | $(target)
	$(CC) $(CFLAGS) $(OBJS) -o$(target)/client
raw_server: $(OBJR) | $(target)
	$(CC) $(CFLAGS) $(OBJR) -o$(target)/client

$(inter)/client.o: cJSON.h | $(inter)
	$(CC) $(CFLAGS) -c client.c -o $(inter)/client.o
$(inter)/server.o: cJSON.h | $(inter)
	$(CC) $(CFLAGS) -c server.c -o $(inter)/server.o
$(inter)/raw_client.o: cJSON.h | $(inter)
	$(CC) $(CFLAGS) -c raw_client.c -o $(inter)/raw_client.o
$(inter)/cJSON.o: | $(inter)
	$(CC) $(CFLAGS) -c cJSON.c -o $(inter)/cJOSN.o

$(target):
	mkdir $@

$(bin):
	mkdir $@

.PHONY: clean

clean:
	rm obj/* bin/* ||: