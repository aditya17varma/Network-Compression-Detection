CC=gcc
CFLAGS=-I.
DEPS = cJSON.h
OBJ = json_parser.o cJSON.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

json_parser: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJ)