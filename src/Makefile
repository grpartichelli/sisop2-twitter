all: client server

client: client.o
	gcc -pthread -o client client.o -fsanitize=address

client.o: client.c
	gcc -c client.c -fsanitize=address

server: server.o
	gcc -pthread -o server server.o  -fsanitize=address

server.o: server.c
	gcc -c server.c -fsanitize=address


clean:
	rm *.o client server
