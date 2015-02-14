OBJS = zip.o test.o
CC = gcc
CFLAGS = -c

exe:	$(OBJS)
	$(CC) $(OBJS) -o exe

zip.o:	zip.c
	$(CC) $(CFLAGS) zip.c

test.o:	zip.o
	$(CC) $(CFLAGS) test.c

