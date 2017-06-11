
CC=i686-w64-mingw32-gcc
CFLAGS=
LDFLAGS=-lguac -lguacd -lwsock32

all: bin/ball.exe

clean:
	$(RM) *.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

bin/ball.exe: main.o
	$(CC) main.o $(LDFLAGS) -o bin/ball.exe

