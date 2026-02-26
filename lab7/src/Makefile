# Сборка всех программ 7-й лабораторной

all: tcpclient tcpserver udpclient udpserver

tcpclient: tcpclient.c
	gcc -Wall -Wextra -std=c99 -o tcpclient tcpclient.c

tcpserver: tcpserver.c
	gcc -Wall -Wextra -std=c99 -o tcpserver tcpserver.c

udpclient: udpclient.c
	gcc -Wall -Wextra -std=c99 -o udpclient udpclient.c

udpserver: udpserver.c
	gcc -Wall -Wextra -std=c99 -o udpserver udpserver.c

clean:
	rm -f tcpclient tcpserver udpclient udpserver

.PHONY: all clean