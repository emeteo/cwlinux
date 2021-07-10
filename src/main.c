
#include <stdbool.h>
#include <stdio.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <assert.h>

/* Strings */

#include <string.h>
/* includes for timers */
#include <signal.h>
#include <time.h>

/* mpdclient */
#include <mpd/client.h>

#include "debug.h"
#include "menu.h"
#include "serial.h"

// Numero de pantalla que se esta mostrando
int DisplayedScreen;
int ClearScreen;
struct cw_mpd_status {
  int updated;
  int volume;
  int song_id;
  char artist[255];
  char title [255];
  enum mpd_state state; 
    
} MpdStatus;

#define MAX_SCREENS  1
void print_screen_1 (void);

void (*print_screens [MAX_SCREENS] ) (void) = {print_screen_1};

static struct mpd_connection *mpd_conn=NULL;

int cw_mpd_connect(void)
{
    printf("cw_mpd_connect...");
    mpd_conn = mpd_connection_new("soundserver", 6600, 1000);
    if ( mpd_conn == NULL){
        printf("Can't connect to mpd\n");
        return -1;
    }
    printf("OK\n");
    return 0 ;
}
void cw_mpd_disconnect(void)
{
        mpd_connection_free( mpd_conn);
        mpd_conn = NULL;
}

int cw_mpd_send_stop(void)
{
        if ( (mpd_conn == NULL) && (cw_mpd_connect() != 0 )) return -1;
    
        mpd_run_stop(mpd_conn);
    
        
        return 0;
}
int cw_mpd_send_play(void)
{
        if ( (mpd_conn == NULL) && (cw_mpd_connect() != 0 )) return -1;
        
        mpd_run_play(mpd_conn);
        
        return 0;
}

int cw_mpd_status(void)
{
    struct mpd_status * status;
    
    
    
    if ( (mpd_conn == NULL) && (cw_mpd_connect() != 0 )) return -1;
    
    debug_print("cw_mpd_status\n");
#if 0    
    mpd_command_list_begin(mpd_conn, true);
    mpd_send_status(mpd_conn);
    mpd_send_current_song(mpd_conn);
    mpd_command_list_end(mpd_conn);
    //status = mpd_recv_status(mpd_conn);
#else
    status=mpd_run_status(mpd_conn);    
#endif    
    MpdStatus.updated = 1;
    MpdStatus.volume = mpd_status_get_volume( status );
    MpdStatus.state = mpd_status_get_state ( status );
    MpdStatus.song_id = mpd_status_get_song_id ( status );
    if (status == NULL ) 
    {
       printf("Can't recive status from mpd\n");
       return -1;
    }
    
    mpd_status_free(status);
    return 0;
}

int cw_mpd_get_song(void)
{
    struct mpd_song *song;
    char *result;
    debug_print("cw_mpd_get_song\n");
    
    if ( (mpd_conn == NULL) && (cw_mpd_connect() != 0 )) return -1;
    
    song = mpd_run_current_song ( mpd_conn );
    
    if ( song == NULL ) return -1;
    
    strncpy ( MpdStatus.artist, mpd_song_get_tag ( song,MPD_TAG_ARTIST ,0), sizeof(MpdStatus.artist));
    strncpy ( MpdStatus.title, mpd_song_get_tag ( song,MPD_TAG_TITLE ,0), sizeof(MpdStatus.title));
    
    mpd_song_free (song);
    
    return 0;
}


/* Return the volume set */
int cw_mpd_IncDecVolume(int step)
{
    struct mpd_status * status;
    int new_volume;
    unsigned volume;
    
    if ( (mpd_conn == NULL) && (cw_mpd_connect() != 0 )) return -1;
    
    status = mpd_run_status(mpd_conn);
    
    if (status == NULL ) 
    {
       printf("Can't recive status from mpd\n");
       return -1;
    }
    
    volume = mpd_status_get_volume( status );
    
    new_volume = volume + step;
    if ( new_volume > 100 ) new_volume = 100;
    if ( new_volume < 0 ) new_volume = 1;

    mpd_run_set_volume( mpd_conn, (unsigned)new_volume );
    
    mpd_status_free(status);
    
    debug2_print( "****** Volumen %d\n", new_volume);
    return new_volume;
}



