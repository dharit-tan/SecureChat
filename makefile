CC=gcc
CFLAGS= -g

all: reed client

reed: reed.c
	$(CC) $(CCFLAGS) reed.c -o reed

client: client.c
	$(CC) $(CCFLAGS) client.c -o client

clean:
	rm -f reed *.o