all:	
	gcc -o  webserver webserver.c -std=gnu99
	gcc -o client client.c -std=gnu99