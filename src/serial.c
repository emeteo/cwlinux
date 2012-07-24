#include <fcntl.h>
#include <termios.h>  
#include <stdio.h>   
#include <strings.h>


#define BAUDRATE B19200
#define SERIALDEVICE "/dev/ttyUSB0"
#define FALSE 0
#define TRUE 1

char cmd[255];
int serial_fd;

volatile int STOP=FALSE;

#define CW_CMD_MODEL 48
#define CW_CMD_FW_VERSION 49
#define CW_CMD_CLEAR_DSP 88
#define CW_CMD_BACKLIGHT_ON 66
#define CW_CMD_BACKLIGHT_OFF 70
#define CW_CMD_BRIGHTNESS 64


#define CW_CMD_TXT_HOME 72
#define CW_CMD_TXT_INS_POINT 71

#define DEBUG 1
#define debug_print(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)


struct termios oldtio;

int open_port(void)
{
	int fd; /* File descriptor for the port */
	struct termios newtio;

	
      	
	fd = open(SERIALDEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1) {
		perror(SERIALDEVICE);
		return (fd);
	} 	

	/* Save current port settings */
	tcgetattr( fd,&oldtio );

	bzero( &newtio, sizeof( newtio ) );
	/* Control modes */
	newtio.c_cflag = CS8| CREAD | CLOCAL;
	
	cfsetispeed( & newtio, BAUDRATE );
	cfsetospeed( & newtio, BAUDRATE );
	//newtio.c_cflag &=  ~(ICANON | ECHO | ECHOE | ISIG); 
	/* Input modes */
	newtio.c_iflag = IGNPAR;
	newtio.c_iflag &= ~(IXON | IXOFF | IXANY);
	/* Output modes */
	newtio.c_oflag = 0;
	/* Flag constants */
	newtio.c_lflag = ICANON;
	
	newtio.c_cc[VTIME] = 10;
	newtio.c_cc[VMIN] = 0;

	/* Flush data received but not readed */
	tcflush ( fd, TCIFLUSH );
	tcsetattr ( fd, TCSANOW, &newtio );
      return (fd);
}

void close_port ( int fd )
{
//	tcsetattr( fd, TCSANOW, &oldtio );
	return;
}

char* cw_cmd( int command, int * params ) 
{
	int ncount, i, nanswer;
	
	i = 0;
	cmd[i] = 254; i++;
	cmd[i] = command; i++;

	switch( command ) {
	case CW_CMD_FW_VERSION:
	case CW_CMD_MODEL: 
		nanswer = 2;
		break;

	case CW_CMD_TXT_INS_POINT:
		cmd[i] = params[0]; i++;
		cmd[i] = params[1]; i++;
		break;
	
	default: 
		nanswer= 0;
		break;
	}
		
	cmd[i] = 253; i++;

	debug_print ( "command:[0]=%d:[1]=%d:[2]=%d, count=%d\n", cmd[0], cmd[1],cmd[2],i  );
	write ( serial_fd, cmd, i);

	bzero ( cmd, i );

	if ( nanswer ) {
		ncount = read ( serial_fd, cmd, 2 );
		if ( ncount > 0 ) {
			printf ( "Count: %d :%d - %d \n", ncount,cmd[0], cmd[1] );
			return cmd;
		} else {
			debug_print ( "No answer received: %d\n", ncount );
		}
	}
	return NULL;
}



int main ( int argc, char **argv )
{
	int res;
	char buf[255];
	int params[5];
	
	if ( (serial_fd = open_port ()) < 0) {
		return -1;
	}
	
	cmd[0] = 254;
	cmd[1] = 88;
	cmd[2] = 253;

	
	// parameters
	if ( (argc > 1 ) && strcmp ( argv[1], "--version" ) == 0 ) {
		debug_print ( "Arguments:%s\n", argv[1] );
		cw_cmd ( CW_CMD_FW_VERSION, NULL );
	}	

	cw_cmd ( CW_CMD_CLEAR_DSP, NULL );
	sleep ( 1 );

	cw_cmd ( CW_CMD_TXT_HOME, NULL );
	write ( serial_fd, "home", 4 );

	
	params[0] = 0;
	params[1] = 1;
	cw_cmd ( CW_CMD_TXT_INS_POINT, params );
	write ( serial_fd, "home2", 5 );




	
#if 0
	write ( serial_fd, buf, 3);
	
	while ( STOP == FALSE ) {
		res = read ( serial_fd, buf, 255 );
		buf[res]=0;
		printf( ":%s:%d\n", buf, res );
		if ( buf[0]=='z' ) STOP = TRUE;
		sleep (1);
	}

#endif
	
	close_port( serial_fd );

}    
