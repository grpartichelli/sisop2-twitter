all: app_cliente server

app_cliente: client.o
	gcc -pthread -o app_cliente client.o -fsanitize=address

client.o: client.c
	gcc -c client.c -fsanitize=address

server: server.o
	gcc -pthread -o server server.o  -fsanitize=address

server.o: server.c
	gcc -c server.c -fsanitize=address


clean:
	rm *.o app_cliente server
