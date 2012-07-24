#include <fcntl.h>
#include <termios.h>  
#include <stdio.h>   
#include <stdlib.h>
#include <string.h>


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


#define CW_CONF_TEXT_INV_ON 102
#define CW_CONF_TEXT_INV_OFF 103

#define CW_CMD_PUT_PIXEL 112
#define CW_CMD_CLEAR_PIXEL 113

#define CW_CONF_WIDE_VBAR 	118
#define CW_CONF_NARROW_VBAR	115

#define CW_CMD_DRAW_VBAR	61
#define CW_CMD_ERASE_VBAR	45

#define CW_CMD_DRAW_HBAR	124
#define CW_CMD_ERASE_HBAR	43

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
	/* Control modes */ newtio.c_cflag = CS8| CREAD | CLOCAL;
	
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
	tcsetattr( fd, TCSANOW, &oldtio );
	return;
}

char* cw_cmd( int command, int * params ) 
{
	int ncount, i, nanswer;
	
	i= 0;
	ncount = 0;
	nanswer = 0;

	bzero ( cmd, sizeof(cmd)  );

	cmd[i] = 254; i++;
	cmd[i] = command; i++;

	switch( command ) {
	case CW_CMD_FW_VERSION:
	case CW_CMD_MODEL: 
		nanswer = 2;
		break;

	case CW_CMD_TXT_INS_POINT:
	case CW_CMD_PUT_PIXEL:
	case CW_CMD_CLEAR_PIXEL: 
	case CW_CMD_DRAW_VBAR:
	case CW_CMD_ERASE_VBAR:
		cmd[i] = params[0]; i++;
		cmd[i] = params[1]; i++;
		break;
	case CW_CMD_DRAW_HBAR:
	case CW_CMD_ERASE_HBAR:
		cmd[i] = params[0]; i++;
		cmd[i] = params[1]; i++;
		cmd[i] = params[2]; i++;
		break;

	default: 
		nanswer= 0;
		break;
	}
		
	cmd[i] = 253; i++;

	debug_print ( "command:[0]=%d:[1]=%d:[2]=%d:[3]=%d, count=%d\n", cmd[0], cmd[1],cmd[2],cmd[3],i  );
	write ( serial_fd, cmd, i);

	bzero ( cmd, sizeof(cmd) );

	if ( nanswer ) {
		ncount = read ( serial_fd, cmd, 2 );
		if ( ncount > 0 ) {
			printf ( "Count: %d :%d - %d \n", ncount,cmd[0], cmd[1] );
			return cmd;
		} else {
			debug_print ( "No answer received: %d\n", ncount );
		}
	}
	debug_print ( "<<<< EXIT <<<< %d\n", 0);
	return NULL;
}


int cw_put_txt ( int col, int row, char* txt )
{
	int params[2];
	
	size_t len;

	if (  ( col < 0 ) || (col > 19 ) || 
	      ( row < 0 ) || (row > 3) )
		return -1;

	len = strlen ( txt );

	params[0] = col;
	params[1] = row;

	cw_cmd (CW_CMD_TXT_INS_POINT, params );
	write ( serial_fd, txt, len );
	return 0;
}

int cw_conf_vbar ( int wide )
{
	
	if (wide )
		cw_cmd( CW_CONF_WIDE_VBAR, NULL );
	else 
		cw_cmd( CW_CONF_NARROW_VBAR, NULL );
	return 0;
}

int cw_draw_vbar( int col, int height )
{
	int params[2];

	if (( col < 0 ) || (col > 19 )) return -1;
	
	if ( height < 0 ) height =0;
	if ( height > 32 ) height =32;
	
	params [0] = col;
	params [1] = height;

	cw_cmd ( CW_CMD_DRAW_VBAR, params );

	return 0;
}

