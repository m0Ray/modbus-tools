#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <error.h>
#include <regex.h>

#include <modbus/modbus.h>
#include <modbus/modbus-rtu.h>

#include "modbustools.h"

void usage(char* self, int e) {
	fprintf(stderr, "Usage: %s [-d /dev/ttyX] [-r baudrate] [-b databits] [-p (N|E|O)] [-s stopbits] [-t (i|r|b|c)] [-f (h|d|b)] [-n retries] [-v] <bus_address> <start-register> [end-register]\n", self);
	fprintf(stderr, "Will write read tab-delimited data to standard output.\n\
  Options description:\n\
  -d: UART device or IP:port (default %s)\n\
  -r: UART baud rate: 9600, 19200, 57600, 115200 etc (default %d)\n\
  -b: UART data bits: 5-8 (default %d)\n\
  -p: UART parity check: None, Even, Odd (default %c)\n\
  -b: UART stop bits: 1-2 (default %d)\n\
  -t: register type: Input (default), Register, Bit, Coil, Float32 (register pairs)\n\
  -f: output format: Hex (default for inputs and registers), Decimal or Binary (default for bits and coils). No effect on Float32 register type.\n\
  -n: retry count (default %d)\n\
  -v: verbose debug on stderr\n", MODBUSTOOLS_DEFAULT_UART, MODBUSTOOLS_DEFAULT_BAUDRATE, MODBUSTOOLS_DEFAULT_DATABITS, MODBUSTOOLS_DEFAULT_PARITY, MODBUSTOOLS_DEFAULT_STOPBITS, MODBUSTOOLS_DEFAULT_RETRIES);
	exit(e);
}

void cleanup(modbus_t* modbus) {
	modbus_close(modbus);
	modbus_free(modbus);
}

