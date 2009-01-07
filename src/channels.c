/******************************************************************************
 *									      *
 *		To kiedys beda porzadne kanaly :)))			      *
 *		( jak Bog da :P ) - Crevan				      *
 *									      *
 ******************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#ifdef sun
  #include <strings.h>
#endif
#include <time.h>
#include "mud.h"

CHAN_DATA *first_channel;
CHAN_DATA *last_channel;

bool check_channel( CHAR_DATA *ch, char *command )
{
    if ( !is_name( command, ch->channels ) )
	return FALSE;
    
    return TRUE;
}

bool check_channel_off( CHAR_DATA *ch, char *command )
{
    if ( !check_channel( ch, command ) )
	return TRUE;
    
    if ( is_name( command, ch->channels_off ) )
	return TRUE;
    
    return FALSE;
}

bool add_channel( CHAR_DATA *ch, char *command )
{
   char buf[MAX_INPUT_LENGTH];

   if ( check_channel( ch, command ) )
             return FALSE;

    buf[0] = '\0';

   if ( ch->channels[0] != '\0' ){
      sprintf( buf, ch->channels );
      strcat( buf, " " );
   }
   strcat( buf, command );

   STRFREE( ch->channels );
   ch->channels = STRALLOC( buf );

   return TRUE;
}

bool add_channel_off( CHAR_DATA *ch, char *command )
{
    char buf[MAX_INPUT_LENGTH];
    
    if ( is_name( command, ch->channels_off ) )
	return FALSE;    
    
    buf[0] = '\0';
    
    if ( ch->channels_off[0] != '\0' ){
	sprintf( buf, ch->channels );
	strcat( buf, " " );
    }
    strcat( buf, command );
    
    STRFREE( ch->channels_off );
    ch->channels_off = STRALLOC( buf );
    
    return TRUE;
}

bool remove_channel_off( CHAR_DATA *ch, char *command )
{
   char* chan;
   char arg[256];
   char buf[MAX_STRING_LENGTH];

   if ( !check_channel_off( ch, command ) )
             return FALSE;

   buf[0] = '\0';

   for( chan = one_argument( ch->channels_off, arg ) ; chan != NULL && arg[0] != '\0' ; ){
      if ( str_cmp( command, arg ) && check_channel( ch, arg ) ){
          strcat( buf, " " );
          strcat( buf, arg );
      }
      chan = one_argument(chan, arg);
   }

   STRFREE( ch->channels_off );
   if ( buf[0] != '\0')
       ch->channels_off = STRALLOC( buf+1 );
   else 
       ch->channels_off = STRALLOC( "" );

   return TRUE;
}

bool remove_channel( CHAR_DATA *ch, char *command )
{
   char* chan;
   char arg[256];
   char buf[MAX_STRING_LENGTH];

   if ( !check_channel( ch, command ) )
             return FALSE;

   buf[0] = '\0';

   for( chan = one_argument( ch->channels, arg ) ; chan != NULL && arg[0] != '\0' ; ){
      if ( str_cmp( command, arg ) ){
          strcat( buf, " " );
          strcat( buf, arg );
      }
      chan = one_argument(chan, arg);
   }

   STRFREE( ch->channels );
   if ( buf[0] != '\0') 
       ch->channels = STRALLOC( buf+1 );
   else 
       ch->channels = STRALLOC( "" );
   remove_channel_off( ch, command );

   return TRUE;
}



void listen_this_channel( CHAR_DATA *ch, char *command )
{
   DESCRIPTOR_DATA *d;
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *och;
   int col = 0;

   sprintf( buf, "Na kanale '%s' obecni sa:\n\r", command );
   send_to_char( buf, ch );
   for( d = first_descriptor; d; d = d->next ){
       och = d->original ? d->original : d->character;

       if ( d->connected != CON_PLAYING )
               continue;

       if ( !check_channel( och, command ) || !can_see( ch, och ) || IS_IMMORTAL( och ) )
               continue;

       sprintf( buf, "%-13s%5s%3s%3s  ", och->name, (check_channel_off(och,command))?("(off)"):(""),
                (xIS_SET( och->act, PLR_SILENCE ))? "(s)" : "",
                (xIS_SET( och->act, PLR_NO_EMOTE ))? "(e)" : "" );
                
       send_to_char( buf, ch );
       if ( ++col % 3 == 0 ) send_to_char( "\n\r", ch );
   }
   send_to_char( "\n\r", ch );
   return;
}

void send_emote_to_virtual_channel( CHAR_DATA *ch, char *command, char *emote )
{
   char buf[MAX_STRING_LENGTH];
   DESCRIPTOR_DATA *d;
   CHAR_DATA *och;
   CHAN_DATA *chan;

    chan = get_channel( command );

   if ( !IS_NPC(ch) && xIS_SET(ch->act, PLR_NO_EMOTE) ){
       send_to_char( "Nie mozesz okazywac swoich emocji.\n\r", ch );
       return;
   }

   remove_channel_off( ch, command );

   for( d = first_descriptor; d != NULL; d = d->next ){
       och = d->original ? d->original : d->character;

       if ( d->connected != CON_PLAYING )
               continue;

       if ( check_channel_off( och, command ) )
               continue;

	if ( IS_SET( d->character->in_room->room_flags, ROOM_SILENCE ) )
		continue;

       sprintf( buf, "[%s] %s %s\n\r", command,
                (can_see(och,ch) || IS_IMMORTAL(ch) ) ? ( ch->name ) : ( "Someone" ), emote );
       set_char_color( chan->color, ch );
       send_to_char( buf, d->character );
   }
   if ( chan->is_logged != 0 )
       log_string_plus( buf, LOG_NORMAL, MAX_LEVEL );
   return;
}

void send_message_to_virtual_channel( CHAR_DATA *ch, char *command, char *message )
{
   char buf[MAX_STRING_LENGTH];
   DESCRIPTOR_DATA *d;
   CHAR_DATA *och;
   CHAN_DATA *chan;
    
    chan = get_channel( command );

   remove_channel_off( ch, command );

   for( d = first_descriptor; d != NULL; d = d->next ){
       och = d->original ? d->original : d->character;

       if ( d->connected != CON_PLAYING )
               continue;

       if ( check_channel_off( och, command ) )
               continue;

	if ( IS_SET( d->character->in_room->room_flags, ROOM_SILENCE ) )
		continue;
        
       set_char_color( chan->color, ch );       
       sprintf( buf, "[%s] %s: %s\n\r", command,
                (can_see(och,ch) || IS_IMMORTAL( ch )) ? ( ch->name ) : ( "Someone" ), message );
       send_to_char( buf, d->character );
   }
   if ( chan->is_logged != 0 )
       log_string_plus( buf, LOG_NORMAL, MAX_LEVEL );
   return;
}

void act_for_channels( const char *format, CHAR_DATA *ch, CHAR_DATA *vch, CHAR_DATA *looker, char *buf )
{
    static char * const he_she	[] = { "it",  "he",  "she" };
    static char * const him_her	[] = { "it",  "him", "her" };
    static char * const his_her	[] = { "its", "his", "her" };

    const char *str;
    const char *i;
    char *point;

    if ( format == NULL || format[0] == '\0' )
	return;


	point	= buf;
	str	= format;
	while ( *str != '\0' ){
	    if ( *str != '$' ){
		*point++ = *str++;
		continue;
	    }
	    ++str;

	    if ( vch == NULL && *str >= 'A' && *str <= 'Z' ){
		bug( "Act_for_channels: missing vch for code %d.", *str );
		i = " <@@@> ";
	    }
	    else{
		switch ( *str ){
		default:  bug( "Act_for_channels: bad code %d.", *str );
			  i = " <@@@> ";				break;
		case 'n': i = (can_see(looker,ch) || !IS_IMMORTAL(ch)) ? ( ch->name ) : ("Someone"); break;
		case 'N': i = (can_see(looker,vch) || !IS_IMMORTAL(vch)) ? ( vch->name ) : ("Someone"); break;
		case 'c': i = (can_see(looker,ch) || !IS_IMMORTAL(ch)) ? ( ch->ncel ) : ("Someone"); break;
		case 'C': i = (can_see(looker,vch) || !IS_IMMORTAL(vch)) ? ( vch->ncel ) : ("Someone"); break;
		case 'x': i = (can_see(looker,ch) || !IS_IMMORTAL(ch)) ? ( ch->ndop ) : ("Someone"); break;
		case 'X': i = (can_see(looker,vch) || !IS_IMMORTAL(vch)) ? ( vch->ndop ) : ("Someone"); break;		
		case 'b': i = (can_see(looker,ch) || !IS_IMMORTAL(ch)) ? ( ch->nbie ) : ("Someone"); break;
		case 'B': i = (can_see(looker,vch) || !IS_IMMORTAL(vch)) ? ( vch->nbie ) : ("Someone"); break;		
		case 'z': i = (can_see(looker,ch) || !IS_IMMORTAL(ch)) ? ( ch->nnar ) : ("Someone"); break;
		case 'Z': i = (can_see(looker,vch) || !IS_IMMORTAL(vch)) ? ( vch->nnar ) : ("Someone"); break;		
		case 'g': i = (can_see(looker,ch) || !IS_IMMORTAL(ch)) ? ( ch->nmie ) : ("Someone"); break;
		case 'G': i = (can_see(looker,vch) || !IS_IMMORTAL(vch)) ? ( vch->nmie ) : ("Someone"); break;		
		case 'w': i = (can_see(looker,ch) || !IS_IMMORTAL(ch)) ? ( ch->nwol ) : ("Someone"); break;
		case 'W': i = (can_see(looker,vch) || !IS_IMMORTAL(vch)) ? ( vch->nwol ) : ("Someone"); break;		
		case 'e': i = he_she  [URANGE(0, ch  ->sex, 2)];	break;
		case 'E': i = he_she  [URANGE(0, vch ->sex, 2)];	break;
		case 'm': i = him_her [URANGE(0, ch  ->sex, 2)];	break;
		case 'M': i = him_her [URANGE(0, vch ->sex, 2)];	break;
		case 's': i = his_her [URANGE(0, ch  ->sex, 2)];	break;
		case 'S': i = his_her [URANGE(0, vch ->sex, 2)];	break;
		}
	    }

	    ++str;
	    while ( ( *point = *i ) != '\0' )
		++point, ++i;
	}
	*point++ = '\0';
 	buf[0]   = UPPER(buf[0]);

    return;
}


void send_social_to_virtual_channel( CHAR_DATA *ch, char *command, char *social, char *argument )
{
   char buf[MAX_STRING_LENGTH];
   char buf2[MAX_STRING_LENGTH];
   DESCRIPTOR_DATA *d;
   CHAR_DATA *och;
   CHAR_DATA *victim;
   SOCIALTYPE *asocial;
   const char *format;
   CHAN_DATA *chan;

   chan = get_channel( command );
   
   asocial = find_social( social );
   if ( !asocial )
   {
       send_to_char( "Nie ma takiego sociala!\n\r", ch );
       return;
   }

   if ( !IS_NPC(ch) && xIS_SET(ch->act, PLR_NO_EMOTE) )
   {
       send_to_char( "Nie mozesz okazywac swoich emocji!\n\r", ch );
       return;
   }

   victim = NULL;
   if ( argument[0] == '\0' ) format = asocial->others_no_arg;
   else if ( ( ( victim = get_char_world( ch, argument ) ) == NULL ) || IS_NPC( victim ) )
   {
       send_to_char( "Nikogo takiego tu nie ma.\n\r", ch );
       return;
   }
   else if ( !check_channel( victim, command ) )
   {
	sprintf( buf2, "%s nie jest dolaczony do kanalu '%s'\n\r", victim->name, command );
	send_to_char( buf2, ch );
	return;
   }
   else if ( check_channel_off( victim, command ) )
   {
	sprintf( buf2, "%s ma wlasnie wylaczony kanal '%s'\n\r", victim->name, command );
	send_to_char( buf2, ch );
	return;
   }
   else if ( victim == ch ) format = asocial->others_auto;
   else format = asocial->others_found;

   if ( format == NULL || format[0] == '\0' )
   {
       send_to_char( "Nie mozesz uzyc tego sociala z takim parametrem na kanale.\n\rSkontaktuj sie z IMMO i napisz jaki to social i jaki parametr!\n\r", ch );
       return;
   }

   remove_channel_off( ch, command );
   for( d = first_descriptor; d != NULL; d = d->next )
   {
       och = d->original ? d->original : d->character;

       if ( d->connected != CON_PLAYING )
               continue;

       if ( check_channel_off( och, command ) )
               continue;

	if ( IS_SET( d->character->in_room->room_flags, ROOM_SILENCE ) )
		continue;

       act_for_channels( format, ch, victim, och, buf );
       set_char_color( chan->color, ch );
       sprintf( buf2, "[%s] %s\n\r", command, buf );
       send_to_char( buf2, d->character );
   }
   if ( chan->is_logged != 0 )
       log_string_plus( buf, LOG_NORMAL, MAX_LEVEL );
   return;
}

bool check_virtual_channels( CHAR_DATA *ch, char *command, char *message )
{
   char buf[MAX_STRING_LENGTH];
   char social[MAX_INPUT_LENGTH];

    if ( IS_NPC( ch ) )
	return FALSE;

   if ( ch == NULL || command == NULL || message == NULL || command[0] == '\0' )
        return FALSE;

   if ( !check_channel( ch, command ) )
        return FALSE;

   if ( IS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
       return FALSE;
   
   if ( message[0] == '\0' )
            listen_this_channel( ch, command );
   else if ( !str_cmp( message, "wylacz" ) ){
                if ( add_channel_off( ch, command ) )
                     sprintf( buf, "Wylaczyl%ss kanal '%s'.\n\r", 
		     ( ch->sex == SEX_FEMALE ) ? "a" : "e", command );
                else sprintf( buf, "Kanal '%s' jest juz wylaczony.\n\r", command );
                send_to_char( buf, ch );
             }
   else if ( !str_cmp( message, "wlacz" ) ){
                if ( remove_channel_off( ch, command ) )
                     sprintf( buf, "Wlaczyl%ss kanal '%s'.\n\r", 
		     ( ch->sex == SEX_FEMALE ) ? "a" : "e", command );
                else sprintf( buf, "Kanal '%s' jest juz wlaczony.\n\r", command );
                send_to_char( buf, ch );
             }
   else if ( xIS_SET( ch->act, PLR_SILENCE ) ){
	sprintf( buf, "Nie mozesz uzywac kanalow, bowiem jestes uciszon%s!\n\r", 
	( ch->sex == SEX_MALE ) ? "y" : ( ch->sex == SEX_FEMALE ) ? "a" : "e" );
	send_to_char( buf, ch );
	return TRUE;
   }
   else if ( message[0] == ':' ) 
       send_emote_to_virtual_channel( ch, command, message+1 );
   else if ( message[0] == '&' ){
	message = one_argument( message+1, social );
	send_social_to_virtual_channel( ch, command, social, message );   
        
   }
   else 
       send_message_to_virtual_channel( ch, command, message );

   return TRUE;
}

CHAN_DATA *get_channel( char * name )
{
    CHAN_DATA *chan;
    
    for ( chan = first_channel; chan; chan = chan->next )
	if ( !str_cmp( name, chan->name ) )
	    return chan;
    return NULL;
}

void write_channels_list( )
{
    CHAN_DATA *chan;
    FILE *fp;
    char filename[256];
    
    sprintf( filename, "%s%s", CHANNEL_DIR, CHANNEL_LIST );
    fp = fopen( filename, "w" );
    if ( !fp )
    {
	bug( "FATALNIE: nie moge zapisywac do pliku channels.lst !\n\r" );
	return;
    }
    for ( chan = first_channel; chan; chan = chan->next )
	fprintf( fp, "%s\n", chan->filename );
    fprintf( fp, "$\n" );
    fclose( fp );
}

void save_channel( CHAN_DATA *chan )
{
    FILE *fp;
    char filename[256];
    char buf[MAX_STRING_LENGTH];

    if ( !chan )
    {
	bug( "save_channel: wskaznik do kanalu = NULL!", 0 );
	return;
    }
        
    if ( !chan->filename || chan->filename[0] == '\0' )
    {
	sprintf( buf, "save_channel: %s nie ma swojego pliku", chan->name );
	bug( buf, 0 );
	return;
    }
    
    sprintf( filename, "%s%s", CHANNEL_DIR, chan->filename );
    fclose( fpReserve );
    fp = fopen( filename, "w" );
    
    if ( !fp )
    {
    	bug( "save_channel: nie moge otworzyc pliku do pisania", 0 );
    	perror( filename );
    }
    else
    {
	fprintf( fp, "#CHANNEL\n" );
	fprintf( fp, "Name         %s~\n",	chan->name		);
	fprintf( fp, "Filename     %s~\n",	chan->filename		);
	fprintf( fp, "Creator      %s~\n",	chan->creator		);
	fprintf( fp, "Guild        %s~\n",	chan->guild		);
	fprintf( fp, "Users        %d\n",	chan->users		);
	fprintf( fp, "Race         %d\n",	chan->race_allowed	);
	fprintf( fp, "Class        %d\n",	chan->class_allowed	);
	fprintf( fp, "Sex          %d\n",	chan->sex_allowed	);
	fprintf( fp, "MinTalk      %d\n",	chan->min_talk		);
	fprintf( fp, "Log          %d\n",	chan->is_logged		);
	fprintf( fp, "Private      %d\n",	chan->is_private	);
	fprintf( fp, "Color        %d\n",	chan->color		);
	fprintf( fp, "End\n\n"						);
	fprintf( fp, "#END\n"						);
    }
    fclose( fp );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !str_cmp( word, literal ) )	\
				{					\
				    field  = value;			\
				    fMatch = TRUE;			\
				    break;				\
				}
				
void fread_channel( CHAN_DATA *chan, FILE *fp )
{
    char buf[MAX_STRING_LENGTH];
    char *word;
    bool fMatch;

    for ( ; ; )
    {
	word   = feof( fp ) ? "End" : fread_word( fp );
	fMatch = FALSE;
	switch ( UPPER(word[0]) )
	{
	case '*':
	    fMatch = TRUE;
	    fread_to_eol( fp );
	    break;

	case 'C':
	    KEY( "Class",	chan->class_allowed,	fread_number( fp ) );
	    KEY( "Color",	chan->color,		fread_number( fp ) );
	    KEY( "Creator",	chan->creator,		fread_string( fp ) );
	    break;
	
	case 'E':
	    if ( !str_cmp( word, "End" ) )
	    {
		if ( !chan->creator )	chan->creator = STRALLOC( "" );
		if ( !chan->filename )	chan->filename = str_dup( "" );
		if ( !chan->guild )	chan->guild = STRALLOC( "" );
		if ( !chan->name )	chan->name = STRALLOC( "" );
		return;
	    }
	    break;

	case 'F':
	    KEY( "Filename",	chan->filename,		fread_string_nohash( fp ) );
	    break;

	case 'G':
	    KEY( "Guild",	chan->guild,		fread_string( fp ) );
	    break;

	case 'L':
	    KEY( "Log",		chan->is_logged,	fread_number( fp ) );
	    break;

	case 'M':
	    KEY( "MinTalk",	chan->min_talk,		fread_number( fp ) );
	    break;

	case 'N':
	    KEY( "Name",	chan->name,		fread_string( fp ) );
	    break;

	case 'P':
	    KEY( "Private",	chan->is_private,	fread_number( fp ) );
	    break;

	case 'R':
	    KEY( "Race",	chan->race_allowed,	fread_number( fp ) );
	    break;
	    
	case 'S':
	    KEY( "Sex",		chan->sex_allowed,	fread_number( fp ) );
	    break;

	case 'U':
	    KEY( "Users",	chan->users,		fread_number( fp ) );
	    break;
	}	
	if ( !fMatch )
	{
	    sprintf( buf, "Fread_channel: no match: %s", word );
	    bug( buf, 0 );
	}
    }
}


bool load_channel_file( char *channelfile )
{
    char filename[256];
    CHAN_DATA *channel;
    FILE *fp;
    bool found;

    CREATE( channel, CHAN_DATA, 1 );
    
    found = FALSE;
    sprintf( filename, "%s%s", CHANNEL_DIR, channelfile );

    if ( ( fp = fopen( filename, "r" ) ) != NULL )
    {
	found = TRUE;
	for ( ; ; )
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
		bug( "Load_channel_file: # nie znaleziony.", 0 );
		break;
	    }
	    word = fread_word( fp );
	    if ( !str_cmp( word, "CHANNEL" ) )
	    {
		fread_channel( channel, fp );
		break;
	    }
	    else
	    if ( !str_cmp( word, "END"	) )
	        break;
	    else
	    {
		char buf[MAX_STRING_LENGTH];
		
		sprintf( buf, "Load_channel_file: zla sekcja %s.", word );
		bug( buf, 0 );
		break;
	    }
	}
	fclose( fp );
    }
    if ( found )
      LINK( channel, first_channel, last_channel, next, prev );
    else
      DISPOSE( channel );
    return found;
}

void load_channels( )
{
    FILE *fpList;
    char *filename;
    char chanlist[256];
    char buf[MAX_STRING_LENGTH];
    
    
    first_channel = NULL;
    last_channel  = NULL;
    
    log_string( " Loading channels..." );

    sprintf( chanlist, "%s%s", CHANNEL_DIR, CHANNEL_LIST );
    fclose( fpReserve );
    if ( ( fpList = fopen( chanlist, "r" ) ) == NULL )
    {
	perror( chanlist );
	exit( 1 );
    }
    for ( ; ; )
    {
	filename = feof( fpList ) ? "$" : fread_word( fpList );
	log_string( filename );
	if ( filename[0] == '$' )
	  break;
	
	if ( !load_channel_file( filename ) )
	{
	  sprintf( buf, "Cannot load channel file: %s", filename );
	  bug( buf, 0 );
	}
    }
    fclose( fpList );
    log_string(" Done channels." );
    fpReserve = fopen( NULL_FILE, "r" );
    return;
}


void do_makechannel( CHAR_DATA *ch, char *argument )
{
    char filename[256];
    CHAN_DATA *chan;
    
    set_char_color( AT_IMMORT, ch );

    if ( !argument || argument[0] == '\0' )
    {
	send_to_char( "Uzycie: makechannel <nazwa kanalu>\n\r", ch );
	return;
    }

    if ( get_channel( argument ) )
    {
	send_to_char( "Taki kanal juz istnieje!\n\r", ch );
	return;
    }
    
    if ( strlen( argument ) > 12 || strlen( argument ) < 2 )
    {
	send_to_char( "Nazwa powinna miec pomiedzy 2 a 12 znakow.\n\r", ch );
	return;
    }
    sprintf( filename, "%s%s", CHANNEL_DIR, strlower(argument) );

    CREATE( chan, CHAN_DATA, 1 );
    LINK( chan, first_channel, last_channel, next, prev );

    chan->name		= STRALLOC( argument );
    chan->filename	= str_dup( argument );
    chan->creator	= STRALLOC( ch->name );
    chan->guild		= STRALLOC( "" );
    chan->users		= 0;
    chan->min_talk	= 1;
    chan->is_logged	= 0;
    chan->sex_allowed	= -1;
    chan->race_allowed	= -1;
    chan->class_allowed	= -1;
    chan->color	= AT_SAY;
    set_char_color( AT_PLAIN, ch );
    send_to_char( "Zrobione.\n\r" , ch );
    save_channel( chan );
    write_channels_list( );
    return;
}

void do_setchannel( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    CHAN_DATA *chan;
    CHAR_DATA *victim;
    int tmp = 0;
    
    set_char_color( AT_PLAIN, ch );
    if ( IS_NPC( ch ) )
    {
	send_to_char( "Ze co?\n\r", ch );
	return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    
    if ( arg1[0] == '\0' )
    {
	send_to_char( "Uzycie: setchannel <kanal> user <imie gracza>\n\r", ch );
	send_to_char( "Lub   : setchannel <kanal> <pole> <wartosc>\n\r", ch );
	send_to_char( "Pole moze byc nastepujace:\n\r", ch );
	send_to_char( "guild race class sex log mintalk color\n\r", ch );
	return;
    }

    chan = get_channel( arg1 );

    if ( !chan )
    {
	send_to_char( "Nie ma takiego kanalu lub jest on obslugiwany przez tradycyjny kod.\n\r", ch );
	return;
    }
    
    if ( !str_cmp( arg2, "user" ) )
    {
	if ( !( victim = get_char_world( ch, argument ) ) )
	{
	    send_to_char( "Nie ma takiego gracza polaczonego.\n\r", ch );
	    return;
	}
	if ( victim != ch && get_trust( ch ) < get_trust( victim ) )
	{
	    send_to_char( "Nie mozesz ustawiac kanalow graczom z wyzszym priorytetem niz ty.\n\r", ch );
	    return;
	}
	if ( remove_channel( victim, arg1 ) )
	{
	    sprintf( buf, "Usunal%ss %s z kanalu '%s'.\n\r", 
	    ( ch->sex == SEX_FEMALE ) ? "a" : "e", victim->nbie, arg1 );
    	    send_to_char( buf, ch );
    	    --chan->users;
	    sprintf( buf, "%s odlaczyl %s od kanalu '%s'.", ch->name, victim->nbie, arg1 );
	    log_string_plus( buf, LOG_NORMAL, ch->level );
	    if ( ch != victim )
	    {
		sprintf( buf, "Odlaczono Cie od kanalu '%s'.\n\r", arg1 );
    		send_to_char( buf, victim );
	    }
	}
	else 
	{
    	    sprintf( buf, "Dolaczyl%ss %s do kanalu '%s'.\n\r", 
	    ( ch->sex == SEX_FEMALE ) ? "a" : "e", victim->nbie, arg1 );
    	    add_channel( victim, arg1 );
    	    send_to_char( buf, ch );
	    ++chan->users;
	    sprintf( buf, "%s dolaczyl %s do kanalu '%s'.", ch->name, victim->nbie, arg1 );
	    log_string_plus( buf, LOG_NORMAL, ch->level );
	    if ( ch != victim )
	    {
		sprintf( buf, "Dodano Tobie nowy kanal: '%s'.\n\r", arg1 );
		send_to_char( buf, victim );
	    }
	}
    }
    
    write_channels_list( );
    
    if ( !str_cmp( arg2, "guild" ) )
    {
	STRFREE( chan->guild );
	chan->guild = STRALLOC( argument );
	send_to_char( "Zrobione.\n\r", ch );
	save_channel( chan );
	return;
    }
    if ( !str_cmp( arg2, "race" ) )
    {
	tmp = atoi( argument );
	if ( tmp < -1 || tmp > MAX_RACE  )
	{
	    send_to_char( "Niedozwolona wartosc.\n\r", ch );
	    sprintf( buf, "Dozwolony zakres: od -1 (brak rasy) do %d.\n\r", MAX_RACE );
	    send_to_char( buf, ch );
	    return;
	}
	chan->race_allowed = tmp;
	send_to_char( "Zrobione.\n\r", ch );
	save_channel( chan );
	return;
    }
    if ( !str_cmp( arg2, "class" ) )
    {
	tmp = atoi( argument );
	if ( tmp < -1 || tmp > MAX_CLASS )
	{
	    send_to_char( "Niedozwolona wartosc.\n\r", ch );
	    sprintf( buf, "Dozwolony zakres: od -1 (brak klasy) do %d.\n\r", MAX_CLASS );
	    send_to_char( buf, ch );
	    return;
	}
	chan->class_allowed = tmp;
	send_to_char( "Zrobione.\n\r", ch );
	save_channel( chan );
	return;
    }
    if ( !str_cmp( arg2, "sex" ) )
    {
	tmp = atoi( argument );
	if ( tmp < 0 || tmp > 2 )
	{
	    send_to_char( "Niedozwolona wartosc.\n\r", ch );
	    sprintf( buf, "Dozwolone wartosci: 0 (nijaka), %d (meska) i %d (zenska).\n\r", 
	    SEX_MALE, SEX_FEMALE );
	    send_to_char( buf, ch );
	    return;
	}
	chan->sex_allowed = tmp;
	send_to_char( "Zrobione.\n\r", ch );
	save_channel( chan );
	return;
    }
    if ( !str_cmp( arg2, "log" ) )
    {
	tmp = atoi( argument );
	if ( tmp < 0 || tmp > 1 )
	{
	    send_to_char( "Niedozwolona wartosc.\n\r", ch );
	    send_to_char( "Dozwolone wartosci: 0 (nie logowac) i 1 (logowac).\n\r", ch );
	    return;
	}
	chan->is_logged = tmp;
	send_to_char( "Zrobione.\n\r", ch );
	save_channel( chan );
	return;
    }
    if ( !str_cmp( arg2, "mintalk" ) )
    {
	tmp = atoi( argument );
	if ( tmp < 1 || tmp > MAX_LEVEL )
	{
	    send_to_char( "Niedozwolona wartosc.\n\r", ch );
	    sprintf( buf, "Dozwolony zakres: od 1 do %d.\n\r", MAX_LEVEL );
	    send_to_char( buf, ch );
	    return;
	}
	chan->min_talk = tmp;
	send_to_char( "Zrobione.\n\r", ch );
	save_channel( chan );
	return;
    }
    if ( !str_cmp( arg2, "color" ) )
    {
	if ( !str_cmp( argument, "AT_BLACK" ) )
	    chan->color = AT_BLACK;
	else if ( !str_cmp( argument, "AT_BLOOD" )
		|| !str_cmp( argument, "AT_CARNAGE" ) )
	    chan->color = AT_BLOOD;
	else if ( !str_cmp( argument, "AT_DGREEN" )
		|| !str_cmp( argument, "AT_RESET" )
		|| !str_cmp( argument, "AT_RACETALK" ) )
	    chan->color = AT_DGREEN;
	else if ( !str_cmp( argument, "AT_ORANGE" )
		|| !str_cmp( argument, "AT_HUNGRY" ) )
	    chan->color = AT_ORANGE;
	else if ( !str_cmp( argument, "AT_DBLUE" ) )
	    chan->color = AT_DBLUE;
	else if ( !str_cmp( argument, "AT_PURPLE" )
		|| !str_cmp( argument, "AT_LOG" ) )
	    chan->color = AT_PURPLE;
	else if ( !str_cmp( argument, "AT_CYAN" )
		|| !str_cmp( argument, "AT_SOCIAL" ) )
	    chan->color = AT_CYAN;
	else if ( !str_cmp( argument, "AT_GREY" ) 
		|| !str_cmp( argument, "AT_PLAIN" )
		|| !str_cmp( argument, "AT_ACTION" )
		|| !str_cmp( argument, "AT_CONSIDER" )
		|| !str_cmp( argument, "AT_REPORT" )
		|| !str_cmp( argument, "AT_DIVIDER" )
		|| !str_cmp( argument, "AT_MORPH" ) )
	    chan->color = AT_GREY;
	else if ( !str_cmp( argument, "AT_DGREY" ) )
	    chan->color = AT_DGREY;
	else if ( !str_cmp( argument, "AT_RED" )
		|| !str_cmp( argument, "AT_HURT" )
		|| !str_cmp( argument, "AT_DEAD" )
		|| !str_cmp( argument, "AT_FIRE" )
		|| !str_cmp( argument, "AT_WARTALK" ) )
	    chan->color = AT_RED;
	else if ( !str_cmp( argument, "AT_GREEN" )
		|| !str_cmp( argument, "AT_POISON" )
		|| !str_cmp( argument, "AT_SKILL" )
		|| !str_cmp( argument, "AT_OBJECT" )
		|| !str_cmp( argument, "AT_BYE" )
		|| !str_cmp( argument, "AT_NOTE" )
		|| !str_cmp( argument, "AT_IGNORE" ) )
	    chan->color = AT_GREEN;
	else if ( !str_cmp( argument, "AT_YELLOW" )
		|| !str_cmp( argument, "AT_HITME" )
		|| !str_cmp( argument, "AT_IMMORT" )
		|| !str_cmp( argument, "AT_DYING" )
		|| !str_cmp( argument, "AT_FLEE" )
		|| !str_cmp( argument, "AT_RMDESC" )
		|| !str_cmp( argument, "AT_GOLD" )
		|| !str_cmp( argument, "AT_WEAROFF" ) )
	    chan->color = AT_YELLOW;
	else if ( !str_cmp( argument, "AT_BLUE" )
		|| !str_cmp( argument, "AT_MAGIC" )
		|| !str_cmp( argument, "AT_LIST" )
		|| !str_cmp( argument, "AT_GTELL" )
		|| !str_cmp( argument, "AT_THIRSTY" ) )
	    chan->color = AT_BLUE;
	else if ( !str_cmp( argument, "AT_PINK" )
		|| !str_cmp( argument, "AT_PERSON" ) )
	    chan->color = AT_PINK;
	else if ( !str_cmp( argument, "AT_LBLUE" )
		|| !str_cmp( argument, "AT_SAY" )
		|| !str_cmp( argument, "AT_GOSSIP" )
		|| !str_cmp( argument, "AT_SCORE" ) )
	    chan->color = AT_LBLUE;
	else if ( !str_cmp( argument, "AT_WHITE" )
		|| !str_cmp( argument, "AT_YELL" )
		|| !str_cmp( argument, "AT_TELL" )
		|| !str_cmp( argument, "AT_WHISPER" )
		|| !str_cmp( argument, "AT_HIT" )
		|| !str_cmp( argument, "AT_DAMAGE" )
		|| !str_cmp( argument, "AT_RMNAME" )
		|| !str_cmp( argument, "AT_SOBER" )
		|| !str_cmp( argument, "AT_EXITS" )
		|| !str_cmp( argument, "AT_DIEMSG" ) )
	    chan->color = AT_WHITE;
	else if ( !str_cmp( argument, "AT_BLINK" ) )
	{    
	    if ( chan->color < AT_BLINK ) 
	    {
		chan->color += AT_BLINK;
		send_to_char( "Miganie wlaczone.\n\r", ch );
	    }
	    else
	    {
		chan->color -= AT_BLINK;
		send_to_char( "Miganie wylaczone.\n\r", ch );
	    }
	}
	else if ( !str_cmp( argument, "AT_FALLING" ) )
	    chan->color = AT_FALLING;
	else if ( !str_cmp( argument, "AT_DANGER" ) )
	    chan->color = AT_DANGER;
	else {
	send_to_char( "Nieprawidlowy typ koloru.\n\r", ch );
	return;
	}
	save_channel( chan );
	return;
    }
    return;
}

void do_kanaly( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    CHAN_DATA *chan;
    int a, b;
    static char * const sex [] = { "Nijaka", "Meska", "Zenska" };
    static char * const priv [] ={ "bez ograniczen", "prywatny" };
    static char * const color [] = { "Czarny", "Krwisty", "Ciemnozielony",
    "Pomaranczowy", "Ciemnoniebieski", "Fioletowy", "Cyan?", "Szary",
    "Ciemnoszary", "Czerwony", "Zielony", "Zolty", "Niebieski", "Rozowy"
    "Jasnoniebieski", "Bialy", NULL, "+Czarny", "+Krwisty", "+Ciemnozielony",
    "+Pomaranczowy", "+Ciemnoniebieski", "+Fioletowy", "+Cyan?", "+Szary",
    "+Ciemnoszary", "+Czerwony", "+Zielony", "+Zolty", "+Niebieski", "+Rozowy",
    "+Jasnoniebieski", "+Bialy" };
    /* Wiem, ze taka metoda definiowania napisow jest nieodporna na zmiany
    ustawien, ale nie chce mi sie pisac calej procedury pobierajacej nazwe rasy
    lub klasy - Crevan */
    static char * const race [] = { "Ludzie", "Elfy", "Krasnoludy", "Halflingi",
    "Pixie", "Wampiry", "Pol-Ogry", "Pol-Orki", "Pol-Trolle", "Pol-Elfy", "Gith",
    "Mroczne Elfy", "Morskie Elfy", "Jaszczuroludzie", "Gnomy" };
    static char * const class [] = { "Magowie", "Klerycy", "Zlodzieje", "Wojownicy",
    "Wampiry", "Druidzi", "Rangerzy", "Augurerzy", "Zabojcy", "Demony", "Anioly",
    "Paladyni" };
    
    
    a = b = 0;

    if ( IS_NPC( ch ) )
	return;
    
    
    if ( ( ch->level < LEVEL_ASCENDANT ) )
    {
	if ( !str_cmp( ch->channels, "" ) )
	{
	    send_to_char( "Nie masz podlaczonych zadnych kanalow.\n\r", ch );
	    send_to_char( "Sprawa jest dziwna, skontaktuj sie z Niesmiertelnymi.\n\r", ch );
	    return;
	}
	if ( !str_cmp( argument, "wylaczone" ) )
	    sprintf( buf, ch->channels_off );
	else
	    sprintf( buf, ch->channels );

	while ( buf[a++] != '\0' )
	{
	    if ( buf[a] == ' ' )
		( ++b % 5 == 0 ) ? ( buf[a] = '\n' ) : ( buf[a] = '\t' );
	}
	send_to_char( buf, ch );
	return;
    }

    if ( !str_cmp( argument, "lista" ) )
    {
	if ( first_channel == NULL )
	{
	    send_to_char( "Obecnie nie ma zadnych kanalow.\n\r", ch );
	    return;
	}
	a = b = 0;
	set_char_color( AT_PLAIN, ch );
	send_to_char( "--------------------------------------------------------------------------------\n\r", ch );
	send_to_char( "| Nr  | Nazwa kanalu | Klan/Rasa/Klasa/Plec | ML | Kolor              | Uzytk. |\n\r", ch );
	send_to_char( "--------------------------------------------------------------------------------\n\r", ch );
	for ( chan = first_channel; chan; chan = chan->next )
	{
	    sprintf( buf, "| %-3d | %-12s | %-20s | %-2d | %-18s | %-6d |\n\r",
	    ++a, chan->name, 
	    ( !( !str_cmp( chan->guild, "" ) ) ) ? chan->guild : 
	    ( chan->sex_allowed != -1 ) ? sex[chan->sex_allowed] :
	    ( chan->race_allowed != -1 ) ? race[chan->race_allowed] :
	    ( chan->class_allowed != -1 ) ? class[chan->class_allowed] :
	    priv[chan->is_private], chan->min_talk, color[chan->color], chan->users );
	    send_to_char( buf, ch );
	}
	send_to_char( "--------------------------------------------------------------------------------\n\r", ch );
	send_to_char( "\n\rZnaczek '+' przed nazwa koloru oznacza, ze kanal ma wlaczone miganie.\n\r", ch );
	return;
    }
    else
    {
	a = b = 0;
	for ( chan = first_channel; chan; chan = chan->next )
	{
	    if ( !is_name( chan->name, ch->channels ) )
		continue;
		
	    set_char_color( AT_PLAIN, ch );
	    sprintf( buf, "%2d: ", ++a );
	    send_to_char( buf, ch );
	    set_char_color( AT_IMMORT, ch );
	    sprintf( buf, "%-12s ", chan->name );
	    if ( is_name( chan->name, ch->channels_off ) )
		strcat( buf, "(wyl.)" );
	    ( ++b % 3 == 0 ) ? strcat( buf, "\n\r" ) : strcat( buf, " " );
	    send_to_char( buf, ch );
	}
    }	
    return;
}