int cw_erase_vbar( int col, int height )
{
	int params[2];

	if (( col < 0 ) || (col > 19 )) return -1;

	if ( height < 0 ) height =0;
	if ( height > 32 ) height =32;
	
	params [0] = col;
	params [1] = height;

	cw_cmd ( CW_CMD_ERASE_VBAR, params );

	return 0;
}

int cw_draw_hbar( int col, int row, int len )
{
	int params[3];

	debug_print ( ">>>>>> ENTER >>>> %d\n", 0 );

	if (( col < 0 ) || (col > 19 )) return -1;

	if ( len < 0 ) len =0;
	if ( len > 122 ) len =122;
	
	params [0] = col;
	params [1] = row;
	params [2] = len;

	cw_cmd ( CW_CMD_DRAW_HBAR, params );

	debug_print ( "<<<<< EXIT <<<<<< %d\n", 0 );

	return 0;
}

int cw_erase_hbar( int col, int row, int len )
{
	int params[3];

	if ( len < 0 ) len =0;
	if ( len > 122 ) len =122;
	
	params [0] = col;
	params [1] = row;
	params [2] = len;

	cw_cmd ( CW_CMD_DRAW_HBAR, params );

	return 0;
}
int cw_put_pixel ( int col, int row )
{
	int params[2];

	if  ( col < 0 )    col = 0;
	if  ( col > 121 )  col = 121;
	if  ( row < 0 )    row = 0;
	if  ( row > 31 )  row = 31;

	
	params[0] = col;
	params[1] = row;
 
	cw_cmd (CW_CMD_PUT_PIXEL, params );
	return 0;
}

int cw_clear_pixel ( int col, int row )
{
	int params[2];

	if (  ( col < 0 ) || (col > 121 ) || 
	      ( row < 0 ) || (row > 31) )
		return -1;
	
	params[0] = col;
	params[1] = row;

	cw_cmd (CW_CMD_CLEAR_PIXEL, params );
	return 0;
}

int main ( int argc, char **argv )
{
	int res;
	char buf[255];
	int i, j;
	
	if ( (serial_fd = open_port ()) < 0) {
		return -1;
	}
	

	// parameters
	if ( (argc > 1 ) && strcmp ( argv[1], "--version" ) == 0 ) {
		debug_print ( "Arguments:%s\n", argv[1] );
		cw_cmd ( CW_CMD_FW_VERSION, NULL );
	}	

	cw_cmd ( CW_CMD_CLEAR_DSP, NULL );
	sleep ( 1 );

	cw_conf_vbar ( 0 ); 
	cw_draw_hbar ( 4, 1, 20 );

#if 0
	for ( i = 0 ; i < 33; i ++)
	{
		if ( i % 2 )
			cw_cmd ( CW_CONF_TEXT_INV_ON, NULL );
		else 
			cw_cmd ( CW_CONF_TEXT_INV_OFF, NULL );
		sprintf ( buf, "%d\%", i );
		cw_conf_vbar ( 1 ); 
		cw_draw_vbar ( 4, i );
		// Narrow
		cw_conf_vbar ( 0 ); 
		cw_draw_vbar ( 6, i );
		cw_draw_vbar ( 7, i );

		
		cw_put_txt   ( 4,0,  buf );
		sleep (1);
	}	

#endif
/* Diagonal de (0,0 a (121,31) */
#if 0 
	for ( i=0; i<= 121; i++ )
	{
		cw_put_pixel ( i , ((i *32) / 122 ) -1 );
	}
#endif
/* Rellenar todas las posiciones de numeros */
#if 0
	cw_cmd ( CW_CMD_TXT_HOME, NULL );
	write ( serial_fd, "home", 4 );
	
	for ( i=0; i< 20; i++ )
		for ( j=0; j<4; j++)
		{
			sprintf ( buf, "%d", j );
			cw_put_txt ( i, j, buf );
		}

	close_port( serial_fd );
#endif

	debug_print ( "<<<<< EXIT <<<<<< %d\n", 0 );
	return 0;
}    
