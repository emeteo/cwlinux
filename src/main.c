
#include <stdbool.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <alsa/asoundlib.h>
#include <alsa/mixer.h>


#include "debug.h"
#include "menu.h"
#include "serial.h"


void menu_callback ( void )
{
	cw_clear_dsp ();

	cw_put_txt ( 1,1, "Menu callback" );

	return;
}

/* Return the volume set */

int IncDecAlsaMasterVolume(int step)
{
    long min, max, volume;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "PCM";

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

	return;
}

int main ( int argc, char **argv )
{
	int res;
	char buf[255];
	int i, j;
	char firmware[5], model[10];
	char *result;
	int n_read;
	int do_exit;

	int serial_fd;

	fd_set rfds;
	struct timeval tv;
	int retval;
	
	cw_menu *menu1,*menu2,*menu3, *menu, *next_menu;
	cw_menuitem *menuitem, *submenuitem;

	int currentVolume;
	
	
	do_exit = 0;
	if ( (serial_fd = open_port ()) < 0) {
		return -1;
	}

	cw_auto_key_hold ( true );	
	cw_text_invert   ( false);	

	if ( ( argc > 1 ) && ( ! strcmp ( argv[1], "--stop" )  ) )
			cw_clear_dsp ();
	if ( ( argc > 1 ) && ( ! strcmp ( argv[1], "--start" )  ) )
			cw_clear_dsp ();

#if 0
	/* Build a submenu */
	menu2= cw_new_menu( "Mi segundo menu");
	cw_menu_add_menuitem ( menu2, cw_new_menuitem ( "Submenu 2 - item 1", NULL ) );
	cw_menu_add_menuitem ( menu2, cw_new_menuitem ( "submenu 2 - item 2", NULL ));	
	cw_menu_add_menuitem ( menu2, cw_new_menuitem ( "submenu 2 - item 3", NULL ));	
	cw_menu_add_menuitem ( menu2, cw_new_menuitem ( "submenu 2 - item 4", NULL ));	


	menu1 = cw_new_menu ( "Mi primer menu" );
	menuitem = cw_new_menuitem ( "Menu 1-menuitem 1", NULL );	
	cw_add_menuitem ( menuitem, cw_new_menuitem ( "Menu 1 - menuitem 2", NULL ));	
	submenuitem = cw_new_menuitem ( "Menu 1 - menuitem 3", NULL );
		/* Menu2 is a submenu of menuitem3 */
	cw_add_submenuitem ( submenuitem, menu2 );

	cw_add_menuitem ( menuitem, submenuitem );	
	cw_add_menuitem ( menuitem, cw_new_menuitem ( "Call - menuitem 4", menu_callback));	
	cw_add_menuitem ( menuitem, cw_new_menuitem ( "Menu 1 - menuitem 5", NULL ));	
	submenuitem = cw_new_menuitem ( "Menu 1 - menuitem 5", NULL );
	cw_add_menuitem ( menuitem, cw_new_menuitem ( "Menu 1 - menuitem 6", NULL ));	
	cw_menu_add_menuitem ( menu1, menuitem );


	menu3= cw_new_menu( "Mi tercer menu");
	menuitem = cw_new_menuitem ( "Menu 3-menuitem 1", NULL );	
	cw_add_menuitem ( menuitem, cw_new_menuitem ( "Menu 3 - menuitem 2", NULL ));	
	cw_add_menuitem ( menuitem, cw_new_menuitem ( "Menu 3 - menuitem 3", NULL ));	
	cw_add_menuitem ( menuitem, cw_new_menuitem ( "Menu 3 - menuitem 4", NULL ));	
	cw_menu_add_menuitem ( menu3, menuitem );

//	cw_add_menu( menu1, menu2 );
	cw_add_menu( menu1, menu3 );

	debug2_print( "%s\n", menu1->name );

	menu = menu1;

#endif
	cw_clear_dsp ();
//	cw_display_menu ( menu );

	/* Config vbar */
	cw_conf_vbar(1);
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

								//cw_select_prev_menuitem( menu );
								break;
							case 'B': /* down */
								currentVolume=IncDecAlsaMasterVolume(-100);

								//cw_select_next_menuitem ( menu );
								break;

							case 'C': /* Left */

								currentVolume=IncDecAlsaMasterVolume(-400);

								//next_menu = cw_prev_menu ( menu );
								//if ( next_menu != NULL ) menu = next_menu;
								//cw_clear_dsp ();
								//cw_display_menu ( menu );
								  break;
							case 'D': /* Right */ 

								currentVolume=IncDecAlsaMasterVolume(+400);

								//next_menu = cw_next_menu ( menu );
								//if ( next_menu != NULL ) menu = next_menu;
								//cw_clear_dsp ();
								//cw_display_menu ( menu );

								 break;
							case 'E': /* Selected */
#if 0
								if ( menu && menu->selected != NULL )
								if  ( menu->selected->type == CW_MENU_SUBMENU )
								{	
									debug2_print ( "%s\n", "Submenu selected" );
									next_menu =  menu->selected->submenu;
									if ( next_menu != NULL) menu = next_menu;
									cw_clear_dsp ();
									cw_display_menu ( menu );
		
								}else
									if ( menu->selected->cw_function_menu != NULL ) 
										menu->selected->cw_function_menu ();
#endif
								break;
							case 'F': 
#if 0
								if ( menu->parent == NULL || menu->parent->parent == NULL) do_exit = 0;
								else {
									next_menu = menu->parent->parent;
									if ( next_menu != NULL ) menu = next_menu;
									cw_clear_dsp ();
									cw_display_menu ( menu );
								}
#endif
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

	
#if 0

	result=cw_cmd ( CW_CMD_FW_VERSION, NULL );
	if ( result ) {
		sprintf( firmware, "%d.%d", result[0],result[1]);
	}
	result=cw_cmd ( CW_CMD_MODEL, NULL );
	if ( result ) {
		sprintf( model, "%d%d", result[0],result[1]);
	}


	cw_cmd ( CW_CMD_CLEAR_DSP, NULL );
	/* We ned to clear DSP was finished */
	usleep(100000); 

	sprintf( buf, "Firmware: %s", firmware );
	cw_put_txt ( 0, 0, buf );

	sprintf( buf, "Model: %s", model);
	cw_put_txt ( 0, 1, buf );

	cw_conf_vbar ( 0 ); 
	cw_draw_hbar ( 0, 2, 20 );

#endif
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
/* fill up all possitions with numbers */
#if 0
	cw_cmd ( CW_CMD_TXT_HOME, NULL );
	write ( serial_fd, "home", 4 );
	
	for ( i=0; i< 20; i++ )
		for ( j=0; j<4; j++)
		{
			sprintf ( buf, "%d", j );
			cw_put_txt ( i, j, buf );
		}

	debug2_print ( "<<<<< EXIT <<<<<< %d\n", 0 );
#endif
	cw_destroy_menu_list ( menu1 );
	cw_clear_dsp ();
	close_port( serial_fd );

	return 0;
} 
