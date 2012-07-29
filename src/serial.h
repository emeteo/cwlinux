

#define DEBUG 1
#define debug2_print(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)
#define debug_print(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, fmt,  __VA_ARGS__); } while (0)

#define BAUDRATE B19200
#define SERIALDEVICE "/dev/ttyUSB0"


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
#define CW_CONF_AUTO_KEY_HOLD_ON 50
#define CW_CONF_AUTO_KEY_HOLD_OFF 51

#define CW_CMD_PUT_PIXEL 112
#define CW_CMD_CLEAR_PIXEL 113

#define CW_CONF_WIDE_VBAR 	118
#define CW_CONF_NARROW_VBAR	115

#define CW_CMD_DRAW_VBAR	61
#define CW_CMD_ERASE_VBAR	45

#define CW_CMD_DRAW_HBAR	124
#define CW_CMD_ERASE_HBAR	43

int open_port(void);
void close_port ( int fd );


int cw_clear_dsp ( void );

int cw_auto_key_hold( int on );

int cw_put_txt ( int col, int row, char* txt );

int cw_conf_vbar ( int wide );

int cw_draw_vbar( int col, int height );

int cw_erase_vbar( int col, int height );

int cw_draw_hbar( int col, int row, int len );

int cw_erase_hbar( int col, int row, int len );

int cw_put_pixel ( int col, int row );

int cw_clear_pixel ( int col, int row );