int main( int argc, char** argv ) {

// Configuration variables and defaults
	char* uart = MODBUSTOOLS_DEFAULT_UART;
	int  uart_tcp=0;
	int  uart_tcp_port=MODBUS_TCP_DEFAULT_PORT;
	int  uart_baudrate = MODBUSTOOLS_DEFAULT_BAUDRATE;
	int  uart_databits = MODBUSTOOLS_DEFAULT_DATABITS;
	char uart_parity = MODBUSTOOLS_DEFAULT_PARITY;
	int  uart_stopbits = MODBUSTOOLS_DEFAULT_STOPBITS;
	char rtype = MODBUSTOOLS_DEFAULT_READ_RTYPE;
	int  retry = MODBUSTOOLS_DEFAULT_RETRIES;
	int  outformat = MODBUSTOOLS_DEFAULT_OUTFORMAT;
	int  debug=0;

	long tmp;

// Parse commandline options
	int c;
	while ((c = getopt (argc, argv, "hd:r:b:p:s:t:n:f:v")) != -1) {
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
		case 't':
			switch(*optarg) {
			case 'i':
			case 'r':
			case 'b':
			case 'c':
			case 'f':
				rtype=*optarg;
				break;
			default:
				fprintf(stderr, "%s: invalid register type -- '%s'\n", argv[0], optarg);
				break;
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
		case 'f':
			switch(*optarg) {
			case 'H':
			case 'D':
				outformat = *optarg;
				break;
			default:
				fprintf(stderr, "%s: invalid output format -- '%s'\n", argv[0], optarg);
				break;
			}
			break;
		case 'v':
			debug=1;
			break;
		}
	}

// Determine bus address and registers
	int addr;
	int reg1=0;
	int reg2=0;
	int nreg=1;
	int argc2=argc-optind;
	if(argc2>1) {
		tmp=strtol(argv[optind], NULL, 0);
		if( (tmp>=0) && (tmp<=247) ) {
			addr=(int)tmp;
		} else {
			fprintf(stderr, "%s: invalid bus address -- %d, MODBUS allows from 1 to 247 and 0 for broadcast\n", argv[0], tmp);
			exit(1);
		}
		tmp=strtol(argv[optind+1], NULL, 0);
		if( tmp == (tmp & 0xFFFF) ) {
			reg1=(int)tmp;
		} else {
			fprintf(stderr, "%s: invalid start register number -- %d, MODBUS allows from 0 to 65535\n", argv[0], tmp);
			exit(1);
		}
		if(argc2>2) {
			tmp=strtol(argv[optind+2], NULL, 0);
			if( tmp == (tmp & 0xFFFF) ) {
				reg2=(int)tmp;
			} else {
				fprintf(stderr, "%s: invalid end register number -- %d, MODBUS allows from 0 to 65535\n", argv[0], tmp);
				exit(1);
			}
			if(reg1>=reg2) {
				fprintf(stderr, "%s: WARNING: end register is less or equal to start register. Only one start register will be read.\n", argv[0]);
			} else {
				nreg=reg2-reg1+1;
				if(nreg>MODBUS_MAX_READ_REGISTERS) {
					fprintf(stderr, "%s: too many registers to be read -- %d. Only %d allowed.\n", argv[0], nreg, MODBUS_MAX_READ_REGISTERS);
					exit(1);
				}
			}
		}
		if( (rtype == 'f') && (nreg & 0x01)!=0) {
			fprintf(stderr, "%s: for Float32 type number of registers must be even, but %d given.\n", argv[0], nreg);
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
		regex_t re;
		regmatch_t matchptr[3];
		regcomp(&re, "^([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})(\\:[0-9]{1,5})?$", REG_EXTENDED);
		if(regexec(&re, uart, 3, matchptr, 0)==0) {
			uart_tcp=1;
			if(matchptr[2].rm_so>0) {
				uart_tcp_port = strtoul(uart+matchptr[2].rm_so+1, NULL, 10);
				uart[matchptr[2].rm_so] = 0x00;
			}
		} else {
			error(1, errno, "cannot stat UART device '%s'", uart);
		}
		regfree(&re);
	}

// Debug output
	if(debug) {
		if(uart_tcp) {
			fprintf(stderr, "Device [TCP]: %s:%d\n", uart, uart_tcp_port);
		} else {
			fprintf(stderr, "Device: %s %d %d/%c/%d\n", uart, uart_baudrate, uart_databits, uart_parity, uart_stopbits);
		}
		fprintf(stderr, "Type: %c\n", rtype);
		fprintf(stderr, "Address: %d\n", addr);
		fprintf(stderr, "Start reg: %d\n", reg1);
		fprintf(stderr, "End reg: %d\n", reg2);
		fprintf(stderr, "Register count: %d\n", nreg);
	}

// Connect
	modbus_t* modbus;
	if(uart_tcp) {
		modbus=modbus_new_tcp(uart, uart_tcp_port);
		if (modbus == NULL) {
			fprintf(stderr, "%s: unable to create the libmodbus context\n", argv[0]);
			exit(2);
		}
	} else {
		modbus=modbus_new_rtu(uart, uart_baudrate, uart_parity, uart_databits, uart_stopbits);
		if (modbus == NULL) {
			fprintf(stderr, "%s: unable to create the libmodbus context\n", argv[0]);
			exit(2);
		}
		modbus_rtu_set_serial_mode(modbus, MODBUS_RTU_RS485); // Make configurable!
	}
	if (modbus_connect(modbus) == -1) {
		fprintf(stderr, "%s: MODBUS connection error: %s\n", argv[0], modbus_strerror(errno));
		modbus_free(modbus);
		exit(2);
	}

	// Actually read
	modbus_set_slave(modbus, addr);
	uint16_t* ibuf=NULL;
	Float32* fbuf=NULL;
	uint8_t* cbuf=NULL;
	int result;

	switch(rtype) {
	case 'i':
		ibuf=malloc(nreg*sizeof(uint16_t));
		for(; retry>0; retry--) {
			if(modbus_read_input_registers(modbus, reg1, nreg, ibuf)==-1) {
				result=2;
			} else {
				result=0;
				break;
			}
		}
		break;
	case 'r':
		ibuf=malloc(nreg*sizeof(uint16_t));
		for(; retry>0; retry--) {
			if(modbus_read_registers(modbus, reg1, nreg, ibuf)==-1) {
				result=2;
			} else {
				result=0;
				break;
			}
		}
		break;
	case 'f':
		fbuf=malloc(nreg*sizeof(uint16_t));
		for(; retry>0; retry--) {
			if(modbus_read_input_registers(modbus, reg1, nreg, (uint16_t*)fbuf)==-1) {
				result=2;
			} else {
				result=0;
				break;
			}
		}
		break;
	case 'b':
		break;
//~ ilyxa 11.3.18 single coil read
	case 'c':
		cbuf=malloc(nreg*sizeof(uint8_t));
		for(; retry>0; retry--) {
			if(modbus_read_bits(modbus, reg1, nreg, (uint8_t*)cbuf)==-1) {
				result=2;
			} else {
				result=0;
				break;
			}
			break;
		}
	}

// Debug output
	if(debug) {
		fprintf(stderr, "Retries left: %d\n", retry);
	}

	switch(result) {
	case 0:
		switch(rtype) {
		case 'i':
		case 'r':
			for(int i=0; i<nreg; i++) {
				switch(outformat) {
				case 'H':
					printf("%x\t", ibuf[i]);
					break;
				case 'D':
					printf("%d\t", ibuf[i]);
					break;
				}
			}
			printf("\n");
			break;
		case 'f':
			for(int i=0; i<(nreg/2); i++) {
				printf("%f\t", fbuf[i].fl );
			}
			printf("\n");
			break;
		//~ ilyxa 11.3.18 coil read
		case 'c':
			for(int i=0; i<nreg; i++) {
				switch(outformat) {
				case 'H':
					printf("%x\t", cbuf[0]);
					break;
				case 'D':
					printf("%d\t", cbuf[0]);
					break;
				}
				printf("\n");
			}
		}
		break;
	case 2:
		fprintf(stderr, "%s: MODBUS read error: %s\n", argv[0], modbus_strerror(errno));
		break;
	}

// Clean up
	if(ibuf!=NULL) free(ibuf);
	if(cbuf!=NULL) free(cbuf);
	cleanup(modbus);

// Exit with result
	exit(result);

}
