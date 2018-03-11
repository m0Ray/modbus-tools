BINS=modbus-ping modbus-id modbus-read modbus-write

all: ${BINS}

modbus-ping: modbus-ping.c modbustools.h
	gcc -o modbus-ping -lmodbus modbus-ping.c

modbus-id: modbus-id.c modbustools.h
	gcc -o modbus-id -lmodbus modbus-id.c

modbus-read: modbus-read.c modbustools.h
	gcc -ggdb -o modbus-read -lmodbus modbus-read.c

modbus-write: modbus-write.c modbustools.h
	gcc -ggdb -o modbus-write -lmodbus modbus-write.c

install: ${BINS}
	for f in $(BINS); do cp $$f /usr/bin; done

clean:
	for f in $(BINS); do rm $$f; done
