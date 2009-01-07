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
 *			 New Name Authorization support code                      *
 ****************************************************************************/

/*
 *  New name authorization system
 *  Author: Rantic (supfly@geocities.com)
 *  of FrozenMUD (empire.digiunix.net 4000)
 *
 *  Permission to use and distribute this code is granted provided
 *  this header is retained and unaltered, and the distribution
 *  package contains all the original files unmodified.
 *  If you modify this code and use/distribute modified versions
 *  you must give credit to the original author(s).
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "mud.h"

/* from comm.c, for do_name */
bool check_parse_name	args( ( char *name, bool newchar ) );

/* from act_wiz.c, for do_authorize */
CHAR_DATA *get_waiting_desc args( ( CHAR_DATA *ch, char *name ) );

AUTH_LIST *first_auth_name;
AUTH_LIST *last_auth_name;

bool exists_player( char *name )
{
	struct stat fst;
	char buf[MAX_STRING_LENGTH];

	sprintf( buf, "%s%c/%s", PLAYER_DIR, tolower(name[0]), capitalize(name));

	if( stat(buf, &fst) != -1 )
		return TRUE;
	else
		return FALSE;
}

void clear_auth_list()
{
	AUTH_LIST *auth, *nauth;
	
	for ( auth = first_auth_name; auth; auth = nauth )
	{
		nauth = auth->next;
		if ( !exists_player( auth->name ) )
		{
			UNLINK( auth, first_auth_name, last_auth_name, next, prev );
			if( auth->authed_by )
				STRFREE( auth->authed_by );
			if( auth->change_by )
				STRFREE( auth->change_by );
			if( auth->denied_by )
				STRFREE( auth->denied_by );
			STRFREE( auth->name );
			DISPOSE( auth );
		}
	}
	save_auth_list();
}

void write_auth_file( FILE *fpout, AUTH_LIST *list )
{
	fprintf( fpout, "Name		%s~\n",	list->name );
	fprintf( fpout, "State		%d\n", list->state );
	if( list->authed_by )
		fprintf( fpout, "AuthedBy       %s~\n", list->authed_by );
	if( list->change_by )
		fprintf( fpout, "Change		%s~\n", list->change_by );
	if( list->denied_by )
		fprintf( fpout, "Denied		%s~\n", list->denied_by );
	fprintf( fpout, "End\n\n" );
}

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )  if ( !strcmp( word, literal ) ){ field  = value; fMatch = TRUE;    break; }     

                                   
		
AUTH_LIST *fread_auth( FILE *fp )
{
	AUTH_LIST *new_auth;
	bool fMatch;
	char *word;
	char buf[MAX_STRING_LENGTH];
	
	CREATE( new_auth, AUTH_LIST, 1 );
	
	new_auth->authed_by = NULL;
	new_auth->change_by = NULL;
	new_auth->denied_by = NULL;

	for ( ;; )
	{
		word = feof( fp ) ? "End" : fread_word ( fp );
		fMatch = FALSE;
		switch( UPPER( word[0] ) )
		{
			case '*':
				fMatch = TRUE;
				fread_to_eol( fp );
				break;

			case 'A':
				KEY( "AuthedBy",	new_auth->authed_by,	fread_string( fp ) );
				break;
			
			case 'C':
				KEY( "Change",	new_auth->change_by,	fread_string( fp ) );
				break;

			case 'D':
				KEY( "Denied",	new_auth->denied_by,	fread_string( fp ) );
				break;

			case 'E':
				if ( !str_cmp( word, "End" ) )
				{
					fMatch = TRUE;
					LINK( new_auth, first_auth_name, last_auth_name, next, prev );
					return rNONE;
				}
				break;
				
			case 'N':
				KEY( "Name",	new_auth->name,	fread_string( fp ) );
				break;
		
			case 'S':
				if ( !str_cmp( word, "State" ) )
				{
					new_auth->state = fread_number( fp );
					if ( new_auth->state == AUTH_ONLINE 
					|| new_auth->state == AUTH_LINK_DEAD )
					/* Crash proofing. Can't be online when */
			                /* booting up. Would suck for do_auth   */
						new_auth->state = AUTH_OFFLINE;
					fMatch = TRUE;
					break;
				}
				break;
		}
		if ( !fMatch )
		{
			sprintf( buf, "Fread_auth: no match: %s", word );
			bug( buf, 0 );
		}
		
	}

}

