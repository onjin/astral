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
 *			Specific object creation module			    *
 ****************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"



/*
 * Make a fire.
 */
void make_fire(ROOM_INDEX_DATA *in_room, sh_int timer)
{
    OBJ_DATA *fire;

    fire = create_object( get_obj_index( OBJ_VNUM_FIRE ), 0 );
    fire->timer = number_fuzzy(timer);
    obj_to_room( fire, in_room );
    return;
}

/*
 * Make a trap.
 */
OBJ_DATA *make_trap(int v0, int v1, int v2, int v3)
{
    OBJ_DATA *trap;

    trap = create_object( get_obj_index( OBJ_VNUM_TRAP ), 0 );
    trap->timer = 0;
    trap->value[0] = v0;
    trap->value[1] = v1;
    trap->value[2] = v2;
    trap->value[3] = v3;
    return trap;
}


/*
 * Turn an object into scraps.		-Thoric
 */
void make_scraps( OBJ_DATA *obj )
{
  char buf[MAX_STRING_LENGTH];
  OBJ_DATA  *scraps, *tmpobj;
  CHAR_DATA *ch = NULL;

  separate_obj( obj );
  scraps	= create_object( get_obj_index( OBJ_VNUM_SCRAPS ), 0 );
  scraps->timer = number_range( 5, 15 );

  /* don't make scraps of scraps of scraps of ... */
  if ( obj->pIndexData->vnum == OBJ_VNUM_SCRAPS )
  {
     STRFREE( scraps->short_descr );
     scraps->short_descr = STRALLOC( "some debris" );
     STRFREE( scraps->short_descr_dop );
     scraps->short_descr_dop = STRALLOC( "some debris" );
     STRFREE( scraps->short_descr_cel );
     scraps->short_descr_cel = STRALLOC( "some debris" );
     STRFREE( scraps->short_descr_bie );
     scraps->short_descr_bie = STRALLOC( "some debris" );
     STRFREE( scraps->short_descr_nar );
     scraps->short_descr_nar = STRALLOC( "some debris" );
     STRFREE( scraps->short_descr_mie );
     scraps->short_descr_mie = STRALLOC( "some debris" );
     STRFREE( scraps->short_descr_wol );
     scraps->short_descr_wol = STRALLOC( "some debris" );
     STRFREE( scraps->description );
     scraps->description = STRALLOC( "Bits of debris lie on the ground here." );
  }
  else
  {
     sprintf( buf, scraps->short_descr, obj->short_descr );
     STRFREE( scraps->short_descr );
     scraps->short_descr = STRALLOC( buf );
     STRFREE( scraps->short_descr_dop );
     scraps->short_descr_dop = STRALLOC( buf );
     STRFREE( scraps->short_descr_cel );
     scraps->short_descr_cel = STRALLOC( buf );
     STRFREE( scraps->short_descr_bie );
     scraps->short_descr_bie = STRALLOC( buf );
     STRFREE( scraps->short_descr_nar );
     scraps->short_descr_nar = STRALLOC( buf );
     STRFREE( scraps->short_descr_mie );
     scraps->short_descr_mie = STRALLOC( buf );
     STRFREE( scraps->short_descr_wol );
     scraps->short_descr_wol = STRALLOC( buf );
     sprintf( buf, scraps->description, obj->short_descr );
     STRFREE( scraps->description );
     scraps->description = STRALLOC( buf );
  }

  if ( obj->carried_by )
  {
    act( AT_OBJECT, "$p falls to the ground in scraps!",
		  obj->carried_by, obj, NULL, TO_CHAR );
    if ( obj == get_eq_char( obj->carried_by, WEAR_WIELD )
    &&  (tmpobj = get_eq_char( obj->carried_by, WEAR_DUAL_WIELD)) != NULL )
       tmpobj->wear_loc = WEAR_WIELD;

    obj_to_room( scraps, obj->carried_by->in_room);
  }
  else
  if ( obj->in_room )
  {
    if ( (ch = obj->in_room->first_person ) != NULL )
    {
      act( AT_OBJECT, "$p is reduced to little more than scraps.",
	   ch, obj, NULL, TO_ROOM );
      act( AT_OBJECT, "$p is reduced to little more than scraps.",
	   ch, obj, NULL, TO_CHAR );
    }
    obj_to_room( scraps, obj->in_room);
  }
  if ( (obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_KEYRING
  ||    obj->item_type == ITEM_QUIVER    || obj->item_type == ITEM_CORPSE_PC)
  &&    obj->first_content )
  {
    if ( ch && ch->in_room )
    {
	act( AT_OBJECT, "The contents of $p fall to the ground.",
	   ch, obj, NULL, TO_ROOM );
	act( AT_OBJECT, "The contents of $p fall to the ground.",
	   ch, obj, NULL, TO_CHAR );
    }
    if ( obj->carried_by )
	empty_obj( obj, NULL, obj->carried_by->in_room );
    else
    if ( obj->in_room )
	empty_obj( obj, NULL, obj->in_room );
    else
    if ( obj->in_obj )
	empty_obj( obj, obj->in_obj, NULL );
  }
  extract_obj( obj );
}


