
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "menu.h"
#include "serial.h"




cw_menuitem* cw_new_menuitem ( char* name, void (*callback)(void) )
{
	cw_menuitem *menuitem;
	menuitem = ( cw_menuitem* ) malloc ( sizeof( cw_menuitem ));	
	if ( menuitem == NULL ) {
		perror( "cw_new_menu" );
		return NULL;
	}	
	menuitem->name = ( char* ) malloc ( strlen ( name ));
	if ( menuitem->name == NULL ) {
		perror( "cw_new_menu" );
		free( menuitem );
		return NULL;
	}
	strcpy ( menuitem->name, name );

	menuitem->cw_function_menu = callback;
	menuitem->type = ( callback == NULL ? CW_MENU_SUBMENU: CW_MENU_ACTION );

	menuitem->visible = true;
	menuitem->next = NULL;
	menuitem->prev= NULL;
	menuitem->parent= NULL;
	menuitem->submenu= NULL;
	
	
	debug2_print( "name: %s\n", menuitem->name );
	return menuitem;

}


void cw_add_menuitem ( cw_menuitem* a, cw_menuitem* b )
{
	cw_menuitem *p;
	
	if ( a == NULL || b == NULL ) return ;

	/* Find the tail of the list */
	p = a;
	while ( p-> next != NULL) p = p->next;
	
	b->prev = p;
	b->next = NULL;
	p->next = b;
	return ;
}

void cw_add_submenuitem ( cw_menuitem* a, cw_menu* submenu )
{
	
	if ( a == NULL || submenu == NULL ) return ;

	if ( a->type == CW_MENU_SUBMENU ) {
		 a->submenu= submenu;
		 submenu->parent = a;
	}
	
	return ;
}

void cw_destroy_menuitem ( cw_menuitem* item )
{
	if ( item != NULL ) {
		if ( item->name != NULL ) {
			debug2_print( "Destroying menuitem: %s\n", item->name );
			free( item->name );
		}
		if ( item->submenu != NULL ) {
			debug2_print( "Destroying submenuitem:%d\n",0 );
			cw_destroy_menu_list ( item->submenu );
		}
		free( item );
	}
}

void cw_destroy_menuitem_list ( cw_menuitem* item )
{
	cw_menuitem *p,*n;
	if ( item != NULL ) {
		/* Find the tail of the list */
		p= item;
		while ( p!=NULL){
		 	n=p->next;
			cw_destroy_menuitem( p );
			p=n;
		}
	}
	return ;
}

cw_menu* cw_new_menu ( char* name )
{
	cw_menu *menu;
	menu = ( cw_menu* ) malloc ( sizeof( cw_menu ));	
	if ( menu == NULL ) {
		perror( "cw_new_menu" );
		return NULL;
	}	
	menu->name = ( char* ) malloc ( strlen ( name ));
	if ( menu->name == NULL ) {
		perror( "cw_new_menu" );
		free( menu );
		return NULL;
	}
	strcpy ( menu->name, name );
	menu->next = NULL;
	menu->prev= NULL;
	menu->items= NULL;
	
	debug2_print( "name: %s\n", menu->name );
	return menu;

}

void cw_add_menu ( cw_menu* a, cw_menu* b )
{
	cw_menu *p;
	
	if ( a == NULL || b == NULL ) return ;

	/* Find the tail of the list */
	p = a;
	while ( p-> next != NULL) p = p->next;
	
	b->prev = p;
	b->next = NULL;
	p->next = b;
}

void cw_destroy_menu ( cw_menu* menu)
{
	cw_menuitem* parent;
	if ( menu != NULL ) {
		if ( menu->name != NULL ) {
			debug2_print( "Destroying menu: %s\n", menu->name );
			free( menu->name );
		}
		parent = menu->parent ;
		if ( parent != NULL ) parent->submenu = NULL;
		if ( menu->items != NULL )
			cw_destroy_menuitem_list ( menu->items );
		free( menu);
	}
	return ;
}

void cw_destroy_menu_list ( cw_menu* menu )
{
	cw_menu *p,*n;
	if ( menu != NULL ) {
		/* Find the tail of the list */
		p= menu;
		while ( p!=NULL){
		 	n=p->next;
			cw_destroy_menu ( p );
			p=n;
		}
	}
	return ;
}


void cw_menu_add_menuitem ( cw_menu* menu, cw_menuitem * item )
{
	if ( menu == NULL || item == NULL ) return;

	if ( menu->items == NULL ) {
		menu->items = item;
	} else {
		cw_add_menuitem ( menu->items, item );
	}

	item->parent = menu;

	return;
}



void cw_display_menu ( cw_menu* menu )
{
	int row =0;
	cw_menuitem * item ;


	item = menu->items;

	if ( menu->selected != NULL )
		debug2_print( "Selected menuitem: %s\n", menu->selected->name );
	cw_put_txt ( 0,0, " " );	
	cw_put_txt ( 0,1, " " );	
	cw_put_txt ( 0,2, " " );	
	cw_put_txt ( 0,3, " " );	

	while ( item != NULL )
	{
		if ( row < 4 ) {
			if ( menu->selected == item ) {
				cw_text_invert   ( true );	
				cw_put_txt ( 0,row, ">" );	
				cw_text_invert   ( false );	
			}
			debug2_print( "Displaing menuitem: %s\n", item->name );
			cw_put_txt ( 1, row, item->name ); 
			item->visible= true;
		} else item->visible = false;
		row ++;
		item = item->next;
	}
	return;
}

void cw_select_next_menuitem ( cw_menu* menu )
{
	cw_menuitem * item ;
	if ( menu == NULL ) return;
	debug2_print( "Select next menu: %s\n", menu->name );

	if ( menu->selected == NULL )
		menu->selected = menu->items;	
	else{
		menu->selected = menu->selected->next;
		if ( menu->selected == NULL ) menu->selected = menu->items;
		debug2_print( "Selected menuitem: %s\n", menu->selected->name );
	}
	cw_display_menu ( menu );
}

void cw_select_prev_menuitem( cw_menu* menu )
{
	cw_menuitem * lastitem ;

	if ( menu == NULL ) return;
	debug2_print( "Select prev menu: %s\n", menu->name );

	lastitem = menu->items;
	while ( lastitem->next != NULL ) lastitem = lastitem->next;

	if ( menu->selected == NULL ) {
		menu->selected = lastitem;	
	}else {
		menu->selected = menu->selected->prev;
		if ( menu->selected == NULL ) menu->selected = lastitem;
		debug2_print( "Selected menuitem: %s\n", menu->selected->name );
	}
	cw_display_menu ( menu );
}



