OBJS = zip.o test.o
CC = gcc
CFLAGS = -c

exe:	$(OBJS)
	$(CC) $(OBJS) -o exe

zip.o:	zip.cpp
	$(CC) $(CFLAGS) zip.cpp

test.o:	zip.o
	$(CC) $(CFLAGS) test.cpp

