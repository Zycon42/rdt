# Makefile k projektu c.3 predmetu IPK
# Jan Dusek 2011 <xdusek17@stud.fit.vutbr.cz>

CLIENT=rdtclient
SERVER=rdtserver
CC=gcc
CFLAGS=-std=gnu99 -pedantic -Wall -g
LDFLAGS=

# client object files
CL_OBJFILES=client.o args.o rdt.o checksum.o queue.o
# server object files
SE_OBJFILES=server.o args.o rdt.o checksum.o queue.o

.PHONY: all clean dep

all: $(CLIENT) $(SERVER)

-include dep.list

$(CLIENT): $(CL_OBJFILES)
	$(CC) $(LDFLAGS) -o $(CLIENT) $(CL_OBJFILES)

$(SERVER): $(SE_OBJFILES)
	$(CC) $(LDFLAGS) -o $(SERVER) $(SE_OBJFILES)

dep:
	$(CC) -MM *.c > dep.list

clean:
	rm -f *.o $(NAME)