void print_screen_1 (void)
{   
    timer_t timer;
    size_t len_time, len_date;
    char  str_time[22];
    char  status_line1 [22];
    char  status_line2 [22];
    struct tm* tm_info;
    static int pos_ch=0;
    char progress[5]="\\|/-";
    char ch[2]; 
    
    status_line1[0]='\0';
    status_line2[0]='\0';
    if (ClearScreen )
        cw_clear_dsp();
    
    cw_mpd_status();
    
    /* Manager the first line time and date */
    
    time((time_t *)&timer);
    tm_info = localtime((time_t *)&timer);
    
    strftime(str_time, sizeof(str_time), "%H:%M:%S  %d-%m-%Y", tm_info);
    
    debug2_print("#####(%d): %s\n",sizeof(str_time), str_time);
    cw_put_txt( 20-strlen(str_time), 0, str_time);

    /* Manager the second and third line time and date */
    if ( 1 ||  MpdStatus.updated ) {
            switch ( MpdStatus.state )
            {
                case MPD_STATE_PLAY: 
                    cw_mpd_get_song();
                    snprintf(status_line1, 22,MpdStatus.artist); 
                    snprintf(status_line2, 22,MpdStatus.title); 
                    break;
                case MPD_STATE_STOP: 
                    snprintf(status_line1, 22," STOPPED ||"); 
                    status_line2[0]='\0';
                    break;
                default: snprintf(status_line1, 22," ??? UNKNOWN ???"); 
                
            }
            debug2_print("#####: %s\n",status_line1);
            debug2_print("#####: %s\n",status_line2);

	    pos_ch=pos_ch>2? 0:pos_ch+1;
	    ch[0]=progress[pos_ch];
	    ch[1]='\0';

	    cw_put_txt ( 1,2, &ch[0]); 
            cw_put_txt( (20-strlen(status_line1))/2, 2, status_line1);
            cw_put_txt( (20-strlen(status_line1))/2, 3, status_line2);
    }
    ClearScreen=0;
}





void onExpireTimerScreen ( sigval_t value );
void onExpireTimerTransient ( sigval_t value );

#define MAX_TIMERS 2
#define TIMER_SCREEN 0
#define TIMER_TRANSIENT 1


timer_t timer_ids[MAX_TIMERS];

void (*timer_functions[MAX_TIMERS]) (union sigval) = { onExpireTimerScreen,onExpireTimerTransient};


void createTimers(void)
{
    int i;
    struct sigevent sevp;
    timer_t timerid;
    
    sevp.sigev_notify = SIGEV_THREAD;
    
    sevp.sigev_notify_attributes = NULL;
    
    for (i=0; i < MAX_TIMERS; i++)
    {
        sevp.sigev_notify_function = timer_functions[i];
        sevp.sigev_value.sival_int = i;
        timer_create( CLOCK_REALTIME,  &sevp, & (timer_ids  [i]));
    }
}

void deleteTimers (void )
{
    timer_delete( timer_ids[0] );
}

void launchTimer( int t, int msecs, int interval )
{
        struct itimerspec spec;
        
        spec.it_interval.tv_sec     = interval / 1000;
        spec.it_interval.tv_nsec    = interval % 1000;
        spec.it_value.tv_sec        = msecs / 1000 ;
        spec.it_value.tv_nsec       = msecs % 1000;
        
        if ( timer_settime(timer_ids[t], CLOCK_REALTIME , &spec, NULL) !=0) 
            debug2_print ( "failure timer_settime[%d]: %d - %d\n",t, msecs, interval);
        else
            debug2_print ( "timer_settime [%d] OK %d - %d \n", t, msecs, interval );
        return;
}

void cancelTimer ( int t)
{
    launchTimer (t, 0, 0);
    return;
}

static int in_timer =0;
void onExpireTimerScreen ( sigval_t value )
{
    if (in_timer) return;
    in_timer++;
    
    
    debug2_print("onExpireTimerScreen in %d\n", in_timer);
    print_screens[DisplayedScreen]();
    in_timer--;
    return;   
}

