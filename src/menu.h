/* Mario Teijeiro */
#include <stdbool.h>

typedef enum { CW_MENU_ACTION, CW_MENU_SUBMENU} cw_menuitem_type;


/* It is an entry of a menu. it can be of serveral types:
	* ACTION : Normal entry menu. Represents an action
	* SUBMENU 
*/

typedef struct cw_menuitem cw_menuitem;
typedef struct cw_menu cw_menu;

struct cw_menuitem{
	char *name;
	bool visible;
	cw_menuitem_type type;
	void (*cw_function_menu)(void);

	cw_menu* parent;
	cw_menu* submenu;
	cw_menuitem *next;
	cw_menuitem *prev;

};


struct cw_menu{
	char *name;
	cw_menuitem *items;
	cw_menuitem * parent; //NULL if it's not a submenu
	cw_menuitem * selected;
	cw_menu * next;
	cw_menu * prev;
};



cw_menuitem* cw_new_menuitem ( const char* name, void (*callback)(void) );
void cw_add_menuitem ( cw_menuitem* a, cw_menuitem* b );
void cw_destroy_menuitem ( cw_menuitem* item );
void cw_destroy_menuitem_list ( cw_menuitem* item );

cw_menu* cw_new_menu ( char* name );
void cw_add_menu ( cw_menu* a, cw_menu* b );
void cw_destroy_menu ( cw_menu*);
void cw_destroy_menu_list ( cw_menu* menu );



void cw_menu_add_menuitem ( cw_menu* menu, cw_menuitem * item );


void cw_display_menu ( cw_menu* menu );
cw_menu* cw_next_menu( cw_menu * menu );
cw_menu* cw_prev_menu( cw_menu * menu );

void cw_select_prev_menuitem( cw_menu* menu );
void cw_select_next_menuitem ( cw_menu* menu );