void correct_channels( void )
{
    CHAN_DATA *chan;
    char buf[MAX_STRING_LENGTH];
    int a = 0;
    
    if ( first_channel == NULL )
	return;
    
    for ( chan = first_channel; chan; chan = chan->next )
    {
	if ( strlen( chan->name ) > 12 )
	{
	    sprintf( buf, "Kanal %s ma za dluga nazwe, obcinam do 12 znakow.", chan->name );
	    log_string( buf );
	    sprintf( buf, chan->name );
	    buf[12] = '\0';
	    STRFREE( chan->name );
	    chan->name = STRALLOC( buf );
	}
/*	if ( !( !str_cmp( chan->guild, "" ) ) )
	{
	    if ( ( get_clan( chan->guild ) ) != NULL )
		;
	    else
	    {
		sprintf( buf, "Kanal %s ma jako wlasciciela podana niestniejaca organizacje: %s.",
		chan->name, chan->guild );
		log_string( buf );
		log_string( "Usuwam pole 'guild' z kanalu.\n\r" );
		STRFREE( chan->guild );
		chan->guild = STRALLOC( "" );
	    }
	}*/
	if ( chan->users < 1 )
	{
	    sprintf( buf, "Kanal %s nie ma ani jednego uzytkownika.", chan->name );
	    log_string( buf );
	    chan->users = 0;
	}
	if ( chan->race_allowed < -1 || chan->race_allowed > 14 )
	{
	    sprintf( buf, "Kanal %s ma bledne ustawienie pola 'race'(%d), ustawiam -1.",
	    chan->name, chan->race_allowed );
	    log_string( buf );
	    chan->race_allowed = -1;
	}
	if ( chan->class_allowed < -1 || chan->class_allowed > 11 )
	{
	    sprintf( buf, "Kanal %s ma bledne ustawienie pola 'class'(%d), ustawiam -1.",
	    chan->name, chan->class_allowed );
	    log_string( buf );
	    chan->class_allowed = -1;
	}
	if ( chan->sex_allowed < -1 || chan->sex_allowed > 2 )
	{
	    sprintf( buf, "Kanal %s ma bledne ustawienie pola 'sex'(%d), ustawiam -1.",
	    chan->name, chan->sex_allowed );
	    log_string( buf );
	    chan->sex_allowed = -1;
	}
	if ( chan->min_talk < 1 || chan->min_talk > MAX_LEVEL )
	{
	    sprintf( buf, "Kanal %s ma bledne ustawienie pola 'min_talk'(%d), ustawiam 1.",
	    chan->name, chan->min_talk );
	    log_string( buf );
	    chan->min_talk = 1;
	}
	if ( chan->is_logged < 0 )
	{
	    sprintf( buf, "Kanal %s ma bledne ustawienie pola 'log'(%d), ustawiam 0.",
	    chan->name, chan->is_logged );
	    log_string( buf );
	    chan->is_logged = 0;
	}
	if ( chan->is_private < 0 )
	{
	    sprintf( buf, "Kanal %s ma bledne ustawienie pola 'private'(%d), ustawiam 0.",
	    chan->name, chan->is_private );
	    log_string( buf );
	    chan->is_private = 0;
	}
	if ( chan->color < 0 || chan->color > 31 )
	{
	    sprintf( buf, "Kanal %s ma bledne ustawienie pola 'color'(%d), ustawiam AT_SAY.",
	    chan->name, chan->color );
	    log_string( buf );
	    chan->color = AT_SAY;
	}
	save_channel( chan );
	a++;
    }
    if ( a == MAX_CHANNELS )
	log_string( "Maksymalna liczba kanalow przekroczona!" );
    return;
}