void onExpireTimerTransient ( sigval_t value )
{
    if (in_timer) return;
    in_timer++;
    debug2_print("onExpireTimerTransient with %d\n", in_timer);
    cancelTimer ( TIMER_TRANSIENT );
    launchTimer ( TIMER_SCREEN, 1000, 1000);
    ClearScreen=1;
    in_timer--;
    return;   
}


void displayTransientMsg (char * txt )
{
        cw_clear_dsp();
        cw_put_txt(1,1,txt);
        debug2_print("********************Transient msg: %s********************\n", txt);
        cancelTimer(TIMER_SCREEN);
        cancelTimer(TIMER_TRANSIENT);
        launchTimer (TIMER_TRANSIENT, 1500,1500);
}

void displayVolume ( int vol)
{
	char txt[255];	
        cw_clear_dsp();
	sprintf(txt,"%d %%",vol);
	cw_put_txt ( 0 , 1, txt);
	cw_draw_hbar( 2, 2, ( vol*100)/100);
	debug2_print("enviando texto : %s.\n", txt);
        cancelTimer(TIMER_SCREEN);
        cancelTimer(TIMER_TRANSIENT);
        launchTimer (TIMER_TRANSIENT, 1000,1000);
        
	return;
}

int main ( int argc, char **argv )
{

        char buf[255];
	int serial_fd;
        int do_exit = 0;
	fd_set rfds;
	struct timeval tv;
	int retval;
        int n_read;

	int currentVolume;
	
        
        /* Open the serial port */
	if ( (serial_fd = open_port ()) < 0) {
		return -1;
	}

	cw_auto_key_hold    ( true );	
	cw_text_invert      ( false);	
        cw_text_auto_wrap   ( false );
        cw_text_auto_scroll ( false );

	cw_clear_dsp ();

	/* Config vbar */
	cw_conf_vbar(1);
        /* Start with the screen 0 */
        DisplayedScreen =0;
	int ClearScreen;
        /* Create the timers*/
        createTimers();
        
	/* Timer to display de screens */
        launchTimer (TIMER_SCREEN, 1000,1000);
        
	currentVolume=cw_mpd_IncDecVolume(0);
	
        displayVolume(currentVolume);

	/* Setup Serial port */

	FD_ZERO ( &rfds );
	FD_SET ( serial_fd, &rfds );
	
	/* Wait up to 1000us */
	
	while ( ! do_exit )
	{
		usleep ( 10000 );
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO ( &rfds );
		FD_SET ( serial_fd, &rfds );

		retval = select ( FD_SETSIZE,  &rfds, NULL, NULL, &tv );
		if ( retval == -1 ) {
			perror ( "select()");
			break;
		} else if ( retval ) {
			if ( FD_ISSET ( serial_fd, &rfds ) ) {
				while ( (n_read =read ( serial_fd, buf, 1 ) ) > 0 ) {
					buf[1] = '\0';
					if  ( buf[0] >= 65  && buf[0] <=70 ) {
						/* TODO: pass to a function and handle the key 
						  in function of the context we are */
						switch ( buf[0] )
						{
							case 'A': /* up */ 
								currentVolume = cw_mpd_IncDecVolume(+1);
                                                                displayVolume(currentVolume);

								break;
							case 'B': /* down */
								currentVolume=cw_mpd_IncDecVolume(-1);
                                                                displayVolume(currentVolume);
								break;

							case 'C': /* Left */

								currentVolume=cw_mpd_IncDecVolume(-20);
                                                                displayVolume(currentVolume);

								  break;
							case 'D': /* Right */ 

								currentVolume=cw_mpd_IncDecVolume(+20);
                                                                displayVolume(currentVolume);
								 break;
							case 'E': /* Confirm */
                                                                displayTransientMsg("Playing...");
                                                                cw_mpd_send_play();
								break;
							case 'F': /* Cancel */
                                                                displayTransientMsg("Stopping...");
                                                                cw_mpd_send_stop();

								break;
							default: break;
						}
						
						printf ( "%d - %s \n", buf[0], buf);
					} else
						printf ( "Received bad keys: %d - %s \n", buf[0], buf);

				}
			}
		}
	}
        deleteTimers();
        cw_mpd_disconnect();        
	cw_clear_dsp ();
	close_port( serial_fd );

	return 0;
} 
