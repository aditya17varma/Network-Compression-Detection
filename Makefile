CC=gcc
CFLAGS=-I.
DEPS=cJSON.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: client server raw_client

client: client.o cJSON.o
	$(CC) $(CFLAGS) $^  -o $@ 
server: server.o cJSON.o
	$(CC) $(CFLAGS) $^ -o $@
raw_client: raw_client.o cJSON.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f *.o

clean-all: clean
	rm -f client server raw_client