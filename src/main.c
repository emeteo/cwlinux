
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

#include "debug.h"
#include "menu.h"
#include "serial.h"

// Numero de pantalla que se esta mostrando
int DisplayedScreen;
int ClearScreen=1;

void print_screen_1 (void)
{
    static int value =0;
    value = value +1;
    if (ClearScreen )
        cw_clear_dsp();
    cw_put_txt( 2, 2, "SCREEN 1");
    ClearScreen=0;
}



void onExpireTimerScreen ( sigval_t value );
void onExpireTimerVolume ( sigval_t value );

#define MAX_TIMERS 2
#define TIMER_SCREEN 0
#define TIMER_VOLUME 1

    timer_t timer_ids[MAX_TIMERS];
    void (*timer_functions[MAX_TIMERS]) (union sigval) = { onExpireTimerScreen,onExpireTimerVolume};
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
    print_screen_1();
    return;   
}

void onExpireTimerVolume ( sigval_t value )
{
    debug2_print("onExpireTimerScreen with value %d\n", value);
    cancelTimer ( timer_ids[TIMER_VOLUME]);
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


void displayVolume ( int vol)
{
	char txt[255];	

	sprintf(txt,"%d %%",vol);
	cw_put_txt ( 1 , 1, txt);
	cw_draw_hbar( 2, 2, ( vol*120)/100);
	printf("enviando texto : %s.\n", txt);
        cancelTimer(timer_ids[TIMER_SCREEN]);
        launchTimer (timer_ids[TIMER_VOLUME], 5000,1000);
        
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
	/* Create the timers*/
        createTimers();
        
	/* Timer to display de screens */
        launchTimer (timer_ids[TIMER_SCREEN], 1000,1000);
        
	currentVolume=IncDecAlsaMasterVolume(0);
	
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
								currentVolume = IncDecAlsaMasterVolume(+100);

								break;
							case 'B': /* down */
								currentVolume=IncDecAlsaMasterVolume(-100);
								break;

							case 'C': /* Left */

								currentVolume=IncDecAlsaMasterVolume(-400);

								  break;
							case 'D': /* Right */ 

								currentVolume=IncDecAlsaMasterVolume(+400);
								 break;
							case 'E': /* Selected */

								break;
							case 'F': 

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
	cw_clear_dsp ();
	close_port( serial_fd );

	return 0;
} 
