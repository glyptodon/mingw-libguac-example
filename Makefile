
CC=i686-w64-mingw32-gcc
CFLAGS=
LDFLAGS=-lguac -lguacd -lwsock32

all: bin/ball.exe

clean:
	$(RM) *.o

main.o: main.c ball.h socket-wsa.h
	$(CC) $(CFLAGS) -c main.c

socket-wsa.o: socket-wsa.c
	$(CC) $(CFLAGS) -c socket-wsa.c

ball.o: ball.c
	$(CC) $(CFLAGS) -c ball.c

bin/ball.exe: main.o ball.o socket-wsa.o
	$(CC) main.o ball.o socket-wsa.o $(LDFLAGS) -o bin/ball.exe

