all: app_cliente server

app_cliente: client.o
	gcc -pthread -o app_cliente client.o 

client.o: client.c
	gcc -c client.c

server: server.o
	gcc -pthread -o server server.o 

server.o: server.c
	gcc -c server.c

clean:
	rm *.o app_client server