bool can_use_channel( CHAR_DATA *ch, char *argument )
{
    CHAN_DATA *chan;
    
    chan = get_channel( argument );
    if ( !chan )
    {
	bug( "can_use_channel: bledny argument" );
	return FALSE;
    }
    if ( chan->is_private != 0 )
	return FALSE;
    if ( ( chan->race_allowed != -1 ) && ( ch->race != chan->race_allowed ) )
	return FALSE;
    if ( ( chan->sex_allowed != -1 ) && ( ch->sex != chan->sex_allowed ) )
	return FALSE;
    if ( ( chan->class_allowed != -1 ) && ( ch->class != chan->class_allowed ) )
	return FALSE;
    if ( ch->level < chan->min_talk )
	return FALSE;
    if ( IS_PKILL( ch ) && !str_cmp( chan->guild, "pkill" ) )
	return TRUE;
    if ( !( !str_cmp( chan->guild, "" ) ) && !ch->pcdata->clan )
	return FALSE;
    if ( ch->pcdata->clan && !( !str_cmp( chan->guild, ch->pcdata->clan->name ) ) )
	return FALSE;
    
    return TRUE;
}

void check_char_channels( CHAR_DATA *ch )
{
    CHAN_DATA *chan;
    bool can_use;
    int a, b;
    char buf[MAX_STRING_LENGTH];
    
    if ( first_channel == NULL )
	return;

    a = b = 0;
    for ( chan = first_channel; chan; chan = chan->next )
    {
	can_use = can_use_channel( ch, chan->name );
	if ( ( is_name( chan->name, ch->channels ) ) && !can_use 
	&& ( chan->is_private == 0 ) )
	{
	    remove_channel( ch, chan->name );
	    if ( is_name( chan->name, ch->channels_off ) )
		remove_channel_off( ch, chan->name );
	    --chan->users;
	    a++;
	    save_channel( chan );
	}
	if ( ( !is_name( chan->name, ch->channels ) ) && can_use )
	{
	    add_channel( ch, chan->name );
	    ++chan->users;
	    b++;
	    save_channel( chan );
	}
    }
    if ( a != 0 || b != 0 )
    {
	sprintf( buf, "Graczowi '%s' dodano %d kanalow, usunieto %d.\n\r", 
	ch->name, b, a );
	log_string( buf );
    }
    return;
}