void save_auth_list()
{
	FILE *fpout;
	AUTH_LIST *list;
	
	if ( ( fpout = fopen( AUTH_FILE, "w" ) ) == NULL )
	{
		bug( "Cannot open auth.dat for writing.", 0 );
		perror( AUTH_FILE );
		return;
	}
	
	for ( list = first_auth_name; list; list = list->next )
	{
		fprintf( fpout, "#AUTH\n" );
		write_auth_file( fpout, list );
	} 
	
	fprintf( fpout, "#END\n" );
#ifdef SOLANCODE
	FCLOSE( fpout );
#else
	fclose( fpout );
	fpout = NULL;
#endif
}

void load_auth_list()
{
	FILE *fp;
	int x;
	
	first_auth_name = last_auth_name = NULL;
		
	if ( ( fp = fopen( AUTH_FILE, "r" ) ) != NULL )
	{
		x = 0;
		for ( ;; )
		{
			char letter;
			char *word;
			
			letter = fread_letter( fp );
			if ( letter == '*' )
			{
				fread_to_eol( fp );
				continue;
			}
			
			if ( letter != '#' )
			{
				bug( "Load_auth_list: # not found.", 0 );
				break;
			}
			
			word = fread_word( fp );
			if ( !str_cmp( word, "AUTH" ) )
			{
				fread_auth( fp );
				continue;
			}
			else
			if ( !str_cmp( word, "END" ) )
				break;
			else
			{
				bug( "load_auth_list: bad section.", 0 );
				continue;
			}
		}
#ifdef SOLANCODE
		FCLOSE( fp );
#else
		fclose( fp );
		fp = NULL;
#endif
	}
	else
	{
		bug( "Cannot open auth.dat", 0 );
		exit( 0 );
	}
	clear_auth_list();
}

int get_auth_state( CHAR_DATA *ch )
{
	AUTH_LIST *namestate;
	int state;
	
	state = AUTH_AUTHED;
	
	for ( namestate = first_auth_name; namestate; namestate = namestate->next )
	{
		if ( !str_cmp( namestate->name, ch->name ) )
		{
			state = namestate->state;		
			break;
		}
	}
	return state;
}

AUTH_LIST *get_auth_name( char *name )
{
	AUTH_LIST *mname;

      if( last_auth_name && last_auth_name->next != NULL )
         bug( "Last_auth_name->next != NULL: %s", last_auth_name->next->name );
		
	for( mname = first_auth_name; mname; mname = mname->next )
	{
	   if ( !str_cmp( mname->name, name ) ) /* If the name is already in the list, break */
		break;
	}
	return mname;
}

void add_to_auth( CHAR_DATA *ch )
{
	AUTH_LIST *new_name;
	
	new_name = get_auth_name( ch->name );
	if ( new_name != NULL )
		return;
	else
	{
		CREATE( new_name, AUTH_LIST, 1 );
		new_name->name = STRALLOC( ch->name );
		new_name->state = AUTH_ONLINE;        /* Just entered the game */
		LINK( new_name, first_auth_name, last_auth_name, next, prev );
		save_auth_list();
	}
}

void remove_from_auth( char *name )
{
	AUTH_LIST *old_name;

	old_name = get_auth_name( name );
	if ( old_name == NULL ) /* Its not old */
		return;
	else
	{
		UNLINK( old_name, first_auth_name, last_auth_name, next, prev );
		if( old_name->authed_by )
			STRFREE( old_name->authed_by );
		if( old_name->change_by )
			STRFREE( old_name->change_by );
		if( old_name->denied_by )
			STRFREE( old_name->denied_by );
		STRFREE( old_name->name );
		DISPOSE( old_name );
		save_auth_list();
	}
}

