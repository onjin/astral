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
 *                        Alias module                                      *
 ****************************************************************************/
/******************************************************
            Desolation of the Dragon MUD II
      (C) 1997, 1998  Jesse DeFer and Heath Leach
 http://dotd.mudservices.com  dotd@dotd.mudservices.com 
 ******************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "mud.h"

void do_alias( CHAR_DATA *ch, char *argument )
{
    ALIAS_DATA *pal = NULL;
    char arg[MAX_INPUT_LENGTH];
    
    if (!ch || !ch->pcdata)
        return;
    
    argument = one_argument(argument, arg);
    
    if ( !*arg ) 
    {
        if (!ch->pcdata->first_alias)
        {
            send_to_char("Nie masz zdefiniowanych aliasow!\n\r", ch);
            return;
        }
        pager_printf( ch, "%-20s Co to oznacza\n\r", "Alias" );
        for (pal=ch->pcdata->first_alias;pal;pal=pal->next)
            pager_printf( ch, "%-20s %s\n\r",
                          pal->name, pal->cmd );
        return;
    }
    
    if ( !*argument)
    {
        if ( (pal = find_alias(ch, arg)) != NULL )
        {
            DISPOSE(pal->name);
            DISPOSE(pal->cmd);
            UNLINK(pal, ch->pcdata->first_alias, ch->pcdata->last_alias, next, prev);
            DISPOSE(pal);
            send_to_char("Alias usuniety.\n\r", ch);
        } else
            send_to_char("Taki alias nie istnieje.\n\r", ch);
        return;
    }
    
    if ( (pal=find_alias(ch, arg)) == NULL )
    {
        CREATE(pal, ALIAS_DATA, 1);
        pal->name = str_dup(arg);
        pal->cmd  = str_dup(argument);
        LINK(pal, ch->pcdata->first_alias, ch->pcdata->last_alias, next, prev);
        send_to_char("Alias stworzony.\n\r", ch);
    } 
    else 
    {
        if (pal->cmd);
        DISPOSE(pal->cmd);
        pal->cmd  = str_dup(argument);
        send_to_char("Alias zmodyfikowany.\n\r", ch);
    }
}

void free_aliases( CHAR_DATA *ch )
{
    ALIAS_DATA *pal, *next_pal;
    
    if (!ch || !ch->pcdata)
        return;
    
    for ( pal = ch->pcdata->first_alias; pal; pal = next_pal )
    {
        next_pal = pal->next;
        if (pal->name)
            DISPOSE(pal->name);
        if (pal->cmd)
            DISPOSE(pal->cmd);
        DISPOSE( pal );
    }
}

ALIAS_DATA *find_alias( CHAR_DATA *ch, char *argument )
{
    ALIAS_DATA *pal;
    char buf[MAX_INPUT_LENGTH];;
    
    if (!ch || !ch->pcdata)
        return(NULL);
    
    one_argument(argument, buf);
    
    for (pal=ch->pcdata->first_alias;pal;pal=pal->next)
        if ( !str_prefix(buf, pal->name) )
            return(pal);
    
    return(NULL);
}

bool check_alias( CHAR_DATA *ch, char *command, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    ALIAS_DATA *alias;

    if ( ( alias = find_alias( ch,command ) ) == NULL )
	return FALSE;

    if ( !alias->cmd || !*alias->cmd )
	return FALSE;

    sprintf( arg, "%s", alias->cmd );

    if ( ch->cmd_recurse == -1 || ++ch->cmd_recurse > 50 )
    {
	if (ch->cmd_recurse!=-1)
	{
	    send_to_char( "Unable to further process command, recurses too much.\n\r", ch );
	    ch->cmd_recurse=-1;
	}
	return FALSE;
    }

/*
   {
    char buf[MAX_INPUT_LENGTH];

    sprintf(buf, "%s", alias->name);

    if ( ( alias = find_alias( ch, arg ) ) != NULL )
    {
	sprintf( arg, "Your alias '%s' calls another alias and cannot be executed.\n\r", buf );
	send_to_char( arg, ch );
	return TRUE;
    }
   }
*/
    if (argument && *argument!='\0')
    {
	strcat(arg, " ");
	strcat(arg, argument);
    }

    interpret( ch, arg );
    return TRUE;
}

