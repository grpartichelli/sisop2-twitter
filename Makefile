all: app_client server

app_client: client.o
	gcc -pthread -o app_client client.o 

client.o: client.c
	gcc -c client.c

server: server.o
	gcc -pthread -o server server.o 

server.o: server.c
	gcc -c server.c

clean:
	rm *.o app_client server