/*
 * Make a corpse out of a character.
 */
void make_corpse( CHAR_DATA *ch, CHAR_DATA *killer )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *corpse;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    char *name;

    if ( IS_NPC(ch) )
    {
	name		= ch->short_descr_dop;
	if ( ch->pIndexData->vnum == MOB_VNUM_SZKIELET )
	    corpse = create_object( get_obj_index( OBJ_VNUM_SZKIELET_RESZTKI ), 0 );
	else
	    corpse = create_object(get_obj_index(OBJ_VNUM_CORPSE_NPC), 0);
	corpse->timer = 6;
	if ( ch->gold > 0 )
	{
	    if ( ch->in_room )
	    {
	      ch->in_room->area->gold_looted += ch->gold;
	      sysdata.global_looted += ch->gold/100;
	    }
	    obj_to_obj( create_money( ch->gold ), corpse );
	    ch->gold = 0;
	}

/* Cannot use these!  They are used.
	corpse->value[0] = (int)ch->pIndexData->vnum;
	corpse->value[1] = (int)ch->max_hit;
*/
/*	Using corpse cost to cheat, since corpses not sellable */
	corpse->cost     = (-(int)ch->pIndexData->vnum);
        if ( ch->pIndexData->vnum == MOB_VNUM_ZOMBIE )
	    corpse->timer = 1;
	corpse->value[2] = corpse->timer; 
	corpse->value[3] = ( int ) ch->max_hit;
	corpse->value[4] = ch->numattacks;
	corpse->value[5] = ch->level;
    }
    else
    {
	name		= ch->ncel;
	corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_PC), 0);
        if ( in_arena( ch ) )
	  corpse->timer	= 0;
	else
	  corpse->timer = 40;
        corpse->value[2] = (int)(corpse->timer/8);
	corpse->value[4] = ch->level;
	if ( CAN_PKILL( ch ) )
	  xSET_BIT( corpse->extra_flags, ITEM_CLANCORPSE );
	/* Pkill corpses get save timers, in ticks (approx 70 seconds)
	   This should be enough for the killer to type 'get all corpse'. */
	if ( CAN_PKILL(ch) && CAN_PKILL(killer) )
	  corpse->value[3] = 1;
	else
	  corpse->value[3] = (int)(ch->max_hit/20); /* Slice */
    }

    if ( CAN_PKILL( ch ) && CAN_PKILL( killer ) && ch != killer )
    {
	sprintf( buf, "%s", killer->name );
	STRFREE( corpse->action_desc );
	corpse->action_desc = STRALLOC( buf );
    }

    /* Added corpse name - make locate easier , other skills */
    if ( corpse->pIndexData->vnum == OBJ_VNUM_SZKIELET_RESZTKI )
    {
	sprintf( buf, "resztki %s", name );
	STRFREE( corpse->name );
	corpse->name = STRALLOC( buf );
    }
    else
    {
	sprintf( buf, "corpse cialo %s", name );
	STRFREE( corpse->name );
	corpse->name = STRALLOC( buf );
    }
    sprintf( buf, corpse->short_descr, name );
    STRFREE( corpse->short_descr );
    corpse->short_descr = STRALLOC( buf );

    sprintf( buf, corpse->short_descr_dop, name );
    STRFREE( corpse->short_descr_dop );
    corpse->short_descr_dop = STRALLOC( buf );

    sprintf( buf, corpse->short_descr_cel, name );
    STRFREE( corpse->short_descr_cel );
    corpse->short_descr_cel = STRALLOC( buf );

    sprintf( buf, corpse->short_descr_bie, name );
    STRFREE( corpse->short_descr_bie );
    corpse->short_descr_bie = STRALLOC( buf );

    sprintf( buf, corpse->short_descr_nar, name );
    STRFREE( corpse->short_descr_nar );
    corpse->short_descr_nar = STRALLOC( buf );

    sprintf( buf, corpse->short_descr_mie, name );
    STRFREE( corpse->short_descr_mie );
    corpse->short_descr_mie = STRALLOC( buf );

    sprintf( buf, corpse->short_descr_wol, name );
    STRFREE( corpse->short_descr_wol );
    corpse->short_descr_wol = STRALLOC( buf );

    sprintf( buf, corpse->description, name );
    STRFREE( corpse->description );
    corpse->description = STRALLOC( buf );

    for ( obj = ch->first_carrying; obj; obj = obj_next )
    {
	obj_next = obj->next_content;
	obj_from_char( obj );
	if ( IS_OBJ_STAT( obj, ITEM_INVENTORY )
	  || IS_OBJ_STAT( obj, ITEM_DEATHROT ) )
	    extract_obj( obj );
	else
	    obj_to_obj( obj, corpse );
    }

    obj_to_room( corpse, ch->in_room );
    return;
}