void check_auth_state( CHAR_DATA *ch )
{
	AUTH_LIST *old_auth;
      CMDTYPE *command;
      int level = LEVEL_IMMORTAL;
	char buf[MAX_STRING_LENGTH];
	char *name;
	int x, y;

      command = find_command( "authorize" );
      if ( !command )
 	  level = LEVEL_IMMORTAL;
      else
	  level = command->level;
	
	old_auth = get_auth_name( ch->name );
	if ( old_auth == NULL )
		return;
		
	if ( old_auth->state == AUTH_OFFLINE /* checking as they enter the game */
	|| old_auth->state == AUTH_LINK_DEAD )
	{
		old_auth->state = AUTH_ONLINE;
		save_auth_list();
	}
	else if ( old_auth->state == AUTH_CHANGE_NAME )
	{
		ch_printf_color(ch,
		  "&R\n\rAdministratorzy muda nie zaakceptowali twego \n\r"
		  "imienia %s. Musisz wybrac inne imie.\n\r"
  		  "Zobacz 'help name'.\n\r", ch->nwol);
	}
	else if ( old_auth->state == AUTH_DENIED )
	{
		set_char_color( AT_RED, ch );
		send_to_char( "Zabrania ci sie wstepu na Muda z takim imieniem.\n\r", ch );
		remove_from_auth( ch->name );
    	    
	    	quitting_char = ch;
	    	save_char_obj( ch );
	    	saving_char = NULL;
	    	extract_char( ch, TRUE );
	    	for ( x = 0; x < MAX_WEAR; x++ )
			for ( y = 0; y < MAX_LAYERS; y++ )
			    save_equipment[x][y] = NULL;
				
		name = capitalize( ch->name );
		sprintf( buf, "%s%c/%s", PLAYER_DIR, tolower(name[0]), name );
		if ( !remove( buf ) )
		{
		    sprintf( buf, "Pre-Auth %s denied. Player file destroyed.\n\r", name );
			to_channel( buf, CHANNEL_AUTH, "Auth", level );
		}
	}
	else if ( old_auth->state == AUTH_AUTHED )
	{
		if ( ch->pcdata->authed_by )
		    STRFREE( ch->pcdata->authed_by );
		if( old_auth->authed_by )
		{
			ch->pcdata->authed_by = QUICKLINK( old_auth->authed_by );
			STRFREE( old_auth->authed_by );
		}
		else
			ch->pcdata->authed_by = STRALLOC( "The Code" );
		
	      	ch_printf_color( ch,                                            
		  "\n\r&GAdministratorzy zaakceptowali Twe imie %s.\n\r"
		  "Mozesz przemierzac %s.\n\r", ch->nwol, sysdata.mud_name );
		REMOVE_BIT(ch->pcdata->flags, PCFLAG_UNAUTHED);
		remove_from_auth( ch->name ); 
		return;
	}
	return;
}

