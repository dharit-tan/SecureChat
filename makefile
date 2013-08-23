CC=gcc
CFLAGS= -g

all: reed

reed: reed.c
	$(CC) $(CCFLAGS) reed.c -o reed

clean:
	rm -f reed *.o