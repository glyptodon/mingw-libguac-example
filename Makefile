
CC=i686-w64-mingw32-gcc
CFLAGS=-Werror
LDFLAGS=-lguac -lguacd -lwsock32

all: bin/ball.exe ball.zip

clean:
	$(RM) *.o bin/ball.exe ball.zip

main.o: main.c ball.h
	$(CC) $(CFLAGS) -c main.c

ball.o: ball.c
	$(CC) $(CFLAGS) -c ball.c

bin/ball.exe: main.o ball.o
	$(CC) main.o ball.o $(LDFLAGS) -o bin/ball.exe

ball.zip: bin/ball.exe
	zip -j ball.zip bin/*