/* new auth */
void do_authorize( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim = NULL;
    AUTH_LIST *auth;
    CMDTYPE *command;
    int level = LEVEL_IMMORTAL;
    bool offline, authed, changename, denied, pending;
    char *name;
    int x, y;

    offline = authed = changename = denied = pending = FALSE;
    auth = NULL;
  
    /* Checks level of authorize command, for log messages. - Samson 10-18-98 */
    command = find_command( "authorize" );
    if ( !command )
 	level = LEVEL_IMMORTAL;
    else
	level = command->level;

    set_char_color( AT_IMMORT, ch );

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' )
    {
	send_to_char( "By zautoryzowac postac wpisz:             auth <imie>\n\r", ch );
	send_to_char( "By wyrzucic oczekujaca postac wpisz:      auth <imie> no\n\r", ch );
	send_to_char( "By nakazac postaci zmiane imienia wpisz:  auth <imie> name\n\r", ch );
      send_to_char( "By zweryfikowac liste oczekujacych:       auth fixlist\n\r", ch );

      set_char_color( AT_DIVIDER, ch );
      send_to_char("\n\r--- Postacie oczekujace na autoryzacje ---\n\r", ch);

	for ( auth = first_auth_name; auth; auth = auth->next )
	{
	 	 if ( auth->state == AUTH_CHANGE_NAME )
			changename = TRUE;
		 else if ( auth->state == AUTH_AUTHED )
			authed = TRUE;
		 else if ( auth->state == AUTH_DENIED )
			denied = TRUE;
		 
		 if ( auth->name != NULL &&  auth->state < AUTH_CHANGE_NAME)
		 	pending = TRUE;

	}
	if ( pending )
	{
		for ( auth = first_auth_name; auth; auth = auth->next )
		{
			if ( auth->state < AUTH_CHANGE_NAME )
			{
				switch( auth->state )
				{
					default:
						sprintf( buf, "Stan nieznany?" );
						break;
					case AUTH_LINK_DEAD:
						sprintf( buf, "Martwy Link" );
						break;
					case AUTH_ONLINE:
						sprintf( buf, "Aktualnie w grze" );
						break;
					case AUTH_OFFLINE:
						sprintf( buf, "Aktualnie poza gra" );
						break;
				}
					
				ch_printf( ch, "\t%s\t\t%s\n\r", auth->name, buf );
			}
		}
	}
	
	else
		send_to_char( "\tHmm...nikt nie czeka...\n\r", ch );
		
	if ( authed )
	{
		set_char_color( AT_DIVIDER, ch );
		send_to_char("\n\rZautoryzowane postacie:\n\r", ch );
		send_to_char("---------------------------------------------\n\r", ch );
		for ( auth = first_auth_name; auth; auth = auth->next )
		{
			if ( auth->state == AUTH_AUTHED )
				ch_printf( ch,"Imie: %s\t Zautoryzowane przez: %s\n\r", 
				auth->name, auth->authed_by );
		}
	}
	if ( changename )
	{
		set_char_color( AT_DIVIDER, ch );
		send_to_char("\n\rDo zmiany imienia:\n\r", ch );
		send_to_char("---------------------------------------------\n\r", ch );
		for ( auth = first_auth_name; auth; auth = auth->next )
		{
			if ( auth->state == AUTH_CHANGE_NAME )
				ch_printf( ch,"Imie: %s\t Zmiana zazadana przez: %s\n\r", 
				auth->name, auth->change_by );
		}
	}
	if ( denied )
	{
		set_char_color( AT_DIVIDER, ch );
		send_to_char("\n\rWyrzucone postacie:\n\r", ch );
		send_to_char("---------------------------------------------\n\r", ch );
		for ( auth = first_auth_name; auth; auth = auth->next )
		{
			if ( auth->state == AUTH_DENIED )
				ch_printf( ch,"Imie: %s\t Wyrzucone przez: %s\n\r", 
				auth->name, auth->denied_by );
		}
 	}
 	return;
    }

    if ( !str_cmp( arg1, "fixlist" ) )
    {
	send_to_pager( "Czyszcze liste...\n\r", ch );
	clear_auth_list();
	send_to_pager( "Zrobione.\n\r", ch );
	return;
    }

    auth = get_auth_name( arg1 ); 
    if ( auth != NULL )
    {
	if ( auth->state == AUTH_OFFLINE )
	{
	    offline = TRUE;
	    if ( arg2[0]=='\0' || !str_cmp( arg2,"accept" ) || !str_cmp( arg2,"yes" ))
	    {
		auth->state = AUTH_AUTHED;
		auth->authed_by = QUICKLINK( ch->name );
		save_auth_list();
		sprintf( buf, "%s: zautoryzowane", auth->name);
		to_channel( buf, CHANNEL_AUTH, "Auth", level );
		ch_printf( ch, "Zautoryzowales postac o imieniu %s.\n\r", auth->name );
		return;
	    }
	    else if ( !str_cmp( arg2, "no" ) || !str_cmp( arg2, "deny" ) )
	    {
		auth->state = AUTH_DENIED;
		auth->denied_by = QUICKLINK( ch->name );
		save_auth_list();
		sprintf( buf, "%s: wyrzucone (deny)", auth->name );
		to_channel( buf, CHANNEL_AUTH, "Auth", level );
		ch_printf( ch, "Wyrzucasz %s.\n\r", auth->name );
	/* Addition so that denied names get added to reserved list - Samson 10-18-98 */
		sprintf( buf, "%s add", auth->name );
		do_reserve( ch, buf ); /* Samson 7-27-98 */
		return;
	    }
	    else if ( !str_cmp( arg2, "name" ) || !str_cmp(arg2, "n" ) )
  	    {
		auth->state = AUTH_CHANGE_NAME;
		auth->change_by = QUICKLINK( ch->name );
		save_auth_list();
		sprintf( buf, "%s: zazadana zmiana imiena", auth->name );
		to_channel( buf, CHANNEL_AUTH, "Auth", level );
		ch_printf( ch, "Nakazales postaci o imieniu %s, zmiane imienia.\n\r", auth->name );
	/* Addition so that requested name changes get added to reserved list - Samson 10-18-98 */
		sprintf( buf, "%s add", auth->name );
		do_reserve(ch, buf);
		return;
	    }
	    else
	    {
		send_to_char("Niewlasciwy parametr.\n\r", ch);
		return;
	    }
	}
	else
	{
	    victim = get_waiting_desc( ch, arg1 );
	    if ( victim == NULL )
		return;
	  
	    set_char_color( AT_IMMORT, victim );
	    if ( arg2[0]=='\0' || !str_cmp( arg2,"accept" ) || !str_cmp( arg2,"yes" ))
	    {
		if ( victim->pcdata->authed_by )
		    STRFREE( victim->pcdata->authed_by );
		victim->pcdata->authed_by = QUICKLINK( ch->name );
		sprintf( buf, "%s: authorized", victim->name );
		to_channel( buf, CHANNEL_AUTH, "Auth", level );
	
		ch_printf( ch, "Zautoryzowales postac o imieniu %s.\n\r", victim->name );
  	  
		ch_printf_color( victim,
		    "\n\r&GAdministratorzy zaakceptowali Twe imie %s.\n\r"
		    "Mozesz przemiezac %s.\n\r", victim->name, sysdata.mud_name );
		REMOVE_BIT(victim->pcdata->flags, PCFLAG_UNAUTHED);
		remove_from_auth( victim->name ); 
  		return;
	    }
	    else if ( !str_cmp( arg2, "no" ) || !str_cmp( arg2, "deny" ) )
	    {
		send_to_char_color( "&RPostac o takim imieniu nie ma dostepu do tego muda.\n\r", victim );
		sprintf( buf, "%s: zabroniony dostep (deny)", victim->name );
		to_channel( buf, CHANNEL_AUTH, "Auth", level );
		ch_printf( ch, "Zostajesz wyrzucony %s.\n\r", victim->name );
		remove_from_auth( victim->name );
	/* Addition to add denied names to reserved list - Samson 10-18-98 */
      	sprintf( buf, "%s add", victim->name );
		do_reserve(ch, buf);
#ifdef SOLANCODE
		if ( ch->level == 1 )
		    do_quit( ch, "yes" );
#else
		if ( ch->level == 1 )
		    do_quit( ch, "" );
#endif
		else
		{
		    quitting_char = victim;
		    save_char_obj( victim );
		    saving_char = NULL;
		    extract_char( victim, TRUE );
		    for ( x = 0; x < MAX_WEAR; x++ )
			for ( y = 0; y < MAX_LAYERS; y++ )
			    save_equipment[x][y] = NULL;
			
		    name = capitalize( victim->name );
		    sprintf( buf, "%s%c/%s", PLAYER_DIR, tolower(name[0]), name );
		    if ( !remove( buf ) )
		    {
			sprintf( buf, "Pre-Auth %s wyrzucony. Plik gracza zostaje zniszczony.\n\r", name );
			to_channel( buf, CHANNEL_AUTH, "Auth", level );
		    }
		}
	    }	
  	    else if ( !str_cmp( arg2, "name" ) || !str_cmp(arg2, "n" ) )
  	    {
		auth->state = AUTH_CHANGE_NAME;
		auth->change_by = QUICKLINK( ch->name );
		save_auth_list();
		sprintf( buf, "%s: nakazana zmiana imienia", victim->name );
		to_channel( buf, CHANNEL_AUTH, "Auth", level );
		ch_printf_color(victim,
		  "&R\n\rAdministratorzy muda nie zaakceptowali twego \n\r"
		  "imienia %s. Musisz wybrac inne imie.\n\r"
  		  "Zobacz 'help name'.\n\r", victim->nwol);
		ch_printf( ch, "Nakazales postaci o imieniu %s, zmiane imienia.\n\r", victim->name);
	/* Addition to put denied name on reserved list - Samson 10-18-98 */
	      sprintf( buf, "%s add", victim->name );
		do_reserve(ch, buf);
		return;
  	    }
  	    else
  	    {
		send_to_char("Niewlasciwy parametr.\n\r", ch);
		return;
	    } 
	} 
    }
    else
    {
	send_to_char( "Nikt taki nie oczekuje na autoryzacje.\n\r", ch );
	return;
    }
}

