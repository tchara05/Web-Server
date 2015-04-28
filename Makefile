all: server.c
	gcc -pthread -g -o server server.c threadpool.c -I.