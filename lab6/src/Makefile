# Сборка всех программ
all: client server

# Сборка клиента
client: client.o excr_3.o
	gcc -o client client.o excr_3.o -pthread

# Сборка сервера
server: server.o excr_3.o
	gcc -o server server.o excr_3.o -pthread

# Компиляция client.o
client.o: client.c excr_3.h
	gcc -Wall -Wextra -std=c99 -c client.c -o client.o

# Компиляция server.o
server.o: server.c excr_3.h
	gcc -Wall -Wextra -std=c99 -c server.c -o server.o

# Компиляция excr_3.o
excr_3.o: excr_3.c excr_3.h
	gcc -Wall -Wextra -std=c99 -c excr_3.c -o excr_3.o

# Очистка
clean:
	rm -f client.o server.o excr_3.o client server

# Объявляем, что all и clean не являются файлами
.PHONY: all clean