/* new auth */
void do_name( CHAR_DATA *ch, char *argument )
{
  char fname[1024];
  struct stat fst;
  CHAR_DATA *tmp;
  AUTH_LIST *auth_name;
  
  auth_name = NULL;
  auth_name = get_auth_name( ch->name );
  if ( auth_name == NULL )
  {
  	send_to_char( "Ze co?\n\r", ch );
	return;
  }

  argument[0] = UPPER(argument[0]);

  if (!check_parse_name(argument, TRUE))
  {
    send_to_char("Niewlasciwe imie, wybierz inne.\n\r", ch);
    return;
  }

  if (!str_cmp(ch->name, argument))
  {
    send_to_char("To JUZ jest twoje imie!\n\r", ch);
    return;
  }

  for ( tmp = first_char; tmp; tmp = tmp->next )
  {
    if (!str_cmp(argument, tmp->name))
    break;
  }

  if ( tmp )
  {
    send_to_char("To imie jest juz zajete.  Wybierz inne.\n\r", ch);
    return;
  }

  sprintf( fname, "%s%c/%s", PLAYER_DIR, tolower(argument[0]),
                        capitalize( argument ) );
  if ( stat( fname, &fst ) != -1 )
  {
    send_to_char("To imie jest juz zajete.  Wybierz inne.\n\r", ch);
    return;
  }
  sprintf( fname, "%s%c/%s", PLAYER_DIR, tolower(ch->name[0]),
			capitalize(ch->name) );
  unlink(fname); /* cronel, for auth */


  STRFREE( ch->name );
  ch->name = STRALLOC( argument );
  STRFREE( ch->pcdata->filename );
  ch->pcdata->filename = STRALLOC( argument ); 
  send_to_char("Twoje imie zostalo zmienione.\n\r", ch );
  auth_name->name = STRALLOC( argument );
  auth_name->state = AUTH_ONLINE;
  if ( auth_name->change_by )
	STRFREE( auth_name->change_by );
  save_auth_list();
  send_to_char( "Uzyj komendy ODMIEN by podac prawidlowa odmiane swego imienia\n\r", ch );
  return;
}  

