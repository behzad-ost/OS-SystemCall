client,server: server.c utils.o heartbeat.o
	gcc server.c utils.o heartbeat.o -o server
	gcc client.c -o client

utils.o: utils.c
	gcc -c utils.c

heartbeat.o: heartbeat.c
	gcc -c heartbeat.c