OBJS = zip.o test.o
CC = c99
CFLAGS = -c

exe:	$(OBJS)
	$(CC) $(OBJS) -o exe

zip.o:	zip.c
	$(CC) $(CFLAGS) zip.c

test.o:	zip.o test.c
	$(CC) $(CFLAGS) test.c

