OBJS = zip.o comp.o test.o
CC = gcc
CFLAGS = -std=c99 -c

exe:	$(OBJS)
	$(CC) $(OBJS) -o exe

comp.o:	comp.c
	$(CC) $(CFLAGS) comp.c

zip.o:	comp.o zip.c
	$(CC) $(CFLAGS) zip.c

test.o:	zip.o test.c
	$(CC) $(CFLAGS) test.c