/* changed for new auth */
void do_mpapplyb( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];
    CMDTYPE *command;
    int level = LEVEL_IMMORTAL;

    /* Checks to see level of authorize command.
       Makes no sense to see the auth channel if you can't auth. - Samson 12-28-98 */
    command = find_command( "authorize" );
    
    if ( !command )
       level = LEVEL_IMMORTAL;
    else
       level = command->level;


	if ( !IS_NPC( ch ) || ch->desc || IS_AFFECTED( ch, AFF_CHARM ))
	{
		send_to_char( "Ze co?\n\r", ch );
		return;
	}
	
	if (argument[0] == '\0')
	{
		progbug("Mpapplyb - bad syntax", ch );
		return;
	}

  	if ( (victim = get_char_room( ch, argument ) ) == NULL )
  	{
  	  progbug("Mpapplyb - no such player in room.", ch );
  	  return;
  	}

	if ( !victim->desc )
	{
		send_to_char( "Not on linkdeads.\n\r", ch );
		return;
	}

	if ( victim->fighting )
	    stop_fighting( victim, TRUE );
	char_from_room(victim);
	char_to_room(victim, get_room_index(ROOM_VNUM_SCHOOL));
	act( AT_WHITE, "$n pojawia sie w tym swiecie wsrod blyskow swiatla!",
	    victim, NULL, NULL, TO_ROOM );
        do_look(victim, "auto");
	if ( NOT_AUTHED( victim ) )
	{
	   sprintf( log_buf, "%s [%s@%s] Nowy gracz wchodzi do gry.\n\r", victim->name, victim->desc->user,
	  	victim->desc->host );

	   to_channel( log_buf, CHANNEL_AUTH, "Auth", level );
	   sprintf( buf, "\n\rDolaczasz do gry...\n\r"
			"Jednakze, twoja postac nie zostala jeszcze zautoryzowana i nie\n\r"
			"mozesz zdobyc poziomu powyzej piatego.Twoja postac bedzie zachowana,\n\r"
			"jednak nie wszystko bedzie Ci na razie dostepne na %s.\n\r", sysdata.mud_name );
	   send_to_char( buf, victim );
	}

	return;
}

