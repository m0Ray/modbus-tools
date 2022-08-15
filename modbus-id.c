#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <error.h>

#include <modbus/modbus.h>
#include <modbus/modbus-rtu.h>

#include "modbustools.h"

void usage(char* self, int e) {
	fprintf(stderr, "Usage: %s [-d /dev/ttyX] [-r baudrate] [-b databits] [-p (N|E|O)] [-s stopbits] [-n retries] [-v] <bus_address>\n", self);
	fprintf(stderr, "Options description:\n\
  -d: UART device (default %s)\n\
  -r: UART baud rate: 9600, 19200, 57600, 115200 etc (default %d)\n\
  -b: UART data bits: 5-8 (default %d)\n\
  -p: UART parity check: None, Even, Odd (default %c)\n\
  -b: UART stop bits: 1-2 (default %d)\n\
  -n: retry count (default %d)\n\
  -v: verbose debug on stderr\n", MODBUSTOOLS_DEFAULT_UART, MODBUSTOOLS_DEFAULT_BAUDRATE, MODBUSTOOLS_DEFAULT_DATABITS, MODBUSTOOLS_DEFAULT_PARITY, MODBUSTOOLS_DEFAULT_STOPBITS, MODBUSTOOLS_DEFAULT_RETRIES);
	exit(e);
}

int main( int argc, char** argv ) {

// Configuration variables and defaults
	char* uart = MODBUSTOOLS_DEFAULT_UART;
	int  uart_baudrate = MODBUSTOOLS_DEFAULT_BAUDRATE;
	int  uart_databits = MODBUSTOOLS_DEFAULT_DATABITS;
	char uart_parity = MODBUSTOOLS_DEFAULT_PARITY;
	int  uart_stopbits = MODBUSTOOLS_DEFAULT_STOPBITS;
	int retry = MODBUSTOOLS_DEFAULT_RETRIES;
	int debug=0;

	long tmp;

// Parse commandline options
	int c;
	while ((c = getopt (argc, argv, "hd:r:b:p:s:t:n:v")) != -1) {
		switch (c) {
		case 'h':
			usage(argv[0], 0);
		case 'd':
			uart=optarg;
			break;
		case 'r':
			tmp=strtol(optarg, NULL, 0);
			uart_baudrate=(int)tmp;
			break;
		case 'b':
			tmp=strtol(optarg, NULL, 0);
			if(tmp>=5 && tmp<=8) {
				uart_databits=(int)tmp;
			} else {
				fprintf(stderr, "%s: invalid UART data bits setting -- %d\n", argv[0], tmp);
				exit(1);
			}
			break;
		case 'p':
			switch(*optarg) {
			case 'N':
			case 'E':
			case 'O':
				uart_parity=*optarg;
				break;
			default:
				fprintf(stderr, "%s: invalid UART parity setting -- '%s'\n", argv[0], optarg);
				exit(1);
			}
			break;
		case 's':
			tmp=strtol(optarg, NULL, 0);
			if(tmp==1 || tmp==2) {
				uart_stopbits=(int)tmp;
			} else {
				fprintf(stderr, "%s: invalid UART stop bits setting -- %d\n", argv[0], tmp);
				exit(1);
			}
			break;
		case 'n':
			tmp=strtol(optarg, NULL, 0);
			if(tmp>0) {
				retry=tmp;
			} else {
				fprintf(stderr, "%s: invalid retry count -- %d\n", argv[0], tmp);
				exit(1);
			}
			break;
		case 'v':
			debug=1;
			break;
		}
	}

// Determine bus address
	int addr;
	if((argc-optind)>0) {
		long addrl=strtol(argv[optind], NULL, 0);
		if( (addrl>=0) && (addrl<=247) ) {
			addr=(int)addrl;
		} else {
			fprintf(stderr, "%s: invalid bus address -- %d, MODBUS allows from 1 to 247 and 0 for broadcast\n", argv[0], addrl);
			exit(1);
		}
	} else {
		fprintf(stderr, "%s: not enough arguments\n", argv[0]);
		usage(argv[0], 1);
	}

// Check UART settings
	struct stat info;
	if(stat(uart, &info)==0) {
		if(!S_ISCHR(info.st_mode)) {
			fprintf(stderr, "%s: given UART device is not a character device -- '%s'\n", argv[0], uart);
			exit(1);
		}
	} else {
		error(1, errno, "cannot stat UART device '%s'", uart);
	}

// Debug output
	if(debug) {
		printf("Device: %s %d %d/%c/%d\n", uart, uart_baudrate, uart_databits, uart_parity, uart_stopbits);
		printf("Bus address: %d\n", addr);
	}

// Do real work (at last!)
	modbus_t* modbus=modbus_new_rtu(uart, uart_baudrate, uart_parity, uart_databits, uart_stopbits);
	if (modbus == NULL) {
		fprintf(stderr, "%s: unable to create the libmodbus context\n", argv[0]);
		exit(2);
	}
	modbus_rtu_set_serial_mode(modbus, MODBUS_RTU_RS485);  // Make configurable!
	if (modbus_connect(modbus) == -1) {
		fprintf(stderr, "%s: MODBUS connection failed: %s\n", argv[0], modbus_strerror(errno));
		modbus_free(modbus);
		exit(2);
	}
	// Do it!
	uint8_t id_rq[] = { 0x00, 0x2B, 0x0E,      0x00,             0x00};
	//                  ^addr ^read device id  ^read id code (?) ^object id
	modbus_close(modbus);
	modbus_free(modbus);

}
