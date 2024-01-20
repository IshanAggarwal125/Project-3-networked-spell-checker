all: client server

client: 
	gcc client.c -o client -Wall -Werror

server:
	gcc -o server server.c -Wall -Werror -lpthread

clean:
	rm *.o client server