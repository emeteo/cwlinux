#include <fcntl.h>
#include <stdio.h>   
#include <stdlib.h>
#include <string.h>
#include <termios.h>  
#include "serial.h"


char cmd[255];
static int serial_fd;

struct termios oldtio;

int open_port(void)
{
	struct termios newtio;

	
      	
	//fd = open(SERIALDEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
	serial_fd = open(SERIALDEVICE, O_RDWR | O_NOCTTY );
	if (serial_fd == -1) {
		perror(SERIALDEVICE);
		return (serial_fd);
	} 	

	/* Save current port settings */
	tcgetattr( serial_fd,&oldtio );

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
	newtio.c_lflag = 0;
	
	newtio.c_cc[VTIME] = 1;
	newtio.c_cc[VMIN] = 0;

	/* Flush data received but not readed */
	tcflush ( serial_fd, TCIFLUSH );
	tcsetattr ( serial_fd, TCSANOW, &newtio );
      return (serial_fd);
}

void close_port ( int fd )
{
#if 0 
	tcsetattr( fd, TCSANOW, &oldtio );
#endif
	return;
}

char* cw_cmd( int command, int * params ) 
{
	int n_count,n_read, i, n_answer;
	
	i= 0;
	n_count = 0;
	n_answer = 0;
	n_read = 0;

	bzero ( cmd, sizeof(cmd)  );

	cmd[i] = 254; i++;
	cmd[i] = command; i++;

	switch( command ) {
	case CW_CMD_FW_VERSION:
	case CW_CMD_MODEL: 
		n_answer = 2;
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
		n_answer= 0;
		break;
	}
		
	cmd[i] = 253; i++;

	debug2_print ( "command:[0]=%d:[1]=%d:[2]=%d:[3]=%d, count=%d\n", 
			cmd[0], cmd[1],cmd[2],cmd[3],i  );
	write ( serial_fd, cmd, i);
	/* Flush data received but not readed */
	tcflush ( serial_fd, TCIFLUSH );

	bzero ( cmd, sizeof(cmd) );

	if ( n_answer ) {
		while ( n_answer - n_count > 0 )
		{
			n_read= read ( serial_fd, &cmd[n_count], sizeof(cmd)-1 );
			debug2_print ( "Answer received: %d\n", n_read);
			n_count += n_read;
			if ( n_read <= 0 ) {
				debug2_print ( "No answer received: %d\n", n_count );
				return NULL;
			}
		}
		debug_print ( "Answer received(%d): ", n_count );
		for ( i=0; i < n_answer; i++)
		{
			debug_print ( "·%d", cmd[i]);
		}
		debug_print ( "%s", "\n");
		return cmd;
	}
	debug2_print ( "<<<< EXIT <<<< %d\n", 0);
	return NULL;
}


int cw_clear_dsp ( void )
{
	debug2_print ( ">>>>>> ENTER >>>> %d\n", 0 );
	cw_cmd ( CW_CMD_CLEAR_DSP, NULL );
	usleep(100000); 
	debug2_print ( "<<<< EXIT <<<< %d\n", 0);
	return 0;
}

int cw_auto_key_hold( int on )
{
	debug2_print ( ">>>>>> ENTER >>>> %d\n", 0 );
	cw_cmd ( on ? CW_CONF_AUTO_KEY_HOLD_ON: CW_CONF_AUTO_KEY_HOLD_OFF, NULL );
	debug2_print ( "<<<< EXIT <<<< %d\n", 0);
	return 0;
}

int cw_put_txt ( int col, int row, char* txt )
{
	int params[2];
	
	size_t len;

	debug2_print ( ">>>>>> ENTER >>>> %d\n", 0 );
	if (  ( col < 0 ) || (col > 19 ) || 
	      ( row < 0 ) || (row > 3) )
		return -1;

	len = strlen ( txt );

	params[0] = col;
	params[1] = row;

	cw_cmd (CW_CMD_TXT_INS_POINT, params );
	write ( serial_fd, txt, len );
	debug2_print ( "<<<< EXIT <<<< %d\n", 0);
	return 0;
}

int cw_conf_vbar ( int wide )
{
	
	debug2_print ( ">>>>>> ENTER >>>> %d\n", 0 );
	if (wide )
		cw_cmd( CW_CONF_WIDE_VBAR, NULL );
	else 
		cw_cmd( CW_CONF_NARROW_VBAR, NULL );
	debug2_print ( "<<<<< EXIT <<<<<< %d\n", 0 );
	return 0;
}

int cw_draw_vbar( int col, int height )
{
	int params[2];

	debug2_print ( ">>>>>> ENTER >>>> %d\n", 0 );
	if (( col < 0 ) || (col > 19 )) return -1;
	
	if ( height < 0 ) height =0;
	if ( height > 32 ) height =32;
	
	params [0] = col;
	params [1] = height;

	cw_cmd ( CW_CMD_DRAW_VBAR, params );

	debug2_print ( "<<<<< EXIT <<<<<< %d\n", 0 );
	return 0;
}

int cw_erase_vbar( int col, int height )
{
	int params[2];

	debug2_print ( ">>>>>> ENTER >>>> %d\n", 0 );
	if (( col < 0 ) || (col > 19 )) return -1;

	if ( height < 0 ) height =0;
	if ( height > 32 ) height =32;
	
	params [0] = col;
	params [1] = height;

	cw_cmd ( CW_CMD_ERASE_VBAR, params );
	usleep(10000); 

	debug2_print ( "<<<<< EXIT <<<<<< %d\n", 0 );
	return 0;
}

int cw_draw_hbar( int col, int row, int len )
{
	int params[3];

	debug2_print ( ">>>>>> ENTER >>>> %d\n", 0 );

	if (( col < 0 ) || (col > 19 )) return -1;

	if ( len < 0 ) len =0;
	if ( len > 122 ) len =122;
	/* Before draw hbar, first of all, earse
	TODO: only earse if new_len < len  */
	cw_erase_hbar( col, row , 122 );
	
	params [0] = col;
	params [1] = row;
	params [2] = len;

	cw_cmd ( CW_CMD_DRAW_HBAR, params );

	debug2_print ( "<<<<< EXIT <<<<<< %d\n", 0 );

	return 0;
}

int cw_erase_hbar( int col, int row, int len )
{
	int params[3];

	debug2_print ( ">>>>>> ENTER >>>> %d\n", 0 );
	if ( len < 0 ) len =0;
	if ( len > 122 ) len =122;
	
	params [0] = col;
	params [1] = row;
	params [2] = len;

	cw_cmd ( CW_CMD_ERASE_HBAR, params );
	usleep(10000); 

	debug2_print ( "<<<<< EXIT <<<<<< %d\n", 0 );
	return 0;
}
int cw_put_pixel ( int col, int row )
{
	int params[2];

	debug2_print ( ">>>>>> ENTER >>>> %d\n", 0 );
	if  ( col < 0 )    col = 0;
	if  ( col > 121 )  col = 121;
	if  ( row < 0 )    row = 0;
	if  ( row > 31 )  row = 31;

	
	params[0] = col;
	params[1] = row;
 
	cw_cmd (CW_CMD_PUT_PIXEL, params );
	debug2_print ( "<<<<< EXIT <<<<<< %d\n", 0 );
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

   
