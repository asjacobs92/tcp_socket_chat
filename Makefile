all:
	gcc -include include/server.h src/server.c -o bin/server -lpthread
	gcc -include include/client.h src/client.c -o bin/client -lpthread
