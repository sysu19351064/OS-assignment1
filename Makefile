.PHONY:all
all:client server dph 

client:client.c
	gcc -pthread -o client client.c -lm
server:server.c
	gcc -pthread -o server server.c -lm
dph:dph.c
	gcc -pthread -o dph dph.c -lm
.PHONY:clean
clean:
	rm -f server client myfifo dph