void make_blood( CHAR_DATA *ch )
{
	OBJ_DATA *obj;

	obj		= create_object( get_obj_index( OBJ_VNUM_BLOOD ), 0 );
	obj->timer	= number_range( 2, 4 );
	obj->value[1]   = number_range( 3, UMIN(5, ch->level) );
	obj_to_room( obj, ch->in_room );
}


void make_bloodstain( CHAR_DATA *ch )
{
	OBJ_DATA *obj;

	obj		= create_object( get_obj_index( OBJ_VNUM_BLOODSTAIN ), 0 );
	obj->timer	= number_range( 1, 2 );
	obj_to_room( obj, ch->in_room );
}


/*
 * make some coinage
 */
OBJ_DATA *create_money( int amount )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;

    if ( amount <= 0 )
    {
	bug( "Create_money: zero or negative money %d.", amount );
	amount = 1;
    }

    if ( amount == 1 )
    {
	obj = create_object( get_obj_index( OBJ_VNUM_MONEY_ONE ), 0 );
    }
    else
    {
	obj = create_object( get_obj_index( OBJ_VNUM_MONEY_SOME ), 0 );
	sprintf( buf, obj->short_descr, amount );
	STRFREE( obj->short_descr );
	obj->short_descr = STRALLOC( buf );
	STRFREE( obj->short_descr_dop );
	obj->short_descr_dop = STRALLOC( buf );
	STRFREE( obj->short_descr_cel );
	obj->short_descr_cel = STRALLOC( buf );
	STRFREE( obj->short_descr_bie );
	obj->short_descr_bie = STRALLOC( buf );
	STRFREE( obj->short_descr_nar );
	obj->short_descr_nar = STRALLOC( buf );
	STRFREE( obj->short_descr_mie );
	obj->short_descr_mie = STRALLOC( buf );
	STRFREE( obj->short_descr_wol );
	obj->short_descr_wol = STRALLOC( buf );
	obj->value[0]	 = amount;
    }

    return obj;
}
