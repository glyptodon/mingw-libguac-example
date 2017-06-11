
CC=i686-w64-mingw32-gcc
CFLAGS=
LDFLAGS=-lguac -lguacd -lwsock32

all: bin/ball.exe

daemon.o: daemon.c
	$(CC) $(CFLAGS) -c daemon.c

bin/ball.exe: daemon.o
	$(CC) daemon.o $(LDFLAGS) -o bin/ball.exe

