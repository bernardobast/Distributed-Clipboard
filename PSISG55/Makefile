CC=gcc
CFLAGS=-Wall

all: clipboard app_teste write_bomb

clipboard: clipboard.o threads.o 
	$(CC) $(CFLAGS) clipboard.c threads.c -o clipboard -lpthread

app_teste: app_teste.o library.o
	$(CC) $(CFLAGS) app_teste.c library.c -o app_teste

write_bomb: write_bomb.o library.o 
	$(CC) $(CFLAGS) write_bomb.c library.c -o write_bomb