void do_showchannel( CHAR_DATA *ch, char *argument )
{
    CHAN_DATA *chan;
    
    chan = get_channel( argument );
    if ( !chan )
    {
	send_to_char( "Nie ma takiego kanalu lub jest on obslugiwany przez tradycyjny kod.\n\r", ch );
	return;
    }
    ch_printf( ch, "Nazwa : %-12s\tNazwa pliku: %-12s\tTworca: %-12s\n\r",
    chan->name, chan->filename, chan->creator );
    ch_printf( ch, "Rasa  : %-2d\tKlasa : %-2d\tPlec  : %d\tKlan  :%-20s\n\r",
    chan->race_allowed, chan->class_allowed, chan->sex_allowed, chan->guild );
    ch_printf( ch, "MinLev: %-2d\tKolor : %-2d\tPryw.?: %-3s\tLog.? : %-3s\n\r",
    chan->min_talk, chan->color, ( chan->is_private != 0 ) ? "tak" : "nie",
    ( chan->is_logged != 0 ) ? "tak" : "nie" );
    ch_printf( ch, "Liczba uzytkownikow: %-4d\n\r", chan->users );
    return;
}

void do_update_channels( CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    
    victim = get_char_world( ch, argument );
    if ( !victim )
    {
	send_to_char( "Nikogo takiego tu nie ma.\n\r", ch );
	return;
    }
    check_char_channels ( victim );
    return;
}