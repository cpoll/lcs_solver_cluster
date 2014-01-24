CFLAGS = -Wall -O2 -pthread -g
CC = gcc
OBJS = lcs.out server.out
SRCS = lcs.c server.c

all: lcs server

lcs: 
	${CC} ${CFLAGS} lcs.c -o lcs.out

server:
	${CC} ${CFLAGS} server.c -o server.out

clean:
	rm lcs.out
	rm server.out
