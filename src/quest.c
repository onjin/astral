/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*       ROM 2.4 is copyright 1993-1995 Russ Taylor                         *
*       ROM has been brought to you by the ROM consortium                  *
*           Russ Taylor (rtaylor@pacinfo.com)                              *
*           Gabrielle Taylor (gtaylor@pacinfo.com)                         *
*           Brian Moore (rom@rom.efn.org)                                  *
*       By using this code, you have agreed to follow the terms of the     *
*       ROM license, in the file Rom24/doc/rom.license                     *
***************************************************************************/

/***************************************************************************
*  Automated Quest code written by Vassago of MOONGATE, moongate.ams.com   *
*  4000. Copyright (c) 1996 Ryan Addams, All Rights Reserved. Use of this  * 
*  code is allowed provided you add a credit line to the effect of:        *
*  "Quest Code (c) 1996 Ryan Addams" to your logon screen with the rest    *
*  of the standard diku/rom credits. If you use this or a modified version *
*  of this code, let me know via email: moongate@moongate.ams.com. Further *
*  updates will be posted to the rom mailing list. If you'd like to get    *
*  the latest version of quest.c, please send a request to the above add-  *
*  ress. Quest Code v2.00.                                                 *
***************************************************************************/

/***************************************************************************
*  Ported to SMAUG by Vir of Eternal Struggle (es.mudservices.com 4321)    *
*  Additional changes to make life easier also by Vir.  Quest Code         *
*  originally (C)opyright 1996 Ryan Addams of MOONGATE.  Thanx for the     *
*  code, Ryan!! For more SMAUG code, e-mail "leckey@rogers.wave.ca"        *
***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mud.h"

/* Object vnums for Quest Rewards */

#define QUEST_ITEM1 29500
#define QUEST_ITEM2 29501
#define QUEST_ITEM3 29502
#define QUEST_ITEM4 29503
#define QUEST_ITEM5 29504


#define QUEST_VALUE1 15000
#define QUEST_VALUE2 10000
#define QUEST_VALUE3 5000
#define QUEST_VALUE4 7500
#define QUEST_VALUE5 100000

/* Object vnums for object quest 'tokens'. In Moongate, the tokens are
   things like 'the Shield of Moongate', 'the Sceptre of Moongate'. These
   items are worthless and have the rot-death flag, as they are placed
   into the world when a player receives an object quest. */

#define QUEST_OBJQUEST1 29505
#define QUEST_OBJQUEST2 29506
#define QUEST_OBJQUEST3 29507
#define QUEST_OBJQUEST4 29508
#define QUEST_OBJQUEST5 29509

/* Local functions */

void generate_quest	args(( CHAR_DATA *ch, CHAR_DATA *questman ));
void quest_update	args(( void ));
bool qchance            args(( int num ));

bool qchance( int num )
{
 if (number_range(1,100) <= num) return TRUE;
 else return FALSE;
}

/* The main quest function */