/* changed for new auth */
void auth_update( void ) 
{ 
    AUTH_LIST *auth;
    char buf [MAX_INPUT_LENGTH], log_buf [MAX_INPUT_LENGTH];
    CMDTYPE *command;
    DESCRIPTOR_DATA *d;
    int level = LEVEL_IMMORTAL;
    bool found_imm = FALSE;	      /* Is at least 1 immortal on? */
    bool found_hit = FALSE;         /* was at least one found? */

    command = find_command( "authorize" );
    if ( !command )
 	level = LEVEL_IMMORTAL;
    else
	level = command->level;

	strcpy( log_buf, "--- Postacie oczekujace na autoryzacje ---\n\r" );
	for ( auth = first_auth_name; auth; auth = auth->next )
	{
		if ( auth != NULL && auth->state < AUTH_CHANGE_NAME )
		{
			found_hit = TRUE;
			sprintf( buf, "Imie: %s      Status: %s\n\r", auth->name, ( auth->state == AUTH_ONLINE ) ? "Aktualnie w grze" : "Aktualnie poza gra" ); 
			strcat( log_buf, buf );
		}
	}
	if ( found_hit )
      {
        for ( d = first_descriptor; d; d=d->next )
          if ( d->connected == CON_PLAYING
  	      && d->character
  	      && IS_IMMORTAL(d->character)
	      && d->character->level >= level )
            found_imm = TRUE;

        if ( found_imm )
	      to_channel( log_buf, CHANNEL_AUTH, "Auth", level );
	}
}

RESERVE_DATA *first_reserved;
RESERVE_DATA *last_reserved;

void save_reserved	args( ( void ) );
void sort_reserved	args( ( RESERVE_DATA *pRes ) );

/* Modified to require an "add" or "remove" argument in addition to name - Samson 10-18-98 */
void do_reserve(CHAR_DATA *ch, char *argument)
{
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  RESERVE_DATA *res;

  set_char_color( AT_PLAIN, ch );
  
  argument = one_argument(argument, arg);
  argument = one_argument(argument, arg2);

  if (!*arg)
  {
    int wid = 0;
    
    send_to_char("By dodac imie: reserve <imie> add\n\rBy usunac imie: reserve <imie> remove\n\r", ch );
    send_to_char("\n\r-- Zarezerwowane imiona --\n\r", ch);
    for (res = first_reserved; res; res = res->next)
    {
      ch_printf(ch, "%c%-17s ", (*res->name == '*' ? '*' : ' '),
          (*res->name == '*' ? res->name+1 : res->name));
      if (++wid % 4 == 0)
        send_to_char("\n\r", ch);
    }
    if (wid % 4 != 0)
      send_to_char("\n\r", ch);
    return;
  }
  
  if ( !str_cmp( arg2, "remove" ) )
  {
     for (res = first_reserved; res; res = res->next)
       if (!str_cmp(arg, res->name))
       {
         UNLINK(res, first_reserved, last_reserved, next, prev);
         DISPOSE(res->name);
         DISPOSE(res);
         save_reserved();
         send_to_char("Imie zostalo usuniete z listy zarezerwowanych.\n\r", ch);
         return;
       }
     ch_printf( ch, "Imie %s nie znajduje sie na liscie.\n\r", arg );
     return;
     
  }
  
  if ( !str_cmp( arg2, "add" ) )
  {
     	for ( res = first_reserved; res; res = res->next )
	  if ( !str_cmp(arg, res->name) )
	  {
	     ch_printf( ch, "Imie %s juz bylo zarezerwowane.\n\r", arg );
	     return;
	  }

      CREATE(res, RESERVE_DATA, 1);
      res->name = str_dup(arg);
      sort_reserved(res);
      save_reserved();
      send_to_char("Imie zostalo zarezerwowane.\n\r", ch);
      return;
  }
  send_to_char( "Niewlasciwy parametr.\n\r", ch );
}
