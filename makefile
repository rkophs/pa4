all:
	gcc -pthread -o server/server_PFS server/server_PFS.c
	gcc -pthread -o client/client_PFS client/client_PFS.c
	gcc -pthread -o thread thread.c

clean:
	rm -rf client/client_PFS server/server_PFS thread
