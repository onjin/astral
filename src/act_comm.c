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
 *			   Player communication module			    *
 ****************************************************************************/


#include <sys/types.h>
#include <regex.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"


#ifndef WIN32
  #include <regex.h>
#endif
#include<regex.h>
extern char *re_comp(char *);
extern int re_exec(char *);

/*
 *  Externals
 */
void send_obj_page_to_char(CHAR_DATA * ch, OBJ_INDEX_DATA * idx, char page);
void send_room_page_to_char(CHAR_DATA * ch, ROOM_INDEX_DATA * idx, char page);
void send_page_to_char(CHAR_DATA * ch, MOB_INDEX_DATA * idx, char page);
void send_control_page_to_char(CHAR_DATA * ch, char page);

/*
 * Local functions.
 */
void	talk_channel	args( ( CHAR_DATA *ch, char *argument,
			    int channel, const char *verb ) );

char	*check_social_chan	args( ( CHAR_DATA *ch, char *command,
			        char *argument ) );

char *  scramble        args( ( const char *argument, int modifier ) );			    
char *  drunk_speech    args( ( const char *argument, CHAR_DATA *ch ) ); 

/*
 *  Profanity handler stuff  (forgive me for what i must do)
 */
void add_profane_word( char * word);
int is_profane (char *what);
char *bigregex = NULL;
char * preg;



/* Text scrambler -- Altrag */
char *scramble( const char *argument, int modifier )
{
    static char arg[MAX_INPUT_LENGTH];
    sh_int position;
    sh_int conversion = 0;
    
    modifier %= number_range( 80, 300 ); /* Bitvectors get way too large #s */
    for ( position = 0; position < MAX_INPUT_LENGTH; position++ )
    {
    	if ( argument[position] == '\0' )
    	{
    		arg[position] = '\0';
    		return arg;
    	}
    	else if ( argument[position] >= 'A' && argument[position] <= 'Z' )
	    {
	    	conversion = -conversion + position - modifier + argument[position] - 'A';
	    	conversion = number_range( conversion - 5, conversion + 5 );
	    	while ( conversion > 25 )
	    		conversion -= 26;
	    	while ( conversion < 0 )
	    		conversion += 26;
	    	arg[position] = conversion + 'A';
	    }
	    else if ( argument[position] >= 'a' && argument[position] <= 'z' )
	    {
	    	conversion = -conversion + position - modifier + argument[position] - 'a';
	    	conversion = number_range( conversion - 5, conversion + 5 );
	    	while ( conversion > 25 )
	    		conversion -= 26;
	    	while ( conversion < 0 )
	    		conversion += 26;
	    	arg[position] = conversion + 'a';
	    }
	    else if ( argument[position] >= '0' && argument[position] <= '9' )
	    {
	    	conversion = -conversion + position - modifier + argument[position] - '0';
	    	conversion = number_range( conversion - 2, conversion + 2 );
	    	while ( conversion > 9 )
	    		conversion -= 10;
	    	while ( conversion < 0 )
	    		conversion += 10;
	    	arg[position] = conversion + '0';
	    }
	    else
	    	arg[position] = argument[position];
	}
	arg[position] = '\0';
	return arg;	     
}

/* I'll rewrite this later if its still needed.. -- Altrag
char *translate( CHAR_DATA *ch, CHAR_DATA *victim, const char *argument )
{
	return "";
}
*/

LANG_DATA *get_lang(const char *name)
{
    LANG_DATA *lng;

    for (lng = first_lang; lng; lng = lng->next)
	if (!str_cmp(lng->name, name))
	    return lng;
    return NULL;
}

/* percent = percent knowing the language. */
char *translate(int percent, const char *in, const char *name)
{
    LCNV_DATA *cnv;
    static char buf[256];
    char buf2[256];
    const char *pbuf;
    char *pbuf2 = buf2;
    LANG_DATA *lng;

    if ( percent > 99 || !str_cmp(name, "common") )
	return (char *) in;

    /* If we don't know this language... use "default" */
    if ( !(lng=get_lang(name)) )
	if ( !(lng = get_lang("default")) )
	    return (char *) in;

    for (pbuf = in; *pbuf;)
    {
	for (cnv = lng->first_precnv; cnv; cnv = cnv->next)
	{
	    if (!str_prefix(cnv->old, pbuf))
	    {
		if ( percent && (rand() % 100) < percent )
		{
		    strncpy(pbuf2, pbuf, cnv->olen);
		    pbuf2[cnv->olen] = '\0';
		    pbuf2 += cnv->olen;
		}
		else
		{
		    strcpy(pbuf2, cnv->new);
		    pbuf2 += cnv->nlen;
		}
		pbuf += cnv->olen;
		break;
	    }
	}
	if (!cnv)
	{
	    if (isalpha(*pbuf) && (!percent || (rand() % 100) > percent) )
	    {
		*pbuf2 = lng->alphabet[LOWER(*pbuf) - 'a'];
		if ( isupper(*pbuf) )
		    *pbuf2 = UPPER(*pbuf2);
	    }
	    else
		*pbuf2 = *pbuf;
	    pbuf++;
	    pbuf2++;
	}
    }
    *pbuf2 = '\0';
    for (pbuf = buf2, pbuf2 = buf; *pbuf;)
    {
	for (cnv = lng->first_cnv; cnv; cnv = cnv->next)
	    if (!str_prefix(cnv->old, pbuf))
	    {
		strcpy(pbuf2, cnv->new);
		pbuf += cnv->olen;
		pbuf2 += cnv->nlen;
		break;
	    }
	if (!cnv)
	    *(pbuf2++) = *(pbuf++);
    }
    *pbuf2 = '\0';
#if 0
    for (pbuf = in, pbuf2 = buf; *pbuf && *pbuf2; pbuf++, pbuf2++)
	if (isupper(*pbuf))
	    *pbuf2 = UPPER(*pbuf2);
    /* Attempt to align spacing.. */
	else if (isspace(*pbuf))
	    while (*pbuf2 && !isspace(*pbuf2))
		pbuf2++;
#endif
    return buf;
}


char *drunk_speech( const char *argument, CHAR_DATA *ch )
{
  const char *arg = argument;
  static char buf[MAX_INPUT_LENGTH*2];
  char buf1[MAX_INPUT_LENGTH*2];
  sh_int drunk;
  char *txt;
  char *txt1;  

  if ( IS_NPC( ch ) || !ch->pcdata ) return (char *) argument;

  drunk = ch->pcdata->condition[COND_DRUNK];

  if ( drunk <= 0 )
    return (char *) argument;

  buf[0] = '\0';
  buf1[0] = '\0';

  if ( !argument )
  {
     bug( "Drunk_speech: NULL argument", 0 );
     return "";
  }

  /*
  if ( *arg == '\0' )
    return (char *) argument;
  */

  txt = buf;
  txt1 = buf1;

  while ( *arg != '\0' )
  {
    if ( toupper(*arg) == 'T' )
    {
	if ( number_percent() < ( drunk * 2 ) )		/* add 'h' after an 'T' */
	{
	   *txt++ = *arg;
	   *txt++ = 'h';
	}
       else
	*txt++ = *arg;
    }
   else if ( toupper(*arg) == 'X' )
    {
	if ( number_percent() < ( drunk * 2 / 2 ) )
	{
	  *txt++ = 'c', *txt++ = 's', *txt++ = 'h';
	}
       else
	*txt++ = *arg;
    }
   else if ( number_percent() < ( drunk * 2 / 5 ) )  /* slurred letters */
    {
      sh_int slurn = number_range( 1, 2 );
      sh_int currslur = 0;	

      while ( currslur < slurn )
	*txt++ = *arg, currslur++;
    }
   else
    *txt++ = *arg;

    arg++;
  };

  *txt = '\0';

  txt = buf;

  while ( *txt != '\0' )   /* Let's mess with the string's caps */
  {
    if ( number_percent() < ( 2 * drunk / 2.5 ) )
    {
      if ( isupper(*txt) )
        *txt1 = tolower( *txt );
      else
      if ( islower(*txt) )
        *txt1 = toupper( *txt );
      else
        *txt1 = *txt;
    }
    else
      *txt1 = *txt;

    txt1++, txt++;
  };

  *txt1 = '\0';
  txt1 = buf1;
  txt = buf;

  while ( *txt1 != '\0' )   /* Let's make them stutter */
  {
    if ( *txt1 == ' ' )  /* If there's a space, then there's gotta be a */
    {			 /* along there somewhere soon */

      while ( *txt1 == ' ' )  /* Don't stutter on spaces */
        *txt++ = *txt1++;

      if ( ( number_percent() < ( 2 * drunk / 4 ) ) && *txt1 != '\0' )
      {
	sh_int offset = number_range( 0, 2 );
	sh_int pos = 0;

	while ( *txt1 != '\0' && pos < offset )
	  *txt++ = *txt1++, pos++;

	if ( *txt1 == ' ' )  /* Make sure not to stutter a space after */
	{		     /* the initial offset into the word */
	  *txt++ = *txt1++;
	  continue;
	}

	pos = 0;
	offset = number_range( 2, 4 );	
	while (	*txt1 != '\0' && pos < offset )
	{
	  *txt++ = *txt1;
	  pos++;
	  if ( *txt1 == ' ' || pos == offset )  /* Make sure we don't stick */ 
	  {		               /* A hyphen right before a space	*/
	    txt1--;
	    break;
	  }
	  *txt++ = '-';
	}
	if ( *txt1 != '\0' )
	  txt1++;
      }     
    }
   else
    *txt++ = *txt1++;
  }

  *txt = '\0';

  return buf;
}

/*
 * Generic channel function.
 */
void talk_channel( CHAR_DATA *ch, char *argument, int channel, const char *verb )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    char *txtsoc = "\0";
    char soc[MAX_INPUT_LENGTH];
    int position;
#ifndef SCRAMBLE
    int speaking = -1, lang;

    for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
	if ( ch->speaking & lang_array[lang] )
	{
	    speaking = lang;
	    break;
	}