void do_aquest(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *questman;
    OBJ_DATA *obj=NULL, *obj_next;
    bool qobj_found;
    OBJ_INDEX_DATA *obj1, *obj2, *obj3, *obj4, *obj5;
    OBJ_INDEX_DATA *questinfoobj;
    MOB_INDEX_DATA *questinfo;
    char buf [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if ( IS_NPC( ch ) )
	return;
    
    if (!strcmp(arg1, "giveup"))
    {
	if ( xIS_SET(ch->act, PLR_QUESTOR) )
	{
            xREMOVE_BIT(ch->act, PLR_QUESTOR);
	    ch->nextquest = ch->countdown / 2 + 10;
    	    ch->questgiver = NULL;
    	    ch->countdown = 0;
    	    ch->questmob = 0;
	    ch->questobj = 0;
	    sprintf( buf, "Rezygnujesz z misji.\n\rDo podjecia nastepnej misji zostaje ci %d minut.\n\r", ch->nextquest);
	    send_to_char( buf, ch );
	}
	else
	{
	sprintf( buf, "Nie wykonujesz teraz zadnej misji.\n\r");
	send_to_char( buf, ch );
	}
    return;
    }
    		    
    if (!strcmp(arg1, "info"))
    {
        if (xIS_SET(ch->act, PLR_QUESTOR))
	{
	    if (ch->questmob == -1 && ch->questgiver->short_descr != NULL)
	    {
		sprintf(buf, "Twoja misja zostala juz ZAKONCZONA!\n\rWracaj do Sensei Nozono poki czas!\n\r");
		send_to_char(buf, ch);
	    }
	    else if (ch->questobj > 0)
	    {
                questinfoobj = get_obj_index(ch->questobj);
		if (questinfoobj != NULL)
		{
		    sprintf(buf, "Twoja misja polega na odnalezieniu %s!\n\r",questinfoobj->short_descr_dop);
		    send_to_char(buf, ch);
		}
		else send_to_char("Nie wypelniasz aktualnie zadnej misji.\n\r",ch);
		return;
	    }
	    else if (ch->questmob > 0)
	    {
                questinfo = get_mob_index(ch->questmob);
		if (questinfo != NULL)
		{
	            sprintf(buf, "Twoja misja polega na zabiciu %s!\n\r",questinfo->short_descr);
		    send_to_char(buf, ch);
		}
		else send_to_char("Nie wypelniasz aktualnie zadnej misji.\n\r",ch);
		return;
	    }
	}
	else
	    send_to_char("Nie wypelniasz aktualnie zadnej misji.\n\r",ch);
	return;
    }
    if (!strcmp(arg1, "points"))
    {
	sprintf(buf, "Posiadasz %d pkt. questowych(y).\n\r",ch->questpoints);
	send_to_char(buf, ch);
	if ( ch ->level > 14 && ch->level < 50 )
	{    
	    sprintf(buf, "Potrzebujesz jeszcze %d pkt. do zdobycia na poziom.\n\r",(( ch->level + 1) * 6 ) - ch->questlevel);
	    send_to_char(buf, ch);
	}
	return;
    }
    else if (!strcmp(arg1, "time"))
    {
        if (!xIS_SET(ch->act, PLR_QUESTOR))
	{
	    send_to_char("Nie wypelniasz aktualnie zadnej misji.\n\r",ch);
	    if (ch->nextquest > 1)
	    {
		sprintf(buf, "Za %d min. mozesz poprosic o nastepna misje.\n\r",ch->nextquest);
		send_to_char(buf, ch);
	    }
	    else if (ch->nextquest == 1)
	    {
		sprintf(buf, "Juz za niecala minute mozesz poprosic o nastepna misje.\n\r");
		send_to_char(buf, ch);
	    }
	}
        else if (ch->countdown > 0)
        {
	    sprintf(buf, "Czas do zakonaczenia misji: %d\n\r",ch->countdown);
	    send_to_char(buf, ch);
	}
	return;
    }

/* Checks for a character in the room with spec_questmaster set. This special
   procedure must be defined in special.c. You could instead use an 
   ACT_QUESTMASTER flag instead of a special procedure. */

    for ( questman = ch->in_room->first_person; questman != NULL; questman = questman->next_in_room )
    {
	if (!IS_NPC(questman)) continue;
        if (questman->spec_fun == spec_lookup( "spec_questmaster" )) break;
    }

    if (questman == NULL || questman->spec_fun != spec_lookup( "spec_questmaster" ))
    {
        send_to_char("Hmm nie mozesz tu tego uczynic.\n\r",ch);
        return;
    }

    if ( questman->position == POS_FIGHTING)
    {
	send_to_char("Chwilke, poczekaj az walka sie skonczy.\n\r",ch);
        return;
    }

    ch->questgiver = questman;

/* And, of course, you will need to change the following lines for YOUR
   quest item information. Quest items on Moongate are unbalanced, very
   very nice items, and no one has one yet, because it takes awhile to
   build up quest points :> Make the item worth their while. */

    obj1 = get_obj_index(QUEST_ITEM1);
    obj2 = get_obj_index(QUEST_ITEM2);
    obj3 = get_obj_index(QUEST_ITEM3);
    obj4 = get_obj_index(QUEST_ITEM4);
    obj5 = get_obj_index(QUEST_ITEM5);

    if ( obj1 == NULL || obj2 == NULL || obj3 == NULL || obj4 == NULL || obj5 == NULL )
    {
     bug("Error loading quest objects. Char: ", ch->name);
     return;
    }

    if (!strcmp(arg1, "list"))
    {
        act(AT_PLAIN,"$n pyta $B o liste przedmiotow.",ch,NULL,questman,TO_ROOM); 
	act(AT_PLAIN,"Pytasz $B o liste przedmiotow.",ch,NULL,questman,TO_CHAR);
	sprintf(buf, "Lista przedmiotow do kupna:\n\r\n\r\
[1] %dqp.......%s\n\r\
[2] %dqp.......%s\n\r\
[3] %dqp........%s\n\r\
[4] %dqp........%s\n\r\
[5] %dqp......%s\n\r\
[6] 500qp........30 praktyk\n\r\
[7] 500qp........10,000,000 sztuk zlota\n\r",
QUEST_VALUE1, obj1->short_descr, QUEST_VALUE2, obj2->short_descr, 
QUEST_VALUE3, obj3->short_descr, QUEST_VALUE4, obj4->short_descr, 
QUEST_VALUE5, obj5->short_descr);

	send_to_char(buf, ch);
	return;
    }

    else if (!strcmp(arg1, "buy"))
    {
	if (arg2[0] == '\0')
	{
	    send_to_char("By cos kupic wpisz 'QUEST BUY <przedmiot>'.\n\r",ch);
	    return;
	}
        if (is_name(arg2, "1"))
	{
            if (ch->questpoints >= QUEST_VALUE1)
	    {
                ch->questpoints -= QUEST_VALUE1;
	        obj = create_object(get_obj_index(QUEST_ITEM1),ch->level);
	    }
	    else
	    {
		sprintf(buf, "Niestety, %s, nie posiadasz wymaganej ilosci punktow questowych.",ch->nwol);
		do_say(questman,buf);
		return;
	    }
	}
        else if (is_name(arg2, "2"))
	{
            if (ch->questpoints >= QUEST_VALUE2)
	    {
                ch->questpoints -= QUEST_VALUE2;
	        obj = create_object(get_obj_index(QUEST_ITEM2),ch->level);
	    }
	    else
	    {
		sprintf(buf, "Niestety, %s, nie posiadasz wymaganej ilosci punktow questowych.",ch->nwol);
		do_say(questman,buf);
		return;
	    }
	}
        else if (is_name(arg2, "3"))
	{
            if (ch->questpoints >= QUEST_VALUE3)
	    {
                ch->questpoints -= QUEST_VALUE3;
	        obj = create_object(get_obj_index(QUEST_ITEM3),ch->level);
	    }
	    else
	    {
		sprintf(buf, "Niestety, %s, nie posiadasz wymaganej ilosci punktow questowych.",ch->nwol);
		do_say(questman,buf);
		return;
	    }
	}
        else if (is_name(arg2, "4"))
	{
            if (ch->questpoints >= QUEST_VALUE4)
	    {
                ch->questpoints -= QUEST_VALUE4;
	        obj = create_object(get_obj_index(QUEST_ITEM4),ch->level);
	    }
	    else
	    {
		sprintf(buf, "Niestety, %s, nie posiadasz wymaganej ilosci punktow questowych.",ch->nwol);
		do_say(questman,buf);
		return;
	    }
	}
        else if (is_name(arg2, "5"))
	{
            if (ch->questpoints >= QUEST_VALUE5)
	    {
                ch->questpoints -= QUEST_VALUE5;
	        obj = create_object(get_obj_index(QUEST_ITEM5),ch->level);
	    }
	    else
	    {
		sprintf(buf, "Niestety, %s, nie posiadasz wymaganej ilosci punktow questowych.",ch->nwol);
		do_say(questman,buf);
		return;
	    }
	}
        else if (is_name(arg2, "6"))
	{
	    if (ch->questpoints >= 500)
	    {
		ch->questpoints -= 500;
	        ch->practice += 30;
    	        act(AT_MAGIC,"$N daje 30 praktyk $x.", ch, NULL, questman, 
			TO_ROOM );
    	        act(AT_MAGIC,"$N daje Ci 30 praktyk.",   ch, NULL, questman, 
			TO_CHAR );
	        return;
	    }
	    else
	    {
		sprintf(buf, "Niestety, %s, nie posiadasz wymaganej ilosci punktow questowych.",ch->nwol);
		do_say(questman,buf);
		return;
	    }
	}
        else if (is_name(arg2, "7"))
	{
	    if (ch->questpoints >= 500)
	    {
		ch->questpoints -= 500;
                ch->gold += 10000000;
                act(AT_MAGIC,"$N daje 10,000,000 sztuk zlota $x.", ch, NULL, 
			questman, TO_ROOM );
                act(AT_MAGIC,"$N przekazuje Ci troche zlota.",   ch, NULL, 
			questman, TO_CHAR );
	        return;
	    }
	    else
	    {
		sprintf(buf, "Niestety, %s, nie posiadasz wymaganej ilosci punktow questowych.",ch->nwol);
		do_say(questman,buf);
		return;
	    }
	}
	else
	{
	    sprintf(buf, "Nie posiadam czegos takiego, %s.",ch->nwol);
	    do_say(questman, buf);
	}
	if (obj != NULL)
	{
            act(AT_PLAIN,"$N daje cos $x.", ch, obj, questman, TO_ROOM );
            act(AT_PLAIN,"$N wrecz Ci twa nagrode.",   ch, obj, questman, TO_CHAR );
	    obj_to_char(obj, ch);
	}
	return;
    }
    else if (!strcmp(arg1, "request"))
    {
	if ( ch->position == POS_SLEEPING)
	{
	sprintf( buf, "Sensei Nazono mowi 'Moj dawny wrog %s, stara sie mnie zabic i musi zostac wyeliminowany.'\n\r", ch->name);
	send_to_char ( buf, ch );
	sprintf( buf, "Sensei Nazono mowi 'Czas na wykonanie tego zadania mija z chwila gdy sie obudzisz'.\n\r");
	send_to_char ( buf, ch );
	return;
	}
	
        act(AT_PLAIN,"$n pyta $B o zadanie.", ch, NULL, questman, TO_ROOM); 
	act(AT_PLAIN,"Pytasz $B o zadanie.",ch, NULL, questman, TO_CHAR);
        if (xIS_SET(ch->act, PLR_QUESTOR))
	{
            sprintf(buf, "Jestes w trakcie misji!\n\rLepiej sie pospiesz i ja ukoncz!");
	    do_say(questman, buf);
	    return;
	}
	if (ch->nextquest > 0)
	{
	    sprintf(buf, "Twa odwaga mnie zadziwia %s, lecz daj tez szanse innym.",ch->nwol);
	    do_say(questman, buf);
	    sprintf(buf, "Sprobuj pozniej.");
	    do_say(questman, buf);
	    return;
	}

	sprintf(buf, "Dziekuje, %s!",ch->name);
	do_say(questman, buf);

	generate_quest(ch, questman);

        if (ch->questmob > 0 || ch->questobj > 0)
	{
            ch->countdown = number_range(10,30);
            xSET_BIT(ch->act, PLR_QUESTOR);
	    sprintf(buf, "Pozostalo Ci %d minut do ukonczenia misji.",ch->countdown);
	    do_say(questman, buf);
	    sprintf(buf, "Niech Bogowie beda z Toba!");
	    do_say(questman, buf);
	}
	return;
    }
    else if (!strcmp(arg1, "complete"))
    {
	if (ch->position == POS_SLEEPING)
	{
	sprintf( buf, "Sensei Nazono mowi 'W nagrode otrzymujesz mily sen o questpointach'.\n\r");
	send_to_char( buf, ch );
	return;
	}
        act(AT_PLAIN,"$n informuje $B o ukonczeniu swej misji.", ch, NULL, questman, 
		TO_ROOM); 
        act(AT_PLAIN,"Informujesz $B o ukonczeniu swej misji.",ch, NULL, 
		questman, TO_CHAR);
	if (ch->questgiver != questman)
	{
	    sprintf(buf, "Hmm.. nie zlecalem Ci zadnego zadania! Musisz mnie z kims mylic.");
	    do_say(questman,buf);
	    return;
	}

        if (xIS_SET(ch->act, PLR_QUESTOR))
	{
	    if (ch->questmob == -1 && ch->countdown > 0)
	    {
		int reward, pointreward, pracreward;
		
		
		
                reward = number_range(10000,150000);
                pointreward = number_range(15,50) + ch->countdown;

		sprintf(buf, "Moje gratulacje!");
		do_say(questman,buf);
		sprintf(buf,"W nagrode otrzymujesz %d punktow questowych, oraz %d sztuk zlota.",pointreward,reward);
		do_say(questman,buf);
                if (qchance(15))
		{
                    pracreward = number_range(1,5);
		    sprintf(buf, "Otrzymujesz %d praktyk!\n\r",pracreward);
		    send_to_char(buf, ch);
		    ch->practice += pracreward;
		}

                xREMOVE_BIT(ch->act, PLR_QUESTOR);
	        ch->questgiver = NULL;
	        ch->countdown = 0;
	        ch->questmob = 0;
		ch->questobj = 0;
	        ch->nextquest = 10;
		ch->gold += reward;
		ch->questpoints += pointreward;
		if ( ch->level > 14 && ch->level < 50 )
		    ch->questlevel += pointreward;
		if ( ch->questlevel > (( ch->level + 1 ) * 6 ))
		    ch->questlevel = (( ch->level +1 ) * 6 );
	        return;
	    }
	    else if (ch->questobj > 0 && ch->countdown > 0)
	    {
		qobj_found = FALSE;
		for ( obj = ch->first_carrying; obj; obj = obj->next )
		{
		    if ( ( obj->carried_by == ch ) 
		    && ( obj->pIndexData->vnum == ch->questobj ) )
		    {	
			qobj_found = TRUE;
			break;
		    }
		}
		if (qobj_found == TRUE)
		{
		    int reward, pointreward, pracreward;

                    reward = number_range(5000,100000);
                    pointreward = number_range(10,35) + ch->countdown;

		    act(AT_PLAIN,"Dajesz $v $X.",ch, obj, questman, TO_CHAR);
		    act(AT_PLAIN,"$n daje $v $X.",ch, obj, questman, TO_ROOM);

	    	    sprintf(buf, "Moje gratulacje!");
		    do_say(questman,buf);
		    sprintf(buf,"W nagrode otrzymujesz %d punktow questowych, oraz %d sztuk zlota.",pointreward,reward);
		    do_say(questman,buf);
                    if (qchance(15))
		    {
		        pracreward = number_range(1,6);
		        sprintf(buf, "Otrzymujesz %d praktyk!\n\r",pracreward);
		        send_to_char(buf, ch);
		        ch->practice += pracreward;
		    }

                    xREMOVE_BIT(ch->act, PLR_QUESTOR);
	            ch->questgiver = NULL;
	            ch->countdown = 0;
	            ch->questmob = 0;
		    ch->questobj = 0;
	            ch->nextquest = 10;
		    ch->gold += reward;
		    ch->questpoints += pointreward;
		    if ( ch->level > 14 )
			ch->questlevel += pointreward;
		    if ( ch->questlevel > (( ch->level + 1 ) * 6 ))
			ch->questlevel = (( ch->level +1 ) * 6 );
		    extract_obj(obj);
		    return;
		}
		else
		{
		    sprintf(buf, "Twoja misja nie jest jeszcze ukonczona, ale masz jeszcze czas!");
		    do_say(questman, buf);
		    return;
		}
		return;
	    }
	    else if ((ch->questmob > 0 || ch->questobj > 0) && ch->countdown > 0)
	    {
	        sprintf(buf, "Twoja misja nie jest jeszcze ukonczona, ale masz jeszcze czas!");
		do_say(questman, buf);
		return;
	    }
	}
	if (ch->nextquest > 0)
	    sprintf(buf,"Twa misja nie zostala ukonczona w odpowiednim czasie lub nie wypelniales zadnej misji!");
	else sprintf(buf, "Musisz najpierw poprosic (REQUEST) o misje, %s.",ch->nwol);
	do_say(questman, buf);
	return;
    }

    send_to_char("Mozliwe komendy: POINTS INFO TIME REQUEST COMPLETE GIVEUP LIST BUY.\n\r",ch);
    send_to_char("By dowiedziec sie wiecej, wpisz 'HELP AQUEST'.\n\r",ch);
    return;
}

void generate_quest(CHAR_DATA *ch, CHAR_DATA *questman)
{
    CHAR_DATA *victim;
    MOB_INDEX_DATA *vsearch;
    ROOM_INDEX_DATA *room;
    OBJ_DATA *questitem;
    char buf[MAX_STRING_LENGTH];
    long mcounter;
    int level_diff, mob_vnum;

    /*  Randomly selects a mob from the world mob list. If you don't
	want a mob to be selected, make sure it is immune to summon.
	Or, you could add a new mob flag called ACT_NOQUEST. The mob
	is selected for both mob and obj quests, even tho in the obj
	quest the mob is not used. This is done to assure the level
	of difficulty for the area isn't too great for the player. */

    for (mcounter = 0; mcounter < 99999; mcounter ++)
    {
	mob_vnum = number_range(50, 32600);

	if ( (vsearch = get_mob_index(mob_vnum) ) != NULL )
	{
	    level_diff = vsearch->level - ch->level;

		/* Level differences to search for. Moongate has 350
		   levels, so you will want to tweak these greater or
		   less than statements for yourself. - Vassago */
		
            if (((level_diff < 5 && level_diff > -35)
                || (ch->level > 30 && ch->level < 40 && vsearch->level > 15 && vsearch->level < 51)
                || (ch->level > 40 && vsearch->level > 25))
/*		&& IS_EVIL(vsearch)*/
		&& vsearch->pShop == NULL
		&& vsearch->rShop == NULL
	        && !(vsearch->spec_fun == spec_lookup( "spec_cast_adept" ))
		&& !xIS_SET(vsearch->act,ACT_NOQUEST)
		&& !xIS_SET(vsearch->act,ACT_TRAIN)
    		&& !xIS_SET(vsearch->act,ACT_PRACTICE)
    		&& !xIS_SET(vsearch->act,ACT_SCHOLAR)
    		&& !xIS_SET(vsearch->act,ACT_PROTOTYPE)
    		&& !xIS_SET(vsearch->act,ACT_BANKER)
    		&& !xIS_SET(vsearch->act,ACT_NOATTACK)
    		&& !xIS_SET(vsearch->act,ACT_PACIFIST)
    		&& !xIS_SET(vsearch->act,ACT_IMMORTAL)
		&& ( vsearch->vnum < 21000 || vsearch->vnum > 21499 )   /* Darkhaven */
		&& ( vsearch->vnum < 15000 || vsearch->vnum > 15100 )   /* Klany */
		&& ( vsearch->vnum < 29500 || vsearch->vnum > 29599 )   /* Quest */
		&& ( vsearch->vnum < 1200  || vsearch->vnum > 1299  )   /* Gods */
		&& ( vsearch->vnum < 100  || vsearch->vnum > 199  )     /* Newgate */
		&& ( vsearch->vnum < 10300  || vsearch->vnum > 10499  ) /* Akademia */
                && qchance(35)) break;
		else vsearch = NULL;
	}
    }

    if ( vsearch == NULL || ( victim = get_char_world( ch, vsearch->player_name ) ) == NULL || !IS_NPC(victim))
    {
	sprintf(buf, "Niestety, nie mam w tej chwili zadania odpowiedniego dla Ciebie.");
	do_say(questman, buf);
	sprintf(buf, "Sprobuj pozniej.");
	do_say(questman, buf);
	ch->nextquest = 2;
        return;
    }

    if ( ( room = find_location( ch, victim->name ) ) == NULL 
	|| IS_SET(room->area->flags, AFLAG_NOQUEST)
	|| IS_SET(room->room_flags, ROOM_NOQUEST)
	|| ( room->vnum >= 21000 && room->vnum <= 21499) 
	|| ( room->vnum >= 15000 && room->vnum <= 15100 )   /* Klany */
	|| ( room->vnum >= 29500 && room->vnum <= 29599 )   /* Quest */
	|| ( room->vnum >= 1200  && room->vnum <= 1299  )   /* Gods */
	|| ( room->vnum >= 100   && room->vnum <= 199  )     /* Newgate */
	|| ( room->vnum >= 10300 && room->vnum <= 10499  )) /* Akademia */
    {
	sprintf(buf, "Niestety, nie mam w tej chwili zadania odpowiedniego dla Ciebie.");
	do_say(questman, buf);
	sprintf(buf, "Sprobuj pozniej.");
	do_say(questman, buf);
	ch->nextquest = 2;
        return;
    }

    /*  40% chance it will send the player on a 'recover item' quest. */

    if (qchance(40))
    {
	int objvnum = 0;

	switch(number_range(0,4))
	{
	    case 0:
	    objvnum = QUEST_OBJQUEST1;
	    break;

	    case 1:
	    objvnum = QUEST_OBJQUEST2;
	    break;

	    case 2:
	    objvnum = QUEST_OBJQUEST3;
	    break;

	    case 3:
	    objvnum = QUEST_OBJQUEST4;
	    break;

	    case 4:
	    objvnum = QUEST_OBJQUEST5;
	    break;
	}

        questitem = create_object( get_obj_index(objvnum), ch->level );
	questitem->timer = 30;
        STRFREE( questitem->action_desc );
        questitem->action_desc = STRALLOC( ch->name );
	obj_to_room(questitem, room);
	ch->questobj = questitem->pIndexData->vnum;

        sprintf(buf, "Jakis zlodziejaszek ukradl %s z mojej prywatnej kolekcji!",questitem->short_descr_bie);
	do_say(questman, buf);
	do_say(questman, "Moj zaprzyjazniony czarnoksieznik odnalazl polozenie tego przedmiotu .");

	/* I changed my area names so that they have just the name of the area
	   and none of the level stuff. You may want to comment these next two
	   lines. - Vassago */

	sprintf(buf, "Znajduje sie on w krainie o nazwie %s w miejscu zwanym jako %s!",room->area->name, room->name);
	do_say(questman, buf);
	return;
    }

    /* Quest to kill a mob */
    
    else 
    {				
    switch(number_range(0,1))
    {
	case 0:
        sprintf(buf, "Moj dawny wrog, %s, stara sie mnie zabic.",victim->short_descr);
        do_say(questman, buf);
        sprintf(buf, "Musi wiec on byc wyeliminowany!");
        do_say(questman, buf);
	break;

	case 1:
        sprintf(buf, "Pewien lotr, %s, uciekl z lochow gdzie byl on wieziony!",victim->short_descr);
	do_say(questman, buf);
	sprintf(buf, "Podczas swej ucieczki %s zabil %d osob!",victim->short_descr, number_range(2,20));
	do_say(questman, buf);
	do_say(questman,"Kara za to jest smierc, a Ty bedziesz jej poslancem!");
	break;
    }

    if (room->name != NULL)
    {
        sprintf(buf, "Szukaj %s w poblizu miejsca o nazwie %s!",victim->short_descr_dop,room->name);
        do_say(questman, buf);

	/* I changed my area names so that they have just the name of the area
	   and none of the level stuff. You may want to comment these next two
	   lines. - Vassago */

	sprintf(buf, "To miejsce znajduje sie w krainie %s.",room->area->name);
	do_say(questman, buf);
    }
    ch->questmob = victim->pIndexData->vnum;
    }
    return;
}

/* Called from update_handler() by pulse_area */

void quest_update(void)
{
    CHAR_DATA *ch, *ch_next;

    for ( ch = first_char; ch != NULL; ch = ch_next )
    {
        ch_next = ch->next;

	if (IS_NPC(ch)) continue;

	if (ch->nextquest > 0 && !xIS_SET(ch->act,PLR_QUESTOR) )
	{
	    ch->nextquest--;

	    if (ch->nextquest == 0)
	    {
	        send_to_char("Znow mozesz prosic o nowa misje.\n\r",ch);
	        continue;
	    }
	}
        else if (xIS_SET(ch->act,PLR_QUESTOR))
        {
	    if (--ch->countdown <= 0)
	    {
    	        char buf [MAX_STRING_LENGTH];

	        ch->nextquest = 10;
	        sprintf(buf, "Czas na ukonczenie misji minal!\n\rMozesz prosic o nastepna za %d minut.\n\r",ch->nextquest);
	        send_to_char(buf, ch);
                xREMOVE_BIT(ch->act, PLR_QUESTOR);
                ch->questgiver = NULL;
                ch->countdown = 0;
                ch->questmob = 0;
	    }
	    if (ch->countdown > 0 && ch->countdown < 6)
	    {
	        send_to_char("Spiesz sie, spiesz, czas na ukonczenie misji plynie szybko!\n\r",ch);
	        continue;
	    }
        }
    }
    return;
}
