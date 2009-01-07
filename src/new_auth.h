 /****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops and Fireblade                                      |             *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 *			 New Name Authorization Header Information                *
 ****************************************************************************/

#define AUTO_AUTH /* Do not remove, used to interact with other snippets! - Samson 12-28-98 */

#define AUTH_FILE SYSTEM_DIR "auth.dat"

typedef struct  auth_list               AUTH_LIST; /* new auth -- Rantic */

/* new auth -- Rantic */
#define NOT_AUTHED(ch)          (!IS_NPC(ch) && get_auth_state( ch ) != AUTH_AUTHED  && IS_SET(ch->pcdata->flags, PCFLAG_UNAUTHED) )

#define NEW_AUTH( ch )          (!IS_NPC(ch) && ch->level == 1 )
	
#define IS_WAITING_FOR_AUTH(ch) (!IS_NPC(ch) && ch->desc                     \
                             && get_auth_state( ch ) == AUTH_ONLINE                 \
	                      && IS_SET(ch->pcdata->flags, PCFLAG_UNAUTHED) ) 

/* new auth -- Rantic */
extern          AUTH_LIST         * first_auth_name;
extern          AUTH_LIST         * last_auth_name;

/* new auth stuff in addit.c */
void load_auth_list               args( ( void ) );
void save_auth_list               args( ( void ) );
int  get_auth_state               args( ( CHAR_DATA *ch ) );
void add_to_auth                  args( ( CHAR_DATA *ch ) );
void remove_from_auth             args( ( char *name ) );
void check_auth_state             args( ( CHAR_DATA *ch ) );
AUTH_LIST *get_auth_name          args( ( char *name ) ) ;
void auth_update			    args( ( void ) );

/* New auth stuff --Rantic */
typedef enum 
{ 
  AUTH_ONLINE=0, AUTH_OFFLINE, AUTH_LINK_DEAD, AUTH_CHANGE_NAME,
  AUTH_DENIED, AUTH_AUTHED 
} auth_types;

struct auth_list
{
   char  * name;           /* Name of character awaiting authorization */
   int   state;            /* Current state of authed */
   char  * authed_by;	   /* Name of immortal who authorized the name */
   char  * change_by;	   /* Name of immortal requesting name change */
   char  * denied_by;      /* Name of immortal who denied the name */
   AUTH_LIST *next;
   AUTH_LIST *prev;
};
