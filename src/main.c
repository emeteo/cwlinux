
#include <stdbool.h>
#include <stdio.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <alsa/asoundlib.h>
#include <alsa/mixer.h>

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

#define MAX_SCREENS  1
void print_screen_1 (void);

void (*print_screens [MAX_SCREENS] ) (void) = {print_screen_1};

static struct mpd_connection *mpd_conn=NULL;

int cw_mpd_connect(void)
{
    mpd_conn = mpd_connection_new("localhost", 6600, 0);
    if ( mpd_conn == NULL){
        printf("Can't connect to mpd\n");
        return -1;
    }
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
    struct mpd_song *song;
    if ( (mpd_conn == NULL) && (cw_mpd_connect() != 0 )) return -1;
    
    mpd_command_list_begin(mpd_conn, true);
    mpd_send_status(mpd_conn);
    mpd_send_current_song(mpd_conn);
    mpd_command_list_end(mpd_conn);
    
    status = mpd_recv_status(mpd_conn);
    if (status == NULL ) 
    {
       printf("Can't recive status from mpd\n");
       return -1;
    }
    
    
    
    
    mpd_status_free(status);
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
    if ( new_volume < 0 ) new_volume = 0;

    mpd_run_set_volume( mpd_conn, (unsigned)new_volume );
    
    mpd_status_free(status);
    return volume;
    
    debug2_print( "****** Volumen %d\n", new_volume);
    return new_volume;
}



void print_screen_1 (void)
{   
    timer_t timer;
    char  buffer[26];
    struct tm* tm_info;
    
    time((time_t *)&timer);
    tm_info = localtime((time_t *)&timer);
    strftime(buffer, 26, "%H:%M:%S", tm_info);
    
    if (ClearScreen )
        cw_clear_dsp();
    
    /* Date */
    cw_put_txt( 0, 0, buffer);
    strftime(buffer, 26, "%d-%m-%Y", tm_info);
    
    cw_put_txt( 0, 1, buffer);
    cw_put_txt( 2, 2, "SCREEN 1");
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

void launchTimer( timer_t id, int msecs, int interval )
{
        struct itimerspec spec;
        
        spec.it_interval.tv_sec     = interval / 1000;
        spec.it_interval.tv_nsec    = interval % 1000;
        spec.it_value.tv_sec        = msecs / 1000 ;
        spec.it_value.tv_nsec       = msecs % 1000;
        
        if ( timer_settime(id, CLOCK_REALTIME , &spec, NULL) !=0) 
            debug2_print ( "failure timer_settime: %d - %d\n", msecs, interval);
        else
            debug2_print ( "timer_settime OK %d - %d \n", msecs, interval );
        return;
}

void cancelTimer ( timer_t id )
{
    launchTimer (id, 0, 0);
    return;
}

void onExpireTimerScreen ( sigval_t value )
{

    debug2_print("onExpireTimerScreen with value %d\n", value);
    print_screens[DisplayedScreen]();
    return;   
}

void onExpireTimerTransient ( sigval_t value )
{
    debug2_print("onExpireTimerScreen with value %d\n", value);
    cancelTimer ( timer_ids[TIMER_TRANSIENT]);
    launchTimer ( timer_ids[TIMER_SCREEN], 1000, 1000);
    ClearScreen=1;
    return;   
}

/* Return the volume set */
int IncDecAlsaMasterVolume(int step)
{
    long min, max, volume;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "Master";

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

    snd_mixer_selem_get_playback_volume (elem, SND_MIXER_SCHN_FRONT_RIGHT, &volume);

    if (volume+step > max ) step=max-volume;
    if (volume+step < min ) step=min-volume;

    snd_mixer_selem_set_playback_volume_all(elem, volume+step) ;

    snd_mixer_close(handle);

    int vol = ((volume+step-min)*100) /(max-min);
    debug2_print( "****** Volumen %d\n", vol);
    return vol;
}

void displayTransientMsg (char * txt )
{
        cw_clear_dsp();
        cw_put_txt(1,1,txt);
        debug2_print("Transient msg: %s\n", txt);
        cancelTimer(timer_ids[TIMER_SCREEN]);
        launchTimer (timer_ids[TIMER_TRANSIENT], 3000,1000);
}

void displayVolume ( int vol)
{
	char txt[255];	
        cw_clear_dsp();
	sprintf(txt,"%d %%",vol);
	cw_put_txt ( 1 , 1, txt);
	cw_draw_hbar( 2, 2, ( vol*120)/100);
	debug2_print("enviando texto : %s.\n", txt);
        cancelTimer(timer_ids[TIMER_SCREEN]);
        launchTimer (timer_ids[TIMER_TRANSIENT], 3000,1000);
        
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

	cw_auto_key_hold ( true );	
	cw_text_invert   ( false);	


	cw_clear_dsp ();

	/* Config vbar */
	cw_conf_vbar(1);
        /* Start with the screen 0 */
        DisplayedScreen =0;
	int ClearScreen;
        /* Create the timers*/
        createTimers();
        
	/* Timer to display de screens */
        launchTimer (timer_ids[TIMER_SCREEN], 1000,1000);
        
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
								currentVolume = cw_mpd_IncDecVolume(+100);

								break;
							case 'B': /* down */
								currentVolume=cw_mpd_IncDecVolume(-100);
								break;

							case 'C': /* Left */

								currentVolume=cw_mpd_IncDecVolume(-400);

								  break;
							case 'D': /* Right */ 

								currentVolume=cw_mpd_IncDecVolume(+400);
								 break;
							case 'E': /* Selected */
                                                                displayTransientMsg("Playing...");
                                                                cw_mpd_send_play();
								break;
							case 'F': /* X */
                                                                displayTransientMsg("Stopping...");
                                                                cw_mpd_send_stop();

								break;
							default: break;
						}
						displayVolume(currentVolume);
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
