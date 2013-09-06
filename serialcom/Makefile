CC=gcc

objects = serialcom.o uart.o hdlc.o

serialcom:$(objects)
	$(CC) -g -o serialcom $(objects) -lpthread
serialcom.o:command.h hdlc.h serialcom.h
	$(CC) -c -g serialcom.c
uart.o:uart.h
	$(CC) -c -g uart.c 
hdlc.o: hdlc.h
	$(CC) -c -g hdlc.c

.PHONY: clean
clean:
	-rm serialcom $(objects)