#endif

    if ( IS_NPC( ch ) && channel == CHANNEL_CLAN )
    {
	send_to_char( "Moby nie moga nalezec do klanow.\n\r", ch );
	return;
    }
    if ( IS_NPC( ch ) && channel == CHANNEL_ORDER )
    {
	send_to_char( "Moby nie moga byc w rzadach.\n\r", ch );
	return;
    }

    if ( IS_NPC( ch ) && channel == CHANNEL_COUNCIL )
    {
	send_to_char( "Moby nie moga nalezec do rad.\n\r", ch);
	return;
    }

    if ( IS_NPC( ch ) && channel == CHANNEL_GUILD )
    {
        send_to_char( "Moby nie moga nalezec do gildii.\n\r", ch );
	return;
    }

    if ( !IS_PKILL( ch ) && channel == CHANNEL_WARTALK )
    {
	if ( !IS_IMMORTAL( ch ) )
	{
	  send_to_char( "Pokojowi gracze nie musza uzywac wartalka.\n\r", ch );
	  return;
	}
    }
                                          
    if ( IS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
    {
	send_to_char( "W tym pokoju nalezy zachowac cisze.\n\r", ch );
	return;
    }
    
    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
    {
      if ( ch->master )
	send_to_char( "Nie sadze...\n\r", ch->master );
      return;
    }

    if ( argument[0] == '\0' )
    {
	sprintf( buf, "%s co?\n\r", verb );
	buf[0] = UPPER(buf[0]);
	send_to_char( buf, ch );	/* where'd this line go? */
	return;
    }

    if ( !IS_NPC(ch) && xIS_SET(ch->act, PLR_SILENCE) )
    {
	ch_printf( ch, "%s... Hmm nie mozesz tego uczynic.\n\r", verb );
	return;
    }

    REMOVE_BIT(ch->deaf, channel);

    switch ( channel )
    {
    default:
/* Emote i sociale na kanale Flash 24.04.1999 */
    if ( argument[0] == ':') 
    {
        argument[0] = ' ';
	set_char_color( AT_GOSSIP, ch );
	ch_printf( ch, "[%s] %s%s\n\r", verb, ch->name, argument );
	sprintf( buf, "[%s] $n$t",     verb ); 
    }
    else if ( argument[0] == '&' ) 
    {
	    ++argument;
	    argument = one_argument( argument, soc );
	    txtsoc = check_social_chan( ch, soc, argument ) ; 
	    
	    if ( txtsoc !=  FALSE ) 
	    {
		set_char_color( AT_GOSSIP, ch );
		ch_printf( ch, "[%s] %s\n\r", verb, txtsoc );
    		sprintf( buf, "[%s] %s",     verb, txtsoc ); 
	    }
	    else
	    {
	    sprintf( buf , " " );
	    }
    }
/* Emote i sociale na kanale Flash koniec */    
    else 
    {
	set_char_color( AT_GOSSIP, ch );
	ch_printf( ch, "[%s] %s: %s\n\r", verb, ch->name ,argument );
	sprintf( buf, "[%s] $n: $t",     verb ); 
    }
    break;
    case CHANNEL_RACETALK:
        set_char_color( AT_RACETALK, ch );
	ch_printf( ch, "[%s] %s: %s\n\r", verb, ch->name, argument );
	sprintf( buf, "[%s] $n: $t",     verb );
        break;
    case CHANNEL_WARTALK:
        set_char_color( AT_WARTALK, ch );
        ch_printf( ch, "[%s] %s: %s\n\r", verb, ch->name, argument );
        sprintf( buf, "[%s] $n: $t", verb );
        break;
    case CHANNEL_IMMTALK:
    case CHANNEL_AVTALK:
	sprintf( buf, "$n%c $t", channel == CHANNEL_IMMTALK ? '>' : ':' );
	position	= ch->position;
	ch->position	= POS_STANDING;
    act( AT_IMMORT, buf, ch, argument, NULL, TO_CHAR );
	ch->position	= position;
	break;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
    {
	sprintf( buf2, "%s: %s (%s)", IS_NPC( ch ) ? ch->short_descr : ch->name,
		 argument, verb );
	append_to_file( LOG_FILE, buf2 );
    }


#ifdef HMM
    if ( is_profane(argument) )
    {
	sprintf( buf2, "%s Profanity warning: %s: %s (%s)", 
		        "say",
                        IS_NPC( ch ) ? ch->short_descr : ch->name,
		        argument, 
                        verb );
        /* force Puff mpat 6 mpforce imp mpat 1 say hi */

        puff = get_char_world( ch, "Puff" );
        if(puff!=NULL)  
        { 
          if ( ( location = get_room_index( 1 ) ) != NULL ){
              original = puff->in_room;
              char_from_room( puff );
              char_to_room( puff, location );
              interpret( puff, buf2 );
              char_to_room( puff, original );
          }
        } 
    }
#endif

    for ( d = first_descriptor; d; d = d->next )
    {
	CHAR_DATA *och;
	CHAR_DATA *vch;

	och = d->original ? d->original : d->character;
	vch = d->character;

	if ( d->connected == CON_PLAYING
	&&   vch != ch
	&&  !IS_SET(och->deaf, channel) )
	{
		char *sbuf = argument;
  	    	char lbuf[MAX_INPUT_LENGTH + 4]; /* invis level string + buf */
  	    	
	    if ( channel != CHANNEL_NEWBIE && NOT_AUTHED(och) )
		continue;		
	    if ( channel == CHANNEL_IMMTALK && !IS_IMMORTAL(och) )
		continue;
            if ( channel == CHANNEL_WARTALK && NOT_AUTHED( och ) )
                continue;
	    if ( channel == CHANNEL_AVTALK && !IS_HERO(och) )
		continue;
	    if ( channel == CHANNEL_HIGHGOD && get_trust( och ) < sysdata.muse_level )
		continue;
	    if ( channel == CHANNEL_HIGH    && get_trust( och ) < sysdata.think_level )
		continue;

            /* Fix by Narn to let newbie council members see the newbie channel. */
	    if ( channel == CHANNEL_NEWBIE  && 
                  ( !IS_IMMORTAL(och) && !NOT_AUTHED(och) 
                  && !( och->pcdata->council && 
                     !str_cmp( och->pcdata->council->name, "Newbie Council" ) ) ) )
		continue;
	    if ( IS_SET( vch->in_room->room_flags, ROOM_SILENCE ) )
	    	continue;
	    if ( channel == CHANNEL_YELL
	    &&   vch->in_room->area != ch->in_room->area )
		continue;

	    if ( channel == CHANNEL_CLAN || channel == CHANNEL_ORDER
	    ||   channel == CHANNEL_GUILD )
	    {
		if ( IS_NPC( vch ) )
		  continue;
		if ( vch->pcdata->clan != ch->pcdata->clan )
	    	  continue;
	    }

	    if ( channel == CHANNEL_COUNCIL )
	    {
		if ( IS_NPC( vch ) )
		  continue;
		if ( vch->pcdata->council != ch->pcdata->council )
	    	  continue;
	    }


	    if ( channel == CHANNEL_RACETALK )
	      if ( vch->race != ch->race )
		continue;

	    if ( xIS_SET(ch->act, PLR_WIZINVIS) &&
	    	can_see(vch, ch) && IS_IMMORTAL(vch))
	    {
	    	sprintf(lbuf, "(%d) ", (!IS_NPC(ch))?ch->pcdata->wizinvis
			:ch->mobinvis);
	    }
	    else
	    {
	    	lbuf[0] = '\0';
	    }
	    
	    position		= vch->position;
	    if ( channel != CHANNEL_SHOUT && channel != CHANNEL_YELL )
		vch->position	= POS_STANDING;
#ifndef SCRAMBLE
	    if ( speaking != -1 && (!IS_NPC(ch) || ch->speaking) )
	    {
		int speakswell = UMIN(knows_language(vch, ch->speaking, ch),
				      knows_language(ch, ch->speaking, vch));

		if ( speakswell < 85 )
		    sbuf = translate(speakswell, argument, lang_names[speaking]);
	    }
#else
	    if ( !knows_language(vch, ch->speaking, ch)
	    &&  (!IS_NPC(ch) || ch->speaking != 0) )
		sbuf = scramble(argument, ch->speaking);
#endif
	   /*  Scramble speech if vch or ch has nuisance flag */

	    if ( !IS_NPC(ch) && ch->pcdata->nuisance
	    &&   ch->pcdata->nuisance->flags > 7 
	    &&	(number_percent()<((ch->pcdata->nuisance->flags-7)*10*
		ch->pcdata->nuisance->power)))
		sbuf = scramble(argument,number_range(1,10));

	    if ( !IS_NPC(vch) && vch->pcdata->nuisance && 
		vch->pcdata->nuisance->flags > 7 
		 &&(number_percent()<((vch->pcdata->nuisance->flags-7)*10*
		 vch->pcdata->nuisance->power)))
			sbuf = scramble(argument, number_range(1,10));

	    MOBtrigger = FALSE;
	    if ( channel == CHANNEL_IMMTALK || channel == CHANNEL_AVTALK )
		act( AT_IMMORT, strcat(lbuf,buf), ch, sbuf, vch, TO_VICT );
            else if (channel == CHANNEL_WARTALK)
        	act( AT_WARTALK, strcat(lbuf,buf), ch, sbuf, vch, TO_VICT );
	    else if (channel == CHANNEL_RACETALK)
		act( AT_RACETALK, strcat(lbuf,buf), ch, sbuf, vch, TO_VICT );
	    else
		act( AT_GOSSIP, strcat(lbuf,buf), ch, sbuf, vch, TO_VICT );
	    vch->position	= position;
	}
    }

    /* too much system degradation with 300+ players not to charge 'em a bit */
    /* 600 players now, but waitstate on clantalk is bad for pkillers */
    if ( ( ch->level<51 )
    && ( channel != CHANNEL_WARTALK )
    && ( +-channel != CHANNEL_CLAN ) )
       WAIT_STATE( ch, 6 );

    return;
}

void to_channel( const char *argument, int channel, const char *verb, sh_int level )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if ( !first_descriptor || argument[0] == '\0' )
      return;

    sprintf(buf, "%s: %s\r\n", verb, argument );

    for ( d = first_descriptor; d; d = d->next )
    {
	CHAR_DATA *och;
	CHAR_DATA *vch;

	och = d->original ? d->original : d->character;
	vch = d->character;

	if ( !och || !vch )
	  continue;
	if ( !IS_IMMORTAL(vch)
	|| ( get_trust(vch) < sysdata.build_level && channel == CHANNEL_BUILD )
	|| ( get_trust(vch) < sysdata.log_level
	&& ( channel == CHANNEL_LOG || channel == CHANNEL_HIGH || 
	channel == CHANNEL_WARN ||channel == CHANNEL_COMM) ) )
	  continue;

	if ( d->connected == CON_PLAYING
	&&  !IS_SET(och->deaf, channel)
	&&   get_trust( vch ) >= level )
	{
	  set_char_color( AT_LOG, vch );
	  send_to_char_color( buf, vch );
	}
    }

    return;
}


void do_chat( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Ze co?\n\r", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_CHAT, "chat" );
    return;
}

void do_clantalk( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Nie mozesz uzywac kanalu CLANTALK?\n\r", ch);
      return;
    }

    if ( IS_NPC( ch ) || !ch->pcdata->clan 
    ||   ch->pcdata->clan->clan_type == CLAN_ORDER
    ||   ch->pcdata->clan->clan_type == CLAN_GUILD )
    {
	send_to_char( "Nie mozesz uzywac kanalu CLANTALK?\n\r", ch );
	return;
    }
    talk_channel( ch, argument, CHANNEL_CLAN, "clantalk" );
    return;
}

/* void do_newbiechat( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch )
       || ( (ch->level > 5) && !IS_IMMORTAL(ch) 
       && !( ch->pcdata->council && 
          !str_cmp( ch->pcdata->council->name, "Newbie Council" ) ) ) )
    {
        send_to_char( "Ze co?\n\r", ch );
        return;
    }
    talk_channel( ch, argument, CHANNEL_NEWBIE, "newbiechat" );
    return;
}
Flash */
void do_newbiechat( CHAR_DATA *ch, char *argument )
{  
    if ( IS_NPC( ch )
    || ( !NEW_AUTH( ch ) && !IS_IMMORTAL(ch) /* new auth */
    && !( ch->pcdata->council && !str_cmp( ch->pcdata->council->name, "Newbie Council" ) ) ) )
    {
        send_to_char( "Ze co?\n\r", ch );
        return;
    }
    talk_channel( ch, argument, CHANNEL_NEWBIE, "newbiechat" );
    return;
}

void do_ot( CHAR_DATA *ch, char *argument )
{
  do_ordertalk( ch, argument );
}

void do_ordertalk( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Ze co?\n\r", ch);
      return;
    }

    if ( IS_NPC( ch ) || !ch->pcdata->clan 
         || ch->pcdata->clan->clan_type != CLAN_ORDER )
    {
	send_to_char( "Ze co?\n\r", ch );
	return;
    }
    talk_channel( ch, argument, CHANNEL_ORDER, "ordertalk" );
    return;
}

void do_counciltalk( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Ze co?\n\r", ch);
      return;
    }

    if ( IS_NPC( ch ) || !ch->pcdata->council )
    {
	send_to_char( "Ze co?\n\r", ch );
	return;
    }
    talk_channel( ch, argument, CHANNEL_COUNCIL, "counciltalk" );
    return;
}

void do_guildtalk( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Ze co?\n\r", ch);
      return;
    }

    if ( IS_NPC( ch ) || !ch->pcdata->clan || ch->pcdata->clan->clan_type != CLAN_GUILD )
    {
	send_to_char( "Ze co?\n\r", ch );
	return;
    }
    talk_channel( ch, argument, CHANNEL_GUILD, "guildtalk" );
    return;
}

void do_music( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Ze co?\n\r", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_MUSIC, "music" );
    return;
}


void do_quest( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Ze co?\n\r", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_QUEST, "quest" );
    return;
}

void do_ask( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Ze co?\n\r", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_ASK, "ask" );
    return;
}



void do_answer( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Ze co?\n\r", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_ASK, "answer" );
    return;
}



void do_shout( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Ze co?\n\r", ch);
      return;
    }
  talk_channel( ch, drunk_speech( argument, ch ), CHANNEL_SHOUT, "shout" );
  WAIT_STATE( ch, 12 );
  return;
}



void do_yell( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Ze co?\n\r", ch);
      return;
    }
  talk_channel( ch, drunk_speech( argument, ch ), CHANNEL_YELL, "yell" );
  return;
}



void do_immtalk( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Ze co?\n\r", ch);
      return;
    }

    talk_channel( ch, argument, CHANNEL_IMMTALK, "immtalk" );
    return;
}


void do_muse( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Ze co?\n\r", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_HIGHGOD, "muse" );
    return;
}


void do_think( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Ze co?\n\r", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_HIGH, "think" );
    return;
}


void do_avtalk( CHAR_DATA *ch, char *argument )
{
    if (NOT_AUTHED(ch))
    {
      send_to_char("Ze co?\n\r", ch);
      return;
    }

    talk_channel( ch, drunk_speech( argument, ch ), CHANNEL_AVTALK, "avtalk" );
    return;
}


