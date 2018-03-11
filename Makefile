BINS=modbus-ping modbus-id modbus-read modbus-write
CC=gcc
#CFLAGS=-ggdb
CFLAGS=-O3
LDFLAGS=-lmodbus

all: ${BINS}

modbus-ping: modbus-ping.c modbustools.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o modbus-ping modbus-ping.c

modbus-id: modbus-id.c modbustools.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o modbus-id modbus-id.c

modbus-read: modbus-read.c modbustools.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o modbus-read modbus-read.c

modbus-write: modbus-write.c modbustools.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o modbus-write modbus-write.c

install: ${BINS}
	for f in $(BINS); do cp $$f /usr/bin; done

clean:
	for f in $(BINS); do rm $$f; done