void do_say( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *vch;
    EXT_BV actflags;
#ifndef SCRAMBLE
    int speaking = -1, lang;

    for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
	if ( ch->speaking & lang_array[lang] )
	{
	    speaking = lang;
	    break;
	}
#endif

    if ( argument[0] == '\0' )
    {
	send_to_char( "Co powiedziec?\n\r", ch );
	return;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
    {
	send_to_char( "W tym pokoju nalezy zachowac cisze.\n\r", ch );
	return;
    }

    actflags = ch->act;
    if ( IS_NPC(ch) )
	xREMOVE_BIT( ch->act, ACT_SECRETIVE );
    for ( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
    {
	char *sbuf = argument;

	if ( vch == ch )
		continue;
			
	/* Check to see if character is ignoring speaker */
	if (is_ignoring(vch, ch))
	{
		/* continue unless speaker is an immortal */
		if(!IS_IMMORTAL(ch) || get_trust(vch) > get_trust(ch))
			continue;
		else
		{
			set_char_color(AT_IGNORE, vch);
			ch_printf(vch,"Starasz sie ignorowac %s, ale"
				" nie udaje Ci sie.\n\r", ch->nbie);
		}
	}
				
#ifndef SCRAMBLE
	if ( speaking != -1 && (!IS_NPC(ch) || ch->speaking) )
	{
	    int speakswell = UMIN(knows_language(vch, ch->speaking, ch),
				      knows_language(ch, ch->speaking, vch));

	    if ( speakswell < 75 )
		sbuf = translate(speakswell, argument, lang_names[speaking]);
	}
#else
	if ( !knows_language(vch, ch->speaking, ch)
	&&  (!IS_NPC(ch) || ch->speaking != 0) )
		sbuf = scramble(argument, ch->speaking);
#endif
	sbuf = drunk_speech( sbuf, ch );

	MOBtrigger = FALSE;
	act( AT_SAY, "$n mowi '$t'", ch, sbuf, vch, TO_VICT );
    }
/*    MOBtrigger = FALSE;
    act( AT_SAY, "$n says '$T'", ch, NULL, argument, TO_ROOM );*/
    ch->act = actflags;
    MOBtrigger = FALSE;
    act( AT_SAY, "Mowisz '$T'", ch, NULL, drunk_speech( argument, ch ), TO_CHAR ); 
    if ( IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
    {
	sprintf( buf, "%s: %s", IS_NPC( ch ) ? ch->short_descr : ch->name,
		 argument );
	append_to_file( LOG_FILE, buf );
    }
    mprog_speech_trigger( argument, ch );
    if ( char_died(ch) )
	return;
    oprog_speech_trigger( argument, ch ); 
    if ( char_died(ch) )
	return;
    rprog_speech_trigger( argument, ch ); 
    return;
}


void do_whisper( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int position;
    int speaking = -1, lang;
#ifndef SCRAMBLE

    for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
	if ( ch->speaking & lang_array[lang] )
	{
	    speaking = lang;
	    break;
	}
#endif

    REMOVE_BIT( ch->deaf, CHANNEL_WHISPER );

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
	send_to_char( "Co szepnac i komu?\n\r", ch );
	return;
    }


    if ( (victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "Nie ma tu kogos takiego.\n\r", ch );
	return;
    }

    if ( ch == victim )
    {
	send_to_char( "Ucinasz sobie pogawedke z wlasna osoba.\n\r", ch );
	return;
    }

    if ( !IS_NPC( victim ) && ( victim->switched ) 
	&& !IS_AFFECTED(victim->switched, AFF_POSSESS) )
    {
      send_to_char( "Ten gracz jest aktualnie przelaczony.\n\r", ch );
      return;
    }
   else if ( !IS_NPC( victim ) && ( !victim->desc ) )
    {
      send_to_char( "Ten gracz stracil polaczenie.\n\r", ch );
      return;
    }
    if ( !IS_NPC(victim) && xIS_SET(victim->act, PLR_AFK) )
      {
      send_to_char( "Ten gracz jest zajety (AFK).\n\r", ch );
      return;
      }
    if ( IS_SET( victim->deaf, CHANNEL_WHISPER ) 
    && ( !IS_IMMORTAL( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) )
    {
      act( AT_PLAIN, "Ten gracz ma wylaczone szeptanie.", ch, NULL, victim,
		TO_CHAR );
      return;
    }
    if ( !IS_NPC(victim) &&  xIS_SET(victim->act, PLR_SILENCE) )
	send_to_char( "Ten gracz jest uciszony.  Uslyszy Twoja wiadomosc lecz nie moze odpowiedziec.\n\r", ch );

    if ( victim->desc		/* make sure desc exists first  -Thoric */
    &&   victim->desc->connected == CON_EDITING 
    &&   get_trust(ch) < LEVEL_GOD )
    {
	act( AT_PLAIN, "Ten gracz jest w edytorze.  Ponow probe za chwilke.", ch, 0, victim, TO_CHAR );
        return;
    }

    /* Check to see if target of tell is ignoring the sender */
    if(is_ignoring(victim, ch))	
    {
	/* If the sender is an imm then they cannot be ignored */
	if(!IS_IMMORTAL(ch) || get_trust(victim) > get_trust(ch))
	{
		set_char_color(AT_IGNORE, ch);
		ch_printf(ch,"%s ignoruje Cie.\n\r",
			victim->name);
		return;
	}
	else
	{
		set_char_color(AT_IGNORE, victim);
		ch_printf(victim, "Starasz sie ignorowac %s, lecz "
			"nie udaje Ci sie.\n\r", ch->nbie);
	}
    }

    act( AT_WHISPER, "Szepczesz do $C '$t'", ch, argument, victim, TO_CHAR );
    position		= victim->position;
    victim->position	= POS_STANDING;
#ifndef SCRAMBLE
    if ( speaking != -1 && (!IS_NPC(ch) || ch->speaking) )
    {
	int speakswell = UMIN(knows_language(victim, ch->speaking, ch),
			      knows_language(ch, ch->speaking, victim));

	if ( speakswell < 85 )
	    act( AT_WHISPER, "$n szepcze Ci '$t'", ch,
		translate(speakswell, argument, lang_names[speaking]), victim, TO_VICT );
#else
    if ( !knows_language(vch, ch->speaking, ch ) &&
       ( !IS_NPC(ch) || ch->speaking != 0) )
            act( AT_WHISPER, "$n szepcze Ci '$t'", ch,
	    	translate(speakswell, argument, lang_names[speaking]), victim, TO_VICT );
#endif
	else
	    act( AT_WHISPER, "$n szepcze Ci '$t'", ch, argument, victim, TO_VICT );
    }
    else
	act( AT_WHISPER, "$n szepcze Ci '$t'", ch, argument, victim, TO_VICT );

    if ( !IS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
	act( AT_WHISPER, "$n szepcze cos do ucha $X.", ch, argument, victim, TO_NOTVICT );

    victim->position	= position;
    if ( IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
    {
	sprintf( buf, "%s: %s (whisper to) %s.",
		 IS_NPC( ch ) ? ch->short_descr : ch->name,
		 argument,
		 IS_NPC( victim ) ? victim->short_descr : victim->name );
	append_to_file( LOG_FILE, buf );
    }
    
    mprog_speech_trigger( argument, ch );
    return;
}

void do_tell( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int position;
    CHAR_DATA *switched_victim = NULL;
#ifndef SCRAMBLE
    int speaking = -1, lang;

    for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
	if ( ch->speaking & lang_array[lang] )
	{
	    speaking = lang;
	    break;
	}
#endif

    REMOVE_BIT( ch->deaf, CHANNEL_TELLS );
    if ( IS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
    {
	send_to_char( "W tym pokoju nalezy zachowac cisze.\n\r", ch );
	return;
    }

    if (!IS_NPC(ch)
    && ( xIS_SET(ch->act, PLR_SILENCE)
    ||   xIS_SET(ch->act, PLR_NO_TELL) ) )
    {
	send_to_char( "Nie mozesz tego uczynic.\n\r", ch );
	return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
	send_to_char( "Co powiedziec i komu?\n\r", ch );
	return;
    }

/*    if ( ( victim = get_char_world( ch, arg ) ) == NULL 
       || ( IS_NPC(victim) && victim->in_room != ch->in_room ) 
       || (!NOT_AUTHED(ch) && NOT_AUTHED(victim) && !IS_IMMORTAL(ch) ) ) */
	 if ( ( victim = get_char_world( ch, arg ) ) == NULL 
       || ( IS_NPC(victim) && victim->in_room != ch->in_room ) 
       || (!NEW_AUTH(ch) && NEW_AUTH(victim) && !IS_IMMORTAL(ch) ) ) /* new auth */
    {
	send_to_char( "Nie ma tu kogos takiego.\n\r", ch );
	return;
    }

    if ( ch == victim )
    {
	send_to_char( "Ucinasz sobie rozmowe z wlasna osoba.\n\r", ch );
	return;
    }

    if (NEW_AUTH(ch) && !NEW_AUTH(victim) && !IS_IMMORTAL(victim) )
    {
	send_to_char( "Ta osoba nie moze cie uslyszec gdyz nie zostala jeszcze zautoryzowana.\n\r", ch);
	return;
    }

    if ( !IS_NPC( victim ) && ( victim->switched ) 
	&& ( get_trust( ch ) > LEVEL_AVATAR ) 
	&& !IS_AFFECTED(victim->switched, AFF_POSSESS) )
    {
      send_to_char( "Ten gracz jest aktualnie przelaczony.\n\r", ch );
      return;
    }

   else if ( !IS_NPC( victim ) && ( victim->switched ) 
 	&&  IS_AFFECTED(victim->switched, AFF_POSSESS) ) 
     switched_victim = victim->switched;

   else if ( !IS_NPC( victim ) && ( !victim->desc ) )
    {
      send_to_char( "Ten gracz stracil polaczenie.\n\r", ch );
      return;
    }

    if ( !IS_NPC(victim) && xIS_SET(victim->act, PLR_AFK) )
    {
	send_to_char( "Ten gracz jest zajety (AFK).\n\r", ch );
	return;
    }

    if ( IS_SET( victim->deaf, CHANNEL_TELLS ) 
    && ( !IS_IMMORTAL( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) )
    {
      act( AT_PLAIN, "Ten gracz nie slucha Cie.", ch, NULL, victim,
		TO_CHAR );
      return;
    }

    if ( !IS_NPC(victim) && xIS_SET(victim->act, PLR_SILENCE) )
	send_to_char( "Ten gracz jest uciszony.  Otrzyma Twoj komunikat ale ni bedzie mogl odpowiedziec.\n\r", ch );

    if ( (!IS_IMMORTAL(ch) && !IS_AWAKE(victim) )
    || (!IS_NPC(victim)&&IS_SET(victim->in_room->room_flags, ROOM_SILENCE ) ) )
    {
    act( AT_PLAIN, "Ten gracz nie moze Cie uslyszec.", ch, 0, victim, TO_CHAR );
	return;
    }

    if ( victim->desc		/* make sure desc exists first  -Thoric */
    &&   victim->desc->connected == CON_EDITING 
    &&   get_trust(ch) < LEVEL_GOD )
    {
	act( AT_PLAIN, "Ten gracz jest w edytorze.  Sprobuj za chwilke.", ch, 0, victim, TO_CHAR );
        return;
    }

    /* Check to see if target of tell is ignoring the sender */
    if(is_ignoring(victim, ch))	
    {
	/* If the sender is an imm then they cannot be ignored */
	if(!IS_IMMORTAL(ch) || get_trust(victim) > get_trust(ch))
	{
		set_char_color(AT_IGNORE, ch);
		ch_printf(ch,"%s ignoruje Cie.\n\r",
			victim->name);
		return;
	}
	else
	{
		set_char_color(AT_IGNORE, victim);
		ch_printf(victim, "Starasz sie ignorowac %s, ale "
			"nie udaje Ci sie.\n\r", ch->nbie);
	}
    }

    ch->retell = victim;
    
    if(!IS_NPC(victim) && IS_IMMORTAL(victim) && victim->pcdata->tell_history &&
    	isalpha(IS_NPC(ch) ? ch->short_descr[0] : ch->name[0]))
    {
	sprintf(buf, "%s mowi Ci '%s'\n\r",
		capitalize(IS_NPC(ch) ? ch->short_descr : ch->name),
		argument);
	
	/* get lasttell index... assumes names begin with characters */
	victim->pcdata->lt_index =
		tolower(IS_NPC(ch) ? ch->short_descr[0] : ch->name[0]) - 'a';
	
	/* get rid of old messages */
	if(victim->pcdata->tell_history[victim->pcdata->lt_index])
		STRFREE(victim->pcdata->tell_history[victim->pcdata->lt_index]);
	
	/* store the new message */
	victim->pcdata->tell_history[victim->pcdata->lt_index] =
		STRALLOC(buf);
    }   	

    if(switched_victim)
      victim = switched_victim;
   
    act( AT_TELL, "Mowisz $X '$t'", ch, argument, victim, TO_CHAR );
    position		= victim->position;
    victim->position	= POS_STANDING;
    if ( speaking != -1 && (!IS_NPC(ch) || ch->speaking) )
    {
	int speakswell = UMIN(knows_language(victim, ch->speaking, ch),
			      knows_language(ch, ch->speaking, victim));

	if ( speakswell < 85 )
	    act( AT_TELL, "$n mowi Ci '$t'", ch, translate(speakswell, argument, lang_names[speaking]), victim, TO_VICT );
	else
	    act( AT_TELL, "$n mowi Ci '$t'", ch, argument, victim, TO_VICT );
    }
    else
	act( AT_TELL, "$n mowi Ci '$t'", ch, argument, victim, TO_VICT );

    victim->position	= position;
    victim->reply	= ch;
    if ( IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
    {
	sprintf( buf, "%s: %s (tell to) %s.",
		 IS_NPC( ch ) ? ch->short_descr : ch->name,
		 argument,
		 IS_NPC( victim ) ? victim->short_descr : victim->name );
	append_to_file( LOG_FILE, buf );
    }
    
    mprog_speech_trigger( argument, ch );
    return;
}



void do_reply( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int position;
#ifndef SCRAMBLE
    int speaking = -1, lang;

    for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
	if ( ch->speaking & lang_array[lang] )
	{
	    speaking = lang;
	    break;
	}
#endif


    REMOVE_BIT( ch->deaf, CHANNEL_TELLS );
    if ( IS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
    {
	send_to_char( "W tym pokoju nalezy zachowac cisze.\n\r", ch );
	return;
    }

    if ( !IS_NPC(ch) && xIS_SET(ch->act, PLR_SILENCE) )
    {
	send_to_char( "Twoj komunikat nie moze tam dotrzec.\n\r", ch );
	return;
    }

    if ( ( victim = ch->reply ) == NULL )
    {
	send_to_char( "Nie ma tu kogos takiego.\n\r", ch );
	return;
    }

    if ( !IS_NPC( victim ) && ( victim->switched )
	&& can_see( ch, victim ) && ( get_trust( ch ) > LEVEL_AVATAR ) )
    {
      send_to_char( "Ten gracz jest aktualnie przelaczony.\n\r", ch );
      return;
    }
   else if ( !IS_NPC( victim ) && ( !victim->desc ) )
    {
      send_to_char( "Ten gracz stracil polaczenie.\n\r", ch );
      return;
    }

    if ( !IS_NPC(victim) && xIS_SET(victim->act, PLR_AFK) )
    {
	send_to_char( "Ten gracz jest zajety (AFK).\n\r", ch );
	return;
    }

    if ( IS_SET( victim->deaf, CHANNEL_TELLS ) 
    && ( !IS_IMMORTAL( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) )
    {
      act( AT_PLAIN, "Ten gracz cie nie slucha.", ch, NULL, victim,
	TO_CHAR );
      return;
    }

    if ( ( !IS_IMMORTAL(ch) && !IS_AWAKE(victim) )
    || ( !IS_NPC(victim) && IS_SET( victim->in_room->room_flags, ROOM_SILENCE ) ) )
    {
    act( AT_PLAIN, "Ta osoba nie moze Cie uslyszec.", ch, 0, victim, TO_CHAR );
	return;
    }

    if ( victim->desc		/* make sure desc exists first  -Thoric */
    &&   victim->desc->connected == CON_EDITING 
    &&   get_trust(ch) < LEVEL_GOD )
    {
	act( AT_PLAIN, "Ten gracz jest w edytorze.  Sprobuj ponownie za chwilke.", ch, 0, victim, TO_CHAR );
        return;
    }

    /* Check to see if the receiver is ignoring the sender */
    if(is_ignoring(victim, ch))
    {
    	/* If the sender is an imm they cannot be ignored */
    	if(!IS_IMMORTAL(ch) || get_trust(victim) > get_trust(ch))
    	{
    		set_char_color(AT_IGNORE, ch);
    		ch_printf(ch,"%s ignoruje Cie.\n\r",
    			victim->name);
    		return;
    	}
    	else
    	{
    		set_char_color(AT_IGNORE, victim);
    		ch_printf(victim, "Starasz sie ignorowac %s, lecz "
    			"nie udaje Ci sie.\n\r", ch->nbie );
    	}
    }

    act( AT_TELL, "Mowisz $X '$t'", ch, argument, victim, TO_CHAR );
    position		= victim->position;
    victim->position	= POS_STANDING;
#ifndef SCRAMBLE
    if ( speaking != -1 && (!IS_NPC(ch) || ch->speaking) )
    {
	int speakswell = UMIN(knows_language(victim, ch->speaking, ch),
			      knows_language(ch, ch->speaking, victim));

	if ( speakswell < 85 )
	    act( AT_TELL, "$n mowi Ci '$t'", ch, translate(speakswell, argument, lang_names[speaking]), victim, TO_VICT );
	else
	    act( AT_TELL, "$n mowi Ci '$t'", ch, argument, victim, TO_VICT );
    }
    else
	act( AT_TELL, "$n mowi Ci '$t'", ch, argument, victim, TO_VICT );
#else
    if ( knows_language( victim, ch->speaking, ch ) ||
    	 (IS_NPC(ch) && !ch->speaking) )
	    act( AT_TELL, "$n mowi Ci '$t'", ch, argument, victim, TO_VICT );
	else
		act( AT_TELL, "$n mowi Ci '$t'", ch, scramble(argument, ch->speaking), victim, TO_VICT );
#endif
    victim->position	= position;
    victim->reply	= ch;
    ch->retell		= victim;
    if ( IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
    {
	sprintf( buf, "%s: %s (reply to) %s.",
		 IS_NPC( ch ) ? ch->short_descr : ch->name,
		 argument,
		 IS_NPC( victim ) ? victim->short_descr : victim->name );
	append_to_file( LOG_FILE, buf );
    }

    if(!IS_NPC(victim) && IS_IMMORTAL(victim) && victim->pcdata->tell_history &&
    	isalpha(IS_NPC(ch) ? ch->short_descr[0] : ch->name[0]))
    {
    	sprintf(buf, "%s mowi Ci '%s'\n\r",
    		capitalize(IS_NPC(ch) ? ch->short_descr : ch->name),
    		argument);
    	
    	/* get lasttell index... assumes names begin with characters */
    	victim->pcdata->lt_index =
    		tolower(IS_NPC(ch) ? ch->short_descr[0] : ch->name[0]) - 'a';
    	
    	/* get rid of old messages */
    	if(victim->pcdata->tell_history[victim->pcdata->lt_index])
    		STRFREE(victim->pcdata->tell_history[victim->pcdata->lt_index]);
    	
    	/* store the new message */
    	victim->pcdata->tell_history[victim->pcdata->lt_index] =
    		STRALLOC(buf);
    }

    return;
}

void do_retell(CHAR_DATA *ch, char *argument)
{
	char buf[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int position;
	CHAR_DATA *switched_victim = NULL;
#ifndef SCRAMBLE
	int speaking = -1, lang;

	for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
	    if ( ch->speaking & lang_array[lang] )
	    {
		speaking = lang;
		break;
	    }
#endif
	REMOVE_BIT(ch->deaf, CHANNEL_TELLS);
	if(IS_SET(ch->in_room->room_flags, ROOM_SILENCE))
	{
		send_to_char("W tym pokoju nalezy zachowac cisze.\n\r", ch);
		return;
	}
	
	if ( !IS_NPC(ch) && (xIS_SET(ch->act, PLR_SILENCE)
	||   xIS_SET(ch->act, PLR_NO_TELL)) )
	{
		send_to_char("Nie mozesz tego uczynic.\n\r", ch);
		return;
	}
	
	if(argument[0] == '\0')
	{
		ch_printf(ch, "Jaki komunikat chcesz wyslac?\n\r");
		return;
	}
	
	victim = ch->retell;
	
	if(!victim)
	{
		send_to_char("Nie ma tu kogos takiego.\n\r", ch);
		return;
	}
	
	if(!IS_NPC(victim) && (victim->switched) &&
		(get_trust(ch) > LEVEL_AVATAR) &&
		!IS_AFFECTED(victim->switched, AFF_POSSESS))
	{
		send_to_char("Ten gracz jest aktualnie przelaczony.\n\r", ch);
		return;
	}
	else if(!IS_NPC(victim) && (victim->switched) &&
		IS_AFFECTED(victim->switched, AFF_POSSESS))
	{
		switched_victim = victim->switched;
	}
	else if(!IS_NPC(victim) &&(!victim->desc))
	{
		send_to_char("Ten gracz stracil polaczenie.\n\r", ch);
		return;
	}
	
	if(!IS_NPC(victim) && xIS_SET(victim->act, PLR_AFK) )
	{
		send_to_char("Ten gracz jest zajety (AFK).\n\r", ch);
		return;
	}
	
	if(IS_SET(victim->deaf, CHANNEL_TELLS) &&
		(!IS_IMMORTAL(ch) || (get_trust(ch) < get_trust(victim))))
	{
		act(AT_PLAIN, "Ten gracz Cie nie slucha.", ch, NULL,
			victim, TO_CHAR);
		return;
	}
	
	if ( !IS_NPC(victim) && xIS_SET(victim->act, PLR_SILENCE) )
		send_to_char("Ten gracz jest uciszony. Otrzyma Twoj komunikat lecz nie bedzie mogl odpowiedziec.\n\r", ch);
	
	if((!IS_IMMORTAL(ch) && !IS_AWAKE(victim)) ||
		(!IS_NPC(victim) &&
		IS_SET(victim->in_room->room_flags, ROOM_SILENCE)))
	{
		act(AT_PLAIN, "Ta osoba nie moze Cie uslyszec.", ch, 0, victim, TO_CHAR);
		return;
	}
	
	if(victim->desc && victim->desc->connected == CON_EDITING &&
		get_trust(ch) < LEVEL_GOD)
	{
		act(AT_PLAIN, "Ten gracz jest w edytorze. Sprobuj "
			"ponownie za chwilke.", ch, 0, victim, TO_CHAR);
		return;
	}
	
	/* check to see if the target is ignoring the sender */
	if(is_ignoring(victim, ch))
	{
		/* if the sender is an imm then they cannot be ignored */
		if(!IS_IMMORTAL(ch) || get_trust(victim) > get_trust(ch))
		{
			set_char_color(AT_IGNORE, ch);
			ch_printf(ch, "%s ignoruje Cie.\n\r",
				victim->name);
			return;
		}
		else
		{
			set_char_color(AT_IGNORE,victim);
			ch_printf(victim, "Starasz sie ignorowac %s, lecz "
				"nie udaje Ci sie.\n\r", ch->nbie);
		}
	}

	/* store tell history for victim */
	if(!IS_NPC(victim) && IS_IMMORTAL(victim) && victim->pcdata->tell_history &&
		isalpha(IS_NPC(ch) ? ch->short_descr[0] : ch->name[0]))
	{
		sprintf(buf, "%s mowil Ci '%s'\n\r",
			capitalize(IS_NPC(ch) ? ch->short_descr : ch->name),
			argument);
		
		/* get lasttel index... assumes names begin with chars */
		victim->pcdata->lt_index =
			tolower(IS_NPC(ch) ? ch->short_descr[0] : ch->name[0])
			 - 'a';
		
		/* get rid of old messages */
		if(victim->pcdata->tell_history[victim->pcdata->lt_index])
			STRFREE(victim->pcdata->tell_history[victim->pcdata->lt_index]);
		
		/* store the new messagec */
		victim->pcdata->tell_history[victim->pcdata->lt_index] =
			STRALLOC(buf);
	}
	
	if(switched_victim)
		victim = switched_victim;
	
	act(AT_TELL, "Mowisz $X '$t'", ch, argument, victim, TO_CHAR);
	position = victim->position;
	victim->position = POS_STANDING;
#ifndef SCRAMBLE
	if ( speaking != -1 && (!IS_NPC(ch) || ch->speaking) )
	{
	    int speakswell = UMIN(knows_language(victim, ch->speaking, ch),
			      knows_language(ch, ch->speaking, victim));

	    if ( speakswell < 85 )
		act( AT_TELL, "$n mowi Ci '$t'", ch, translate(speakswell, argument, lang_names[speaking]), victim, TO_VICT );
	    else
		act( AT_TELL, "$n mowi Ci '$t'", ch, argument, victim, TO_VICT );
	}
	else
	    act( AT_TELL, "$n mowi Ci '$t'", ch, argument, victim, TO_VICT );
#else
	if(knows_language(victim, ch->speaking, ch) ||
		(IS_NPC(ch) && !ch->speaking))
	{
		act(AT_TELL, "$n mowi Ci '$t'", ch, argument, victim,
			TO_VICT);
	}
	else
	{
		act(AT_TELL, "$n mowi Ci '$t'", ch,
			scramble(argument, ch->speaking), victim, TO_VICT);
	}
#endif
	victim->position = position;
	victim->reply = ch;
	if(IS_SET(ch->in_room->room_flags, ROOM_LOGSPEECH))
	{
		sprintf(buf, "%s: %s (retell to) %s.",
			IS_NPC(ch) ? ch->short_descr : ch->name,
			argument,
			IS_NPC(victim) ? victim->short_descr : victim->name);
		append_to_file(LOG_FILE, buf);
	}
	
	mprog_speech_trigger(argument, ch);
	return;
}

void do_repeat(CHAR_DATA *ch, char *argument)
{
	int index;
	
	if(IS_NPC(ch) || !IS_IMMORTAL(ch) || !ch->pcdata->tell_history)
	{
		ch_printf(ch, "Ze co?\n\r");
		return;
	}
	
	if(argument[0] == '\0')
	{
		index = ch->pcdata->lt_index;
	}
	else if(isalpha(argument[0]) && argument[1] == '\0')
	{
		index = tolower(argument[0]) - 'a';
	}
	else
	{
		ch_printf(ch, "Podaj tylko pierwsza litere imienia "
			"osoby.\n\r");
		return;
	}
	
	if(ch->pcdata->tell_history[index])
	{
		set_char_color(AT_TELL, ch);
		ch_printf(ch, ch->pcdata->tell_history[index]);
	}
	else
	{
		ch_printf(ch, "Nikt taki nic do ciebie nie mowil.\n\r");
	}
	
	return;
}


void do_emote( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char *plast;
    CHAR_DATA *vch;
    EXT_BV actflags;
#ifndef SCRAMBLE
    int speaking = -1, lang;

    for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
	if ( ch->speaking & lang_array[lang] )
	{
	    speaking = lang;
	    break;
	}
#endif

    if ( !IS_NPC(ch) && xIS_SET(ch->act, PLR_NO_EMOTE) )
    {
	send_to_char( "Nie mozesz okazywac swych emocji.\n\r", ch );
	return;
    }

    if ( argument[0] == '\0' )
    {
	send_to_char( "Emote co?\n\r", ch );
	return;
    }

    actflags = ch->act;
    if ( IS_NPC(ch) )
	xREMOVE_BIT( ch->act, ACT_SECRETIVE );
    for ( plast = argument; *plast != '\0'; plast++ )
	;

    strcpy( buf, argument );
    if ( isalpha(plast[-1]) )
	strcat( buf, "." );
	for ( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
	{
		char *sbuf = buf;
		
		/* Check to see if character is ignoring emoter */
		if(is_ignoring(vch, ch))
		{
			/* continue unless emoter is an immortal */
			if(!IS_IMMORTAL(ch) || get_trust(vch) > get_trust(ch))
				continue;
			else
			{
				set_char_color(AT_IGNORE, vch);
				ch_printf(vch,"Starasz sie ignorowac %s, lecz"
					" nie udaje Ci sie.\n\r", ch->nbie);
			}
		}
#ifndef SCRAMBLE
		if ( speaking != -1 && (!IS_NPC(ch) || ch->speaking) )
		{
		    int speakswell = UMIN(knows_language(vch, ch->speaking, ch),
					  knows_language(ch, ch->speaking, vch));

		    if ( speakswell < 85 )
			sbuf = translate(speakswell, argument, lang_names[speaking]);
		}
#else
		if ( !knows_language( vch, ch->speaking, ch ) &&
			 (!IS_NPC(ch) && ch->speaking != 0) )
			sbuf = scramble(buf, ch->speaking);
#endif
		MOBtrigger = FALSE;
		act( AT_ACTION, "$n $t", ch, sbuf, vch, (vch == ch ? TO_CHAR : TO_VICT) );
	}
/*    MOBtrigger = FALSE;
    act( AT_ACTION, "$n $T", ch, NULL, buf, TO_ROOM );
    MOBtrigger = FALSE;
    act( AT_ACTION, "$n $T", ch, NULL, buf, TO_CHAR );*/
    ch->act = actflags;
    if ( IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
    {
	sprintf( buf, "%s %s (emote)", IS_NPC( ch ) ? ch->short_descr : ch->name,
		 argument );
	append_to_file( LOG_FILE, buf );
    }
    return;
}


void do_bug( CHAR_DATA *ch, char *argument )
{
    char    buf[MAX_STRING_LENGTH];
    struct  tm *t = localtime(&current_time);

    set_char_color( AT_PLAIN, ch );
    if ( argument[0] == '\0' ) {
        send_to_char( "\n\rUzycie:  'bug <komunikat>'  (twoja lokacja jest automatycznie zachowywana)\n\r", ch );
        return;
    }
    sprintf( buf, "(%-2.2d/%-2.2d):  %s",
	t->tm_mon+1, t->tm_mday, argument );
    append_file( ch, PBUG_FILE, buf );
    send_to_char( " informacje o bledzie.\n\r", ch );
    return;
}

void do_ide( CHAR_DATA *ch, char *argument )
{
    set_char_color( AT_PLAIN, ch );
    send_to_char("\n\rJesli chcesz wyslac jakas idee, wpisz 'idea <komunikat>'.\n\r", ch);
    send_to_char("Jesli chcesz zidentyfikowac przedmiot, uzyj czaru identify.\n\r", ch);
    return;
}

void do_idea( CHAR_DATA *ch, char *argument )
{
    set_char_color( AT_PLAIN, ch );
    if ( argument[0] == '\0' ) {
        send_to_char( "\n\rUzycie:  'idea <komunikat>'\n\r", ch );
        return;
    }
    append_file( ch, IDEA_FILE, argument );
    send_to_char( "Bogowie Astral Dominion sa Ci wdzieczni za Twoj pomysl.\n\r", ch );
    return;
}

void do_typo( CHAR_DATA *ch, char *argument )
{
    set_char_color( AT_PLAIN, ch );
    if ( argument[0] == '\0' ) {
        send_to_char( "\n\rUzycie:  'typo <komunikat>'  (Twoja lokacja zostanie automatycznie zachowana)\n\r", ch );
	if ( get_trust( ch ) >= LEVEL_ASCENDANT )
	  send_to_char( "Usage:  'typo list' lub 'typo clear now'\n\r", ch );
        return;
    }
    if ( !str_cmp( argument, "clear now" )
    &&    get_trust( ch ) >= LEVEL_ASCENDANT ) {
        FILE *fp = fopen( TYPO_FILE, "w" );
        if ( fp )
          fclose( fp );
        send_to_char( "Typo file cleared.\n\r", ch);
        return;
    }
    if ( !str_cmp( argument, "list" ) ) {
	send_to_char( "\n\r VNUM \n\r.......\n\r", ch );
        show_file( ch, TYPO_FILE );
     } else {
	append_file( ch, TYPO_FILE, argument );
	send_to_char( "Bogowie Astral Dominion sa Ci wdzieczni za informacje.\n\r", ch );
    }
    return;
}

void do_rent( CHAR_DATA *ch, char *argument )
{
    set_char_color( AT_WHITE, ch );
    send_to_char( "There is no rent here.  Just save and quit.\n\r", ch );
    return;
}



void do_qui( CHAR_DATA *ch, char *argument )
{
    set_char_color( AT_RED, ch );
    send_to_char( "Jesli chcesz zakonczyc gre, wpisz cala komende QUIT.\n\r", ch);
    return;
}

void do_quit( CHAR_DATA *ch, char *argument )
{
/*  OBJ_DATA *obj; */ /* Unused */
    AUTH_LIST *old_auth;    
    int x, y;
    int level;

    if ( IS_NPC(ch) )
	return;

    if ( ch->position == POS_FIGHTING
       || ch->position ==  POS_EVASIVE
       || ch->position ==  POS_DEFENSIVE
       || ch->position ==  POS_AGGRESSIVE
       || ch->position ==  POS_BERSERK
    )
    { 
	set_char_color( AT_RED, ch );
	send_to_char( "Nie ma mowy! Jestes w trakcie walki!\r", ch );
	return;
    }

    if ( ch->position  < POS_STUNNED  )
    {
        set_char_color( AT_BLOOD, ch );
	if ( ch->sex == 2 )
	send_to_char( "Jeszcze nie umarlas.\n\r", ch );
	else
	send_to_char( "Jeszcze nie umarles.\n\r", ch );
	return;
    }

    if ( get_timer(ch, TIMER_RECENTFIGHT) > 0
    &&  !IS_IMMORTAL(ch) )
    {
	set_char_color( AT_RED, ch );
	send_to_char( "Adrenalina krazy tak szybko. Nie mozesz teraz opuscic gry\n\r", ch );
	return;
    }

    if ( auction->item != NULL && ((ch == auction->buyer) || (ch == auction->seller) ) )
    {
	send_to_char("Poczekaj az sytuacja na aukcji sie rozwiaze.\n\r", ch);
	return;

    }

    if ( IS_PKILL( ch ) && ch->wimpy > (int) ch->max_hit / 2.25 )
    {
        send_to_char( "Twoj strach (wimpy) zostaje ustawiony na maksimom.\n\r", ch );
        do_wimpy( ch, "max" );
    }
    /* new auth */
    if ( NEW_AUTH( ch ) ) 
        remove_from_auth( ch->name );
    else
    {
        old_auth = get_auth_name( ch->name );
	  if( old_auth != NULL 
	  && (old_auth->state == AUTH_ONLINE || old_auth->state == AUTH_LINK_DEAD) )
                 old_auth->state = AUTH_OFFLINE; /* Logging off */
    }
    /* zeby sie nie dalo zaoszczedzac czasu na rezygnacji z questa przez quit */
    /* Crevan */
    if ( xIS_SET( ch->act, PLR_QUESTOR ) )
	do_aquest( ch, "giveup" );

    /* Get 'em dismounted until we finish mount saving -- Blodkai, 4/97 */
    if ( ch->position == POS_MOUNTED )
	do_dismount( ch, "" );
    set_char_color( AT_WHITE, ch );
    send_to_char( "Twoje otoczenia zaczyna sie powoli rozplywac, glosy slabna w oddali.\n\rZaczynasz odczuwac smutek spowodowany powrotem do realnego swiata...\n\r", ch );
    act( AT_SAY, "Odlegly glos mowi, 'Czekamy na Twoj powrot, $w...'", ch, NULL, NULL, TO_CHAR );
    act( AT_BYE, "$n opuszcza gre.", ch, NULL, NULL, TO_CANSEE );
    
    if( ch->pcdata->clan )
	check_virtual_channels( ch, ch->pcdata->clan->channel, ":wyszedl z gry." );
/*    do_clantalk( ch, "opuszcza gre");*/	/* Flash */

    set_char_color( AT_GREY, ch);

    sprintf( log_buf, "%s opuszcza gre (Pokoj %d).", ch->name, 
	( ch->in_room ? ch->in_room->vnum : -1 ) );
    quitting_char = ch;
    save_char_obj( ch );

    if ( sysdata.save_pets && ch->pcdata->pet )
    {
       act( AT_BYE, "$N odchodzi za wlascicielem.", ch, NULL, 
		ch->pcdata->pet, TO_ROOM );
       extract_char( ch->pcdata->pet, TRUE );
    }

    /* Synch clandata up only when clan member quits now. --Shaddai
     */
    if ( ch->pcdata->clan )
        save_clan( ch->pcdata->clan );     

    saving_char = NULL;

    level = get_trust(ch);
    /*
     * After extract_char the ch is no longer valid!
     */
    extract_char( ch, TRUE );

    for ( x = 0; x < MAX_WEAR; x++ )
	for ( y = 0; y < MAX_LAYERS; y++ )
	    save_equipment[x][y] = NULL;

    /* don't show who's logging off to leaving player */
/*
    to_channel( log_buf, CHANNEL_MONITOR, "Monitor", level ); 
*/
    log_string_plus( log_buf, LOG_COMM, level );
    return;
}


void send_rip_screen( CHAR_DATA *ch )
{
    FILE *rpfile;
    int num=0;
    char BUFF[MAX_STRING_LENGTH*2];

    if ((rpfile = fopen(RIPSCREEN_FILE,"r")) !=NULL) {
      while ((BUFF[num]=fgetc(rpfile)) != EOF)
	 num++;
      fclose(rpfile);
      BUFF[num] = 0;
      write_to_buffer(ch->desc,BUFF,num);
    }
}

void send_rip_title( CHAR_DATA *ch )
{
    FILE *rpfile;
    int num=0;
    char BUFF[MAX_STRING_LENGTH*2];

    if ((rpfile = fopen(RIPTITLE_FILE,"r")) !=NULL) {
      while ((BUFF[num]=fgetc(rpfile)) != EOF)
	 num++;
      fclose(rpfile);
      BUFF[num] = 0;
      write_to_buffer(ch->desc,BUFF,num);
    }
}

void send_ansi_title( CHAR_DATA *ch )
{
    FILE *rpfile;
    int num=0;
    char BUFF[MAX_STRING_LENGTH*2];

    if ((rpfile = fopen(ANSITITLE_FILE,"r")) !=NULL) {
      while ((BUFF[num]=fgetc(rpfile)) != EOF)
	 num++;
      fclose(rpfile);
      BUFF[num] = 0;
      write_to_buffer(ch->desc,BUFF,num);
    }
}

void send_ascii_title( CHAR_DATA *ch )
{
    FILE *rpfile;
    int num=0;
    char BUFF[MAX_STRING_LENGTH];

    if ((rpfile = fopen(ASCTITLE_FILE,"r")) !=NULL) {
      while ((BUFF[num]=fgetc(rpfile)) != EOF)
	 num++;
      fclose(rpfile);
      BUFF[num] = 0;
      write_to_buffer(ch->desc,BUFF,num);
    }
}

void do_omenu( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );
	 
    if ( arg1[0] == '\0' )
    {
        send_to_char( "Uzycie: omenu <objekt> [strona]  \n\r",     ch );
        send_to_char( "    Gdzie:    <objekt> jest prototypem objektu  \n\r",     ch );
        send_to_char( "        i     <strona>  jest opcjonalna litera do wyboru strony menu\n\r",     ch );
        return;
    }
	 
    if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "Nie ma tu kogos takiego.\n\r", ch );
	return;
    }

    /* can redit or something */

    ch->inter_type = OBJ_TYPE;
    ch->inter_substate = SUB_NORTH;
    if( ch->inter_editing != NULL) DISPOSE(ch->inter_editing);
    ch->inter_editing      =  str_dup(obj->pIndexData->name);
    sscanf(ch->inter_editing,"%s",ch->inter_editing);  /*one-arg*/
    ch->inter_editing_vnum =  obj->pIndexData->vnum;
    send_obj_page_to_char(ch, obj->pIndexData, arg2[0]);   
}


void do_rmenu( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *idx;
    char arg1[MAX_INPUT_LENGTH];

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
	 
    idx = ch->in_room;
    /* can redit or something */

    ch->inter_type = ROOM_TYPE;
    ch->inter_substate = SUB_NORTH;
    if( ch->inter_editing != NULL) DISPOSE(ch->inter_editing);
    ch->inter_editing      =  str_dup(idx->name);  
    sscanf(ch->inter_editing,"%s",ch->inter_editing);  /*one-arg*/
    ch->inter_editing_vnum =  idx->vnum;
    send_room_page_to_char(ch, idx, arg1[0]);   
}

void do_cmenu( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
	 
    ch->inter_type = CONTROL_TYPE;
    if( ch->inter_editing != NULL) DISPOSE(ch->inter_editing);
    ch->inter_editing      =  str_dup("Control Panel");  
    sscanf(ch->inter_editing,"%s",ch->inter_editing);  /*one-arg*/
    send_control_page_to_char(ch, arg1[0]);   
}


void do_mmenu( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );
	 
    if ( arg1[0] == '\0' )
    {
        send_to_char( "Uzycie: mmenu <ofiara> [strona]  \n\r",     ch );
        send_to_char( "    Gdzie:    <ofiara> jest prototypem moba  \n\r",     ch );
        send_to_char( "        i     <strona> jest opcjonalna litera do wyboru strony manu \n\r",     ch );
        return;
    }
      

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "Nie ma tu kogos takiego.\n\r", ch );
	return;
    }

    if ( !IS_NPC(victim) )
    {
	send_to_char( "Nie mozna tego stosowac do graczy.\n\r", ch );
	return;
    }

    if ( get_trust( ch ) < victim->level )
    {
	set_char_color( AT_IMMORT, ch );
	send_to_char( "Boska sila nie pozwala ci na to.\n\r", ch );
	return;
    }
    ch->inter_type = MOB_TYPE;
    if( ch->inter_editing != NULL) DISPOSE(ch->inter_editing);
    ch->inter_editing      =  str_dup(arg1);
    sscanf(ch->inter_editing,"%s",ch->inter_editing);  /*one-arg*/
    ch->inter_editing_vnum =  victim->pIndexData->vnum;
    send_page_to_char(ch, victim->pIndexData, arg2[0]);   
}


void do_rip( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Rip ON czy OFF?\n\r", ch );
	return;
    }
    if ( (strcmp(arg,"on")==0) || (strcmp(arg,"ON") == 0) ) {
	send_rip_screen(ch);
	xSET_BIT(ch->act,PLR_RIP);
	xSET_BIT(ch->act,PLR_ANSI);
	return;
    }

    if ( (strcmp(arg,"off")==0) || (strcmp(arg,"OFF") == 0) ) {
	xREMOVE_BIT(ch->act,PLR_RIP);
	send_to_char( "!|*\n\rRIP OFF...\n\r", ch );
	return;
    }
}

void do_ansi( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "ANSI ON czy OFF?\n\r", ch );
	return;
    }
    if ( (strcmp(arg,"on")==0) || (strcmp(arg,"ON") == 0) ) {
	xSET_BIT(ch->act,PLR_ANSI);
	set_char_color( AT_WHITE + AT_BLINK, ch);
	send_to_char( "ANSI ON!!!\n\r", ch);
	return;
    }

    if ( (strcmp(arg,"off")==0) || (strcmp(arg,"OFF") == 0) ) {
	xREMOVE_BIT(ch->act,PLR_ANSI);
	send_to_char( "ANSI OFF\n\r", ch );
	return;
    }
}

void do_save( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC(ch) )
	return;
    if ( ch->level < 2 ) {
	send_to_char_color( "&BMusisz miec co najmniej drugi poziom.\n\r", ch );
	return;
    }
    WAIT_STATE( ch, 2 ); /* For big muds with save-happy players, like RoD */
    update_aris(ch);     /* update char affects and RIS */
    save_char_obj( ch );
    saving_char = NULL;
    send_to_char( "Twoja postac zostala zachowana.\n\r", ch );
    return;
}


/*
 * Something from original DikuMUD that Merc yanked out.
 * Used to prevent following loops, which can cause problems if people
 * follow in a loop through an exit leading back into the same room
 * (Which exists in many maze areas)			-Thoric
 */
bool circle_follow( CHAR_DATA *ch, CHAR_DATA *victim )
{
    CHAR_DATA *tmp;

    for ( tmp = victim; tmp; tmp = tmp->master )
	if ( tmp == ch )
	  return TRUE;
    return FALSE;
}


void do_dismiss( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );     

    if ( arg[0] == '\0' )
    {
	send_to_char( "Kogo chcesz odprawic?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "Nie ma tu kogos takiego.\n\r", ch );
        return;
    }

    if ( ( IS_AFFECTED( victim, AFF_CHARM ) )
    && ( IS_NPC( victim ) )
    && ( victim->master == ch ) )
    {
	stop_follower( victim );
        stop_hating( victim );
        stop_hunting( victim );
        stop_fearing( victim );
        if ( victim->pIndexData->vnum == MOB_VNUM_SZKIELET )
	{
	    act( AT_MAGIC, "$n odprawia $B, ktory rozpada sie w pyl...", ch, NULL, victim, TO_ROOM );
	    act( AT_MAGIC, "Odprawiasz $B, ktory rozpada sie w pyl...", ch, NULL, victim, TO_CHAR );
	    while ( victim->first_carrying )
		extract_obj( victim->first_carrying );
	    extract_char( victim, TRUE );
	    return;
	}
	act( AT_ACTION, "$n odprawia $B.", ch, NULL, victim, TO_NOTVICT );
 	act( AT_ACTION, "Odprawiasz $B.", ch, NULL, victim, TO_CHAR );
    }
    else
    {
	send_to_char( "Nie mozesz odprawic Tej osoby.\n\r", ch );
    }

return;
}

void do_follow( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Za kim chcesz chodzic?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "Nie ma tu kogos takiego.\n\r", ch );
	return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master )
    {
	act( AT_PLAIN, "Teraz chodzisz za $Z!", ch, NULL, ch->master, TO_CHAR );
	return;
    }

    if ( victim == ch )
    {
	if ( !ch->master )
	{
	    send_to_char( "Chodzisz juz za soba.\n\r", ch );
	    return;
	}
	stop_follower( ch );
	return;
    }

    if ( ( ch->level - victim->level < -10 || ch->level - victim->level >  10 )
    &&   !IS_HERO(ch) && !(ch->level < 15 && !IS_NPC(victim) 
    && victim->pcdata->council 
    && !str_cmp(victim->pcdata->council->name,"Newbie Council")))
    {
	send_to_char( "Niestety nie mozesz chodzic za ta osoba.\n\r", ch );
	return;
    }

    if ( circle_follow( ch, victim ) )
    {
	send_to_char( "Chodzienie w kolko?... niestety.\n\r", ch );
	return;
    }

    if ( ch->master )
	stop_follower( ch );

    add_follower( ch, victim );
    return;
}



void add_follower( CHAR_DATA *ch, CHAR_DATA *master )
{
    if ( ch->master )
    {
	bug( "Add_follower: non-null master.", 0 );
	return;
    }

    ch->master        = master;
    ch->leader        = NULL;

    /* Support for saving pets --Shaddai */
    if ( IS_NPC(ch) && xIS_SET(ch->act, ACT_PET) && !IS_NPC(master) )
	master->pcdata->pet = ch;

    if ( can_see( master, ch ) )
    act( AT_ACTION, "$n chodzi od tej pory za Toba.", ch, NULL, master, TO_VICT );

    act( AT_ACTION, "Od teraz chodzisz za $Z.",  ch, NULL, master, TO_CHAR );

    return;
}



void stop_follower( CHAR_DATA *ch )
{
    if ( !ch->master )
    {
	bug( "Stop_follower: null master.", 0 );
	return;
    }

    if ( IS_NPC(ch) && !IS_NPC(ch->master) && ch->master->pcdata->pet == ch )
	ch->master->pcdata->pet = NULL;

    if ( IS_AFFECTED(ch, AFF_CHARM) )
    {
	xREMOVE_BIT( ch->affected_by, AFF_CHARM );
	affect_strip( ch, gsn_charm_person );
    }

    if ( can_see( ch->master, ch ) )
	if (!(!IS_NPC(ch->master) && IS_IMMORTAL(ch) && !IS_IMMORTAL(ch->master)))
	    act( AT_ACTION, "$n przestaje za Toba chodzic.",     ch, NULL, ch->master, TO_VICT  );
    act( AT_ACTION, "Przestajesz chodzic za $Z.",      ch, NULL, ch->master, TO_CHAR    );

    ch->master = NULL;
    ch->leader = NULL;
    return;
}



void die_follower( CHAR_DATA *ch )
{
    CHAR_DATA *fch;

    if ( ch->master )
	stop_follower( ch );

    ch->leader = NULL;

    for ( fch = first_char; fch; fch = fch->next )
    {
	if ( fch->master == ch )
	    stop_follower( fch );
	if ( fch->leader == ch )
	    fch->leader = fch;
    }
    return;
}



void do_order( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char argbuf[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *och;
    CHAR_DATA *och_next;
    bool found;
    bool fAll;

    strcpy( argbuf, argument );
    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
	send_to_char( "Co i komu rozkazac?\n\r", ch );
	return;
    }

    if ( IS_AFFECTED( ch, AFF_CHARM ) )
    {
	send_to_char( "Ty raczej przyjmujesz niz wydajesz rozkazy.\n\r", ch );
	return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
	fAll   = TRUE;
	victim = NULL;
    }
    else
    {
	fAll   = FALSE;
	if ( ( victim = get_char_room( ch, arg ) ) == NULL )
	{
	    send_to_char( "Nie ma tu kogos innego.\n\r", ch );
	    return;
	}

	if ( victim == ch )
	{
	    send_to_char( "Tak, tak, juz robisz!\n\r", ch );
	    return;
	}

	if ( !IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch )
	{
	    send_to_char( "Zrob to sam!\n\r", ch );
	    return;
	}
    }

    found = FALSE;
    for ( och = ch->in_room->first_person; och; och = och_next )
    {
	och_next = och->next_in_room;

	if ( IS_AFFECTED(och, AFF_CHARM) && och->master == ch 
	&& ( fAll || och == victim ) )
	{
	    found = TRUE;
	act( AT_ACTION, "$n rozkazuje Ci '$t'.", ch, argument, och, TO_VICT );
	    interpret( och, argument );
	}
    }

    if ( found )
    {
        sprintf( log_buf, "%s: rozkazuje %s.", ch->name, argbuf );
        log_string_plus( log_buf, LOG_NORMAL, ch->level );
 	send_to_char( "Ok.\n\r", ch );
        WAIT_STATE( ch, 12 );
    }
    else
	send_to_char( "Nikt za Toba teraz nie chodzi.\n\r", ch );
    return;
}

/*
char *itoa(int foo)
{
  static char bar[256];

  sprintf(bar,"%d",foo);
  return(bar);

}
*/

/* Overhauled 2/97 -- Blodkai */
void do_group( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	CHAR_DATA *gch;
	CHAR_DATA *leader;

	leader = ch->leader ? ch->leader : ch;
        set_char_color( AT_DGREEN, ch );
        ch_printf( ch, "\n\r    Grupa %-12.12s     [hitpnts]   [ magia ] [mst] [mvs] [rasa]%s\n\r",
		PERSC(leader, ch),
		ch->level < 50 ? " [do poz]" : "" );
	for ( gch = first_char; gch; gch = gch->next )
	{
	    if ( is_same_group( gch, ch ) )
	    {
		set_char_color( AT_DGREEN, ch );
		if (IS_AFFECTED(gch, AFF_POSSESS)) /*reveal no information*/
		  ch_printf( ch,
			"[%2d %s] %-16s %4s/%4s hp %4s/%4s %s %4s/%4s mv %5s xp\n\r",
		    	gch->level,
		    	IS_NPC(gch) ? "Mob" : class_table[gch->class]->who_name,
		    	capitalize( PERS(gch, ch) ),
		    	"????",   
		    	"????",
		    	"????",
		    	"????",
		    	( IS_VAMPIRE(gch) || IS_DEMON(gch)) ? "bp" : "mana",
		    	"????",  
		    	"????",  
		    	"?????"    );
		else
              	    if      ( gch->alignment >  750 ) sprintf(buf, " A");
                    else if ( gch->alignment >  350 ) sprintf(buf, "-A");
                    else if ( gch->alignment >  150 ) sprintf(buf, "+N");
                    else if ( gch->alignment > -150 ) sprintf(buf, " N");
                    else if ( gch->alignment > -350 ) sprintf(buf, "-N");
                    else if ( gch->alignment > -750 ) sprintf(buf, "+S");
                    else                              sprintf(buf, " S");
		    set_char_color( AT_DGREEN, ch );
		    send_to_char( "[", ch );
		    set_char_color( AT_GREEN, ch );
		    ch_printf( ch, "%-2d %2.2s %3.3s",
			gch->level,
			buf,
			IS_NPC(gch) ? "Mob" : class_table[gch->class]->who_name );
		    set_char_color( AT_DGREEN, ch );
		    send_to_char( "]  ", ch );
		    set_char_color( AT_GREEN, ch );
                    ch_printf( ch, "%-12.12s ",
                        capitalize( PERS(gch, ch) ) );
                    if ( gch->hit < gch->max_hit/4 )
                      set_char_color( AT_DANGER, ch );
                    else if ( gch->hit < gch->max_hit/2.5 )
                      set_char_color( AT_YELLOW, ch );
                    else set_char_color( AT_GREY, ch );
                    ch_printf( ch, "%5d", gch->hit );
                    set_char_color( AT_GREY, ch );
                    ch_printf( ch, "/%-5d ", gch->max_hit );
                    if ( IS_VAMPIRE(gch) || IS_DEMON(gch) )
                      set_char_color( AT_BLOOD, ch );
                    else
                      set_char_color( AT_LBLUE, ch );
		    if ( gch->class != CLASS_WARRIOR )
                      ch_printf( ch, "%5d/%-5d ",
			(IS_VAMPIRE(gch) || IS_DEMON(gch)) ? gch->pcdata->condition[COND_BLOODTHIRST] : gch->mana,
                        (IS_VAMPIRE(gch) || IS_DEMON(gch)) ? 10 + gch->level : gch->max_mana );
		    else
		      send_to_char( "            ", ch );
		    if ( gch->mental_state < -25 || gch->mental_state > 25 )
		      set_char_color( AT_YELLOW, ch );
		    else
		      set_char_color( AT_GREEN, ch );
                    ch_printf( ch, "%3.3s  ",
                        gch->mental_state > 75  ? "+++" :
                        gch->mental_state > 50  ? "=++" :
                        gch->mental_state > 25  ? "==+" :
                        gch->mental_state > -25 ? "===" :
                        gch->mental_state > -50 ? "-==" :
                        gch->mental_state > -75 ? "--=" :
                                                  "---" );
		    set_char_color( AT_DGREEN, ch );
                    ch_printf( ch, "%5d ",
                        gch->move );
		    ch_printf( ch, "%6s ",
			gch->race == 0 ? "human" :
			gch->race == 1 ? "elf" :
			gch->race == 2 ? "dwarf" :
                        gch->race == 3 ? "hlflng" :
                        gch->race == 4 ? "pixie" :
                        gch->race == 5 ? "vampir" :
                        gch->race == 6 ? "h-ogre" :
                        gch->race == 7 ? "h-orc" :
                        gch->race == 8 ? "h-trol" :
                        gch->race == 9 ? "h-elf" :
                        gch->race ==10 ? "gith" :
			gch->race ==11 ? "drow" :
			gch->race ==12 ? "seaelf" :
			gch->race ==13 ? "lizard" :
			gch->race ==14 ? "gnome" :
					 "" );
		    set_char_color( AT_GREEN, ch );
		    if ( gch->level < 50 )
		      ch_printf( ch, "%8d ",
			exp_level( gch, gch->level+1) - gch->exp );
		    send_to_char( "\n\r", ch);
	    }
	}
	return;
    }

    if ( !strcmp( arg, "disband" ))
    {
	CHAR_DATA *gch;
	int count = 0;

	if ( ch->leader || ch->master )
	{
	    send_to_char( "Nie mozesz rozwiazac grupy gdy za kims chodzisz.\n\r", ch );
	    return;
	}
	
	for ( gch = first_char; gch; gch = gch->next )
	{
	    if ( is_same_group( ch, gch )
	    && ( ch != gch ) )
	    {
		gch->leader = NULL;
		gch->master = NULL;
		count++;
		send_to_char( "Twoja grupa zostaje rozwiazana.\n\r", gch );
	    }
	}

	if ( count == 0 )
	   send_to_char( "Nie posiadasz nikogo w grupie.\n\r", ch );
	else
	   send_to_char( "Rozwiazujesz swa grupe.\n\r", ch );
	
	return;
    }

    if ( !strcmp( arg, "all" ) )
    {
	CHAR_DATA *rch;
	int count = 0;

        for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
	{
           if ( ch != rch
           &&   !IS_NPC( rch )
	   &&   can_see( ch, rch )
	   &&   rch->master == ch
	   &&   !ch->master
	   &&   !ch->leader
    	   &&   abs( ch->level - rch->level ) < 8
    	   &&   !is_same_group( rch, ch )
   	   &&   IS_PKILL( ch ) == IS_PKILL( rch )
	      )
	   {
		rch->leader = ch;
		count++;
	   }
	}
	
	if ( count == 0 )
	  send_to_char( "Nie ma chetnych do twoje grupy.\n\r", ch );
	else
	{
	if ( ch->sex == 2)
     	   act( AT_ACTION, "$n grupuje osoby chodzace za Nia.", ch, NULL, NULL, TO_ROOM );
	else
     	   act( AT_ACTION, "$n grupuje osoby chodzace za Nim.", ch, NULL, NULL, TO_ROOM );
	   send_to_char( "Grupujesz osoby chodzace za Toba\n\r", ch );
	}
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "Nie ma tu kogos takiego.\n\r", ch );
	return;
    }

    if ( ch->master || ( ch->leader && ch->leader != ch ) )
    {
	send_to_char( "Chodzisz juz za kims innym!\n\r", ch );
	return;
    }

    if ( victim->master != ch && ch != victim )
    {
	act( AT_PLAIN, "$N nie chodzi za Toba.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if ( victim == ch )
    {
        act( AT_PLAIN, "Nie mozesz przylaczyc SIEBIE.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( is_same_group( victim, ch ) && ch != victim )
    {
	victim->leader = NULL;
	act( AT_ACTION, "$n usuwa $B ze swojej grupy.",   ch, NULL, victim, TO_NOTVICT );
	act( AT_ACTION, "$n usuwa Cie ze swojej grupy.",  ch, NULL, victim, TO_VICT    );
	act( AT_ACTION, "Usuwasz $B ze swojej grupy.", ch, NULL, victim, TO_CHAR    );
	return;
    }

    if ( ch->level - victim->level < -8
    ||   ch->level - victim->level >  8 
    ||   ( IS_PKILL( ch ) != IS_PKILL( victim ) ) )
    {
	act( AT_PLAIN, "$N nie moze przylaczyc sie do grupy $c.",     ch, NULL, victim, TO_NOTVICT );
	act( AT_PLAIN, "Nie mozesz przylaczyc sie do grupy $c.",    ch, NULL, victim, TO_VICT    );
	act( AT_PLAIN, "$N nie moze przylaczyc sie do twojej grupy.",     ch, NULL, victim, TO_CHAR    );
	return;
    }

    victim->leader = ch;
    act( AT_ACTION, "$N przylacza sie do grupy $c.", ch, NULL, victim, TO_NOTVICT);
    act( AT_ACTION, "Przylaczasz sie do grupy $c.", ch, NULL, victim, TO_VICT    );
    act( AT_ACTION, "$N przylacza sie do Twojej grupy.", ch, NULL, victim, TO_CHAR    );
    return;
}



/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void do_split( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    int members;
    int amount;
    int share;
    int extra;
    int tax;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Ile chcesz rozdzielic?\n\r", ch );
	return;
    }

    amount = atoi( arg );

    if ( amount < 0 )
    {
	send_to_char( "Twoja grupa sie nie ucieszy.\n\r", ch );
	return;
    }

    if ( amount == 0 )
    {
	send_to_char( "Rozdajesz po 0 sztuk zlota, ale nikt tego nie zauwaza.\n\r", ch );
	return;
    }

    if ( ch->gold < amount )
    {
	send_to_char( "Nie posiadasz tyle zlota.\n\r", ch );
	return;
    }

    members = 0;
    for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
    {
	if ( is_same_group( gch, ch ) )
	    members++;
    }

    
    if ( xIS_SET(ch->act, PLR_AUTOGOLD) && members < 2 )
	return;
    
    if ( members < 2 )
    {
	send_to_char( "Zatrzymaj wszystko dla siebie.\n\r", ch );
	return;
    }

    share = amount / members;
    extra = amount % members;

    if ( share == 0 )
    {
	send_to_char( "Nie zawracaj sobie tym glowy, skapcze.\n\r", ch );
	return;
    }
    
    ch->gold += share + extra;
    ch->gold -= amount;

    set_char_color( AT_GOLD, ch );
    ch_printf( ch,
	"Rozdzielasz %d szt. zlota. Twoja dzialka wynosi %d szt. zlota.\n\r",
	amount, share + extra );

    if ( ch->pcdata->clan && ch->pcdata->clan->clantax > 0 )
    {
	tax = ( share + extra ) * ch->pcdata->clan->clantax / 100;
	ch->gold -= tax;
	ch->pcdata->clan->balance += tax;
	save_clan( ch->pcdata->clan );
	ch_printf( ch, "Jako podatek klanowy oddajesz %d szt. zlota.\n\r", tax );
    }

    sprintf( buf, "$n dzieli %d szt. zlota. Twoja dzialka wynosi %d szt. zlota.",
	amount, share );

    for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
    {
	if ( gch != ch && is_same_group( gch, ch ) ) 
	{
	    act( AT_GOLD, buf, ch, NULL, gch, TO_VICT );
	    if ( gch->pcdata->clan && gch->pcdata->clan->clantax > 0 )
	    {
		gch->gold += share;
		tax = share * gch->pcdata->clan->clantax / 100;
		if ( tax > 0 )
		{
		gch->gold -= tax;
		gch->pcdata->clan->balance += tax;
		sprintf( buf, "Jako podatek klanowy oddajesz %d szt. zlota.\n\r", tax );
		save_clan( get_clan( gch->pcdata->clan->name ) );
		act( AT_GOLD, buf, ch, NULL, gch, TO_VICT );
		}
	    }
	    else
		gch->gold += share;
	}
    }
    return;
}



void do_gtell( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *gch;
#ifndef SCRAMBLE
    int speaking = -1, lang;

    for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
	if ( ch->speaking & lang_array[lang] )
	{
	    speaking = lang;
	    break;
	}
#endif

    if ( argument[0] == '\0' )
    {
	send_to_char( "Co powiedziec do grupy?\n\r", ch );
	return;
    }

    if ( xIS_SET(ch->act, PLR_NO_TELL) )
    {
	send_to_char( "Twoj komunikat do nikogo nie dociera!\n\r", ch );
	return;
    }

    /*
     * Note use of send_to_char, so gtell works on sleepers.
     */
/*    sprintf( buf, "%s mowi do grupy '%s'.\n\r", ch->name, argument );*/
    for ( gch = first_char; gch; gch = gch->next )
    {
	if ( is_same_group( gch, ch ) )
	{
	    set_char_color( AT_GTELL, gch );
	    /* Groups unscrambled regardless of clan language.  Other languages
		   still garble though. -- Altrag */
#ifndef SCRAMBLE
	    if ( speaking != -1 && (!IS_NPC(ch) || ch->speaking) )
	    {
		int speakswell = UMIN(knows_language(gch, ch->speaking, ch),
				      knows_language(ch, ch->speaking, gch));

		if ( speakswell < 85 )
		    ch_printf( gch, "%s mowi do grupy '%s'.\n\r", ch->name, translate(speakswell, argument, lang_names[speaking]) );
		else
		    ch_printf( gch, "%s mowi do grupy '%s'.\n\r", ch->name, argument );
	    }
	    else
		ch_printf( gch, "%s mowi do grupy '%s'.\n\r", ch->name, argument );
#else
	    if ( knows_language( gch, ch->speaking, gch )
	    ||  (IS_NPC(ch) && !ch->speaking) )
		ch_printf( gch, "%s mowi do grupy '%s'.\n\r", ch->name, argument );
	    else
		ch_printf( gch, "%s mowi do grupy '%s'.\n\r", ch->name, scramble(argument, ch->speaking) );
#endif
	}
    }

    return;
}


/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group( CHAR_DATA *ach, CHAR_DATA *bch )
{
    if ( ach->leader ) ach = ach->leader;
    if ( bch->leader ) bch = bch->leader;
    return ach == bch;
}

/*
 * this function sends raw argument over the AUCTION: channel
 * I am not too sure if this method is right..
 */

void talk_auction (char *argument)
{
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *original;

    sprintf (buf,"Auction: %s", argument); /* last %s to reset color */

    for (d = first_descriptor; d; d = d->next)
    {
        original = d->original ? d->original : d->character; /* if switched */
        if ((d->connected == CON_PLAYING) && !IS_SET(original->deaf,CHANNEL_AUCTION) 
        && !IS_SET(original->in_room->room_flags, ROOM_SILENCE) && !NEW_AUTH(original))
            act( AT_GOSSIP, buf, original, NULL, NULL, TO_CHAR );
    }
}

/*
 * Language support functions. -- Altrag
 * 07/01/96
 *
 * Modified to return how well the language is known 04/04/98 - Thoric
 * Currently returns 100% for known languages... but should really return
 * a number based on player's wisdom (maybe 50+((25-wisdom)*2) ?)
 */
int knows_language( CHAR_DATA *ch, int language, CHAR_DATA *cch )
{
	sh_int sn;

	if ( !IS_NPC(ch) && IS_IMMORTAL(ch) )
		return 100;
	if ( IS_NPC(ch) && !ch->speaks ) /* No langs = knows all for npcs */
		return 100;
	if ( IS_NPC(ch) && IS_SET(ch->speaks, (language & ~LANG_CLAN)) )
		return 100;
	/* everyone KNOWS common tongue */
	if ( IS_SET(language, LANG_COMMON) )
		return 100;
	if ( language & LANG_CLAN )
	{
		/* Clan = common for mobs.. snicker.. -- Altrag */
		if ( IS_NPC(ch) || IS_NPC(cch) )
			return 100;
		if ( ch->pcdata->clan == cch->pcdata->clan &&
			 ch->pcdata->clan != NULL )
			return 100;
	}
	if ( !IS_NPC( ch ) )
	{
	    int lang;
	    
		/* Racial languages for PCs */
	    if ( IS_SET(race_table[ch->race]->language, language) )
	    	return 100;

	    for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
	      if ( IS_SET(language, lang_array[lang]) &&
	      	   IS_SET(ch->speaks, lang_array[lang]) )
	      {
		  if ( (sn = skill_lookup(lang_names[lang])) != -1 )
		    return ch->pcdata->learned[sn];
	      }
	}
	return 0;
}

bool can_learn_lang( CHAR_DATA *ch, int language )
{
	if ( language & LANG_CLAN )
		return FALSE;
	if ( IS_NPC(ch) || IS_IMMORTAL(ch) )
		return FALSE;
	if ( race_table[ch->race]->language & language )
		return FALSE;
	if ( ch->speaks & language )
	{
		int lang;
		
		for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
			if ( language & lang_array[lang] )
			{
				int sn;
				
				if ( !(VALID_LANGS & lang_array[lang]) )
					return FALSE;
				if ( ( sn = skill_lookup( lang_names[lang] ) ) < 0 )
				{
					bug( "Can_learn_lang: valid language without sn: %d", lang );
					continue;
				}
				if ( ch->pcdata->learned[sn] >= 99 )
					return FALSE;
			}
	}
	if ( VALID_LANGS & language )
		return TRUE;
	return FALSE;
}

int const lang_array[] = {
 LANG_COMMON, LANG_ELVEN, LANG_DWARVEN, LANG_PIXIE,
 LANG_OGRE, LANG_ORCISH, LANG_TROLLISH, LANG_RODENT,
 LANG_INSECTOID, LANG_MAMMAL, LANG_REPTILE,
 LANG_DRAGON, LANG_SPIRITUAL, LANG_MAGICAL,
 LANG_GOBLIN, LANG_GOD, LANG_ANCIENT, LANG_HALFLING,
 LANG_CLAN, LANG_GITH, LANG_UNKNOWN };

char * const lang_names[] = {
 "common", "elvish", "dwarven", "pixie", "ogre",
 "orcish", "trollese", "rodent", "insectoid",
 "mammal", "reptile", "dragon", "spiritual",
 "magical", "goblin", "god", "ancient",
 "halfling", "clan", "gith", "" };


/* Note: does not count racial language.  This is intentional (for now). */
int countlangs( int languages )
{
	int numlangs = 0;
	int looper;

	for ( looper = 0; lang_array[looper] != LANG_UNKNOWN; looper++ )
	{
		if ( lang_array[looper] == LANG_CLAN )
			continue;
		if ( languages & lang_array[looper] )
			numlangs++;
	}
	return numlangs;
}

void do_speak( CHAR_DATA *ch, char *argument )
{
	int langs;
	char arg[MAX_INPUT_LENGTH];
	
	argument = one_argument(argument, arg );
	
	if ( !str_cmp( arg, "all" ) && IS_IMMORTAL( ch ) )
	{
		set_char_color( AT_SAY, ch );
		ch->speaking = ~LANG_CLAN;
		send_to_char( "Od teraz mowisz wszystkimi jezykami.\n\r", ch );
		return;
	}
	for ( langs = 0; lang_array[langs] != LANG_UNKNOWN; langs++ )
		if ( !str_prefix( arg, lang_names[langs] ) )
			if ( knows_language( ch, lang_array[langs], ch ) )
			{
				if ( lang_array[langs] == LANG_CLAN &&
					(IS_NPC(ch) || !ch->pcdata->clan) )
					continue;
				ch->speaking = lang_array[langs];
				set_char_color( AT_SAY, ch );
				ch_printf( ch, "Od teraz mowisz jezykiem %s.\n\r", lang_names[langs] );
				return;
			}
	set_char_color( AT_SAY, ch );
	send_to_char( "Nie znasz tego jezyka.\n\r", ch );
}

void do_languages( CHAR_DATA *ch, char *argument )
{
	char arg[MAX_INPUT_LENGTH];
	int lang;
	
	argument = one_argument( argument, arg );
	if ( arg[0] != '\0' && !str_prefix( arg, "learn" ) &&
		!IS_IMMORTAL(ch) && !IS_NPC(ch) )
	{
		CHAR_DATA *sch;
		char arg2[MAX_INPUT_LENGTH];
		int sn;
		int prct;
		int prac;
		
		argument = one_argument( argument, arg2 );
		if ( arg2[0] == '\0' )
		{
			send_to_char( "Ktorego jezyka chcesz sie uczyc?\n\r", ch );
			return;
		}
		for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
		{
			if ( lang_array[lang] == LANG_CLAN )
				continue;
			if ( !str_prefix( arg2, lang_names[lang] ) )
				break;
		}
		if ( lang_array[lang] == LANG_UNKNOWN )
		{
			send_to_char( "Nie ma tu takiego jezyka.\n\r", ch );
			return;
		}
		if ( !(VALID_LANGS & lang_array[lang]) )
		{
			send_to_char( "Nie mozesz uczyc sie takiego jezyka.\n\r", ch );
			return;
		}
		if ( ( sn = skill_lookup( lang_names[lang] ) ) < 0 )
		{
			send_to_char( "To nie jest jezyk.\n\r", ch );
			return;
		}
		if ( race_table[ch->race]->language & lang_array[lang] ||
			 lang_array[lang] == LANG_COMMON ||
			 ch->pcdata->learned[sn] >= 99 )
		{
			act( AT_PLAIN, "Przeciez juz mowisz plynnie w jezyku $t.", ch,
				 lang_names[lang], NULL, TO_CHAR );
			return;
		}
		for ( sch = ch->in_room->first_person; sch; sch = sch->next )
			if ( IS_NPC(sch) && xIS_SET(sch->act, ACT_SCHOLAR)
			&&   knows_language( sch, ch->speaking, ch )
			&&   knows_language( sch, lang_array[lang], sch )
			&& (!sch->speaking || knows_language( ch, sch->speaking, sch )) )
				break;
		if ( !sch )
		{
			send_to_char( "Nie ma tu nikogo kto moglby Cie nauczyc tego jezyka.\n\r", ch );
			return;
		}
		if ( countlangs(ch->speaks) >= (ch->level / 10) &&
			 ch->pcdata->learned[sn] <= 0 )
		{
			act( AT_TELL, "$n mowi Ci 'Nie mozesz sie jeszcze uczyc nowego jezyka.'",
				 sch, NULL, ch, TO_VICT );
			return;
		}
		/* 0..16 cha = 2 pracs, 17..25 = 1 prac. -- Altrag */
		prac = 2 - (get_curr_cha(ch) / 17);
		if ( ch->practice < prac )
		{
			act( AT_TELL, "$n mowi Ci 'Nie masz tylu punktow praktyki.'",
				 sch, NULL, ch, TO_VICT );
			return;
		}
		ch->practice -= prac;
		/* Max 12% (5 + 4 + 3) at 24+ int and 21+ wis. -- Altrag */
		prct = 5 + (get_curr_int(ch) / 6) + (get_curr_wis(ch) / 7);
		ch->pcdata->learned[sn] += prct;
		ch->pcdata->learned[sn] = UMIN(ch->pcdata->learned[sn], 99);
		SET_BIT( ch->speaks, lang_array[lang] );
		if ( ch->pcdata->learned[sn] == prct )
			act( AT_PLAIN, "Zaczynasz uczyc sie jezyka $t.", ch, lang_names[lang],
				 NULL, TO_CHAR );
		else if ( ch->pcdata->learned[sn] < 60 )
			act( AT_PLAIN, "Kontynuujesz nauke jezyka $t.", ch, lang_names[lang],
				 NULL, TO_CHAR );
		else if ( ch->pcdata->learned[sn] < 60 + prct )
			act( AT_PLAIN, "Czujesz, ze potrafisz juz porozumiewac sie w jezyku $t.", ch,
				 lang_names[lang], NULL, TO_CHAR );
		else if ( ch->pcdata->learned[sn] < 99 )
			act( AT_PLAIN, "Zaczynasz plynnie mowic w jezyku $t.", ch,
				 lang_names[lang], NULL, TO_CHAR );
		else
			act( AT_PLAIN, "Wiesz juz wszystko o jezyku $t.", ch, lang_names[lang],
				 NULL, TO_CHAR );
		return;
	}
	for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
		if ( knows_language( ch, lang_array[lang], ch ) )
		{
			if ( ch->speaking & lang_array[lang] ||
				(IS_NPC(ch) && !ch->speaking) )
				set_char_color( AT_SAY, ch );
			else
				set_char_color( AT_PLAIN, ch );
			send_to_char( lang_names[lang], ch );
			send_to_char( "\n\r", ch );
		}
	send_to_char( "\n\r", ch );
	return;
}

void do_wartalk( CHAR_DATA *ch, char *argument )
{
    if (NEW_AUTH(ch))
    {
      send_to_char("Ze co?\n\r", ch);
      return;
    }
    talk_channel( ch, argument, CHANNEL_WARTALK, "war" );
    return;
}

void do_racetalk( CHAR_DATA *ch, char *argument )
{
  if (NEW_AUTH(ch))
  {
    send_to_char("Ze co?\n\r", ch);
    return;
  }
  talk_channel( ch, argument, CHANNEL_RACETALK, "racetalk" );
  return;
}


void
init_profanity_checker()
{
#ifndef WIN32
  bigregex = (char *) malloc (4096);
  add_profane_word("fuck");
  /* skip over first 2 slashes and bar */
  bigregex+=3;
  
  add_profane_word("shit");
  add_profane_word("cunt");
  
  preg = re_comp( (char *) bigregex );
#endif
}

void add_profane_word( char * word)
{
#ifndef WIN32
   char _word[4096];
   int i,j;
   
   j=0;
   _word[j] = '\\'; j++;
   _word[j] = '|'; j++;
   _word[j] = '\\'; j++;
   _word[j] = '('; j++;
   for(i=0;i<strlen(word);i++){
      _word[j]='['; j++;
      _word[j]=tolower(word[i]); j++;
      _word[j]=toupper(word[i]); j++;
      _word[j]=']'; j++;
      _word[j]='+'; j++;
      _word[j]='['; j++;
   
   
   
      _word[j]='-'; j++;
      _word[j]=' '; j++;
      _word[j]='\t'; j++;
   
      _word[j]='`'; j++;
      _word[j]='~'; j++;
      _word[j]='1'; j++;
      _word[j]='!'; j++;
      _word[j]='2'; j++;
      _word[j]='@'; j++;
      _word[j]='3'; j++;
      _word[j]='#'; j++;
      _word[j]='4'; j++;
      _word[j]='5'; j++;
      _word[j]='%'; j++;
      _word[j]='6'; j++;
      _word[j]='7'; j++;
      _word[j]='&'; j++;
      _word[j]='8'; j++;
      _word[j]='9'; j++;
      _word[j]='0'; j++;
      _word[j]='_'; j++;
      _word[j]=';'; j++;
      _word[j]=':'; j++;
      _word[j]=','; j++;
      _word[j]='<'; j++;
      /* These need to be escaped  for C */
   
   
      _word[j]='\''; j++;
      _word[j]='\\'; j++;
      _word[j]='\"'; j++;
   
      /* These need to be escaped  for regex*/
      _word[j]='\\'; j++;
      _word[j]='$'; j++;
   
      _word[j]='>'; j++;
      _word[j]='/'; j++;
      _word[j]='\\'; j++;
      _word[j]='^'; j++;
      _word[j]='\\'; j++;
      _word[j]='.'; j++;
      _word[j]='\\'; j++;
      _word[j]=')'; j++;
      _word[j]='\\'; j++;
      _word[j]='?'; j++;
      _word[j]='\\'; j++;
      _word[j]='*'; j++;
   
      _word[j]='\\'; j++;
      _word[j]='('; j++;
      _word[j]='\\'; j++;
      _word[j]='['; j++;
   
      _word[j]='\\'; j++;
      _word[j]='{'; j++;
      _word[j]='\\'; j++;
      _word[j]='+'; j++;
   
#ifdef BIG
   /* i don't get what the deal is with this guy, it seems unescapable,
      so to speak. */
      _word[j]='\\'; j++;
      _word[j]=']'; j++;
#endif
      _word[j]='\\'; j++;
      _word[j]='}'; j++;
      _word[j]='\\'; j++;
      _word[j]='|'; j++;
      _word[j]='\\'; j++;
      _word[j]='='; j++;
   
      /* close up funny characters */
      _word[j]=']'; j++;
      _word[j]='*'; j++;
   }
   _word[j] = '\\'; j++;
   _word[j] = ')'; j++;
   _word[j]='\0';
   
   strcat(bigregex, _word);
#endif
}

int is_profane (char *what)
{
#ifndef WIN32
  int ret;

  ret = re_exec(what);
  if (ret==1)
     return(1);
#endif
  return(0); 
}

/* Installed by Samson on unknown date, allows user to beep other users */
void do_beep(CHAR_DATA *ch, char *argument) 
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
 
    argument = one_argument(argument, arg);

    if (!*arg || !(victim = get_char_world(ch, arg)))
    {
      send_to_char( "Beep who?\n\r", ch );
      return;
    }

    /* NPC check added by Samson 2-15-98 */
    if ( IS_NPC( victim ))
    {
	send_to_char( "Beep who?\n\r", ch );
	return;
    }

    /* PCFLAG_NOBEEP check added by Samson 2-15-98 */
    if ( IS_SET(victim->pcdata->flags, PCFLAG_NOBEEP) )
    {
	ch_printf( ch, "%s is not accepting beeps at this time.\n\r", victim->name );
	return;
    }

    ch_printf(victim, "%s is beeping you!\a\n\r", PERS(ch, victim));
    ch_printf(ch, "You beep %s.\n\r", PERS(victim, ch));
    return;